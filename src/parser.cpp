#include <cstdarg>

#include "parser.h"
#include "lex.h"

static void verror_at(const char *filename, char *input, int line_no,
                      char *loc, char *fmt, va_list ap) {
    // Find a line containing `loc`.
    char *line = loc;
    while (input < line && line[-1] != '\n')
      line--;

    char *end = loc;
    while (*end && *end != '\n')
      end++;

    // Print out the line.
    int indent = fprintf(stderr, "%s:%d: ", filename, line_no);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // Show the error message.
    int pos = int((loc - line)) + indent;

    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

static void error_tok(const Token &tok, const char *filename, char * content, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verror_at(filename, content, tok.row, tok.from, fmt, ap);
    exit(1);
}

static OperatorType getbinopr (int op) {
  switch (op) {
    case '+': return OperatorType::op_add;
    case '-': return OperatorType::op_sub;
    case '*': return OperatorType::op_mul;
    case '/': return OperatorType::op_div;
    case '%': return OperatorType::op_mod;
    case (int)OperatorType::op_concat: return OperatorType::op_concat;
    case (int)OperatorType::op_ne: return OperatorType::op_ne;
    case (int)OperatorType::op_equal: return OperatorType::op_equal;
    case '<': return OperatorType::op_lt;
    /*case TK_LE: return OPR_LE;
    case '>': return OPR_GT;
    case TK_GE: return OPR_GE;
    case TK_AND: return OPR_AND;
    case TK_OR: return OPR_OR;*/
    default: return OperatorType::op_none;
  }
}

static const struct {
  unsigned char left;  /* left priority for each binary operator */
  unsigned char right; /* right priority */
} priority[] = {  /* ORDER OPR */
   {6, 6}, {6, 6}, {7, 7}, {7, 7}, {7, 7},  /* `+' `-' `/' `%' */
   {5, 4},                          /* concat (right associative) */
   {3, 3}, {3, 3},                  /* equality and inequality */
   {3, 3}, {3, 3}, {3, 3}, {3, 3},  /* order */
   {2, 2}, {1, 1}                   /* logical (and/or) */
};

#define UNARY_PRIORITY	8  /* priority for unary operators */

static OperatorType getunopr (int op) {
  switch (op) {
    case '!': return OperatorType::op_not;
    case -'-': return OperatorType::op_unary_sub;
    case '#': return OperatorType::op_len;
    default: return OperatorType::op_none;
  }
}

static int get_operator_type(TokenReader *reader)
{
	const Token &token = reader->peek();
	if (token.type != TokenType::sym) return -1;
	static unordered_map<string, int> symbol_map;
	if (symbol_map.empty()) {
		symbol_map["!"] = '!';
		symbol_map["u-"] = -'-';
		symbol_map["#"] = '#';

		symbol_map["+"] = '+';
		symbol_map["-"] = '-';
		symbol_map["*"] = '*';
		symbol_map["/"] = '/';
		symbol_map["%"] = '%';
		
		symbol_map["&&"] = '&';
		symbol_map["||"] = '|';
		symbol_map[">>"] = '>';
		symbol_map["<<"] = '<';

		symbol_map["|"] = -'|';
		symbol_map["&"] = -'&';
		symbol_map[">"] = -'>';
		symbol_map["<"] = -'<';
		symbol_map["<="] = '<' + '=';
		symbol_map["<="] = '>' + '=';

		symbol_map["=="] = '=';
		symbol_map["!="] = '!' + '=';
	}

	string t;
	const char *p = token.from;
	for(int i = 0; i < token.len; ++i) 
		t.push_back(p[i]);
	
	if (t == "-") {

	}

	return symbol_map.count(t) > 0 ? symbol_map[t] : -1;
}

static void CheckOperTypeExpect()
{

}

static Operation * parse_primary(TokenReader *reader);

/*
** subexpr -> (simpleexp | unop subexpr) { binop subexpr }
** where `binop' is any binary operator with a priority higher than `limit'
*/
static OperationExpression * subexpr(TokenReader *reader, unsigned int limit) {

	/*
		a = 100 + 30 b = 20 arr = [1, 2, 3]
	 */
	OperationExpression *node = NULL;
	OperationExpression *child1;
	if (reader->peek().type != TokenType::eof) {
		OperatorType uop = getunopr(get_operator_type(reader));
		if (uop != OperatorType::op_none) {
			reader->consume();
			child1 = new OperationExpression;
			child1->op_type = uop;
			child1->left = parse_primary(reader);
		}

		// 100 + 18 + 20 / 2 + 100 - 20 需要生成以下的树
		/*
										  - 
										/   \
				+					   +     20
			  /   \					 /  \
			 +		'/'			   +    100         
			/ \     /  \          /   \            
		 100   18  20   2       +      '/'   
		 					   /  \    /  \
							100	  18  20   2
							  
		*/

		else if (reader->peek().type != TokenType::sym) {
			child1 = new OperationExpression;
			child1->left = parse_primary(reader);
			child1->op_type = OperatorType::op_none;
		}

		OperatorType op = getbinopr(get_operator_type(reader));
		if (op != OperatorType::op_none && priority[(int)op].left >= limit) {
			reader->consume();
			OperationExpression *exp = subexpr(reader, priority[(int)op].right);
			if (exp) {
				node = new OperationExpression;
				node->op_type = op;
				node->op_type = op;
				Operation *child2 = new Operation;
				child2->op = new Operation::oper;
				child2->op->op_oper = exp;
				child2->type = OpType::op;
				node->right = child2;
				node->left = child1 ? child1->left : NULL;
			}
			else {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			}
		}
	}
	return node;
}

static Operation * parse_primary(TokenReader *reader)
{
	// prefixexp { `.' NAME | `[' exp `]' | `:' NAME funcargs | funcargs }
	// id, number, string, array, table, function call, index, condition
	// a = +100;
	Operation *node = NULL;
	if (reader->peek().type != TokenType::eof) {
		// 数字
		// +-
		if (reader->peek().type == TokenType::num) {
			node = new Operation;
			node->op = new Operation::oper;
			node->type = OpType::num;
			Number *num = new Number;
			num->raw = reader->peek().from;
			num->raw_len = reader->peek().len;
			num->val = reader->peek().val;
			node->op->number_oper = num;
			reader->consume();
		}
		else if (reader->peek().type == TokenType::str)
		{
			node = new Operation;
			node->op = new Operation::oper;
			node->type = OpType::str;
			String *str = new String;
			str->raw = reader->peek().from;
			str->raw_len = reader->peek().len;
			node->op->string_oper = str;
			reader->consume();
		}
		else if (reader->peek().type == TokenType::iden) {
			int curPos = reader->get_pos();
			const Token &t = reader->get_and_read();
			if (str_equal(t.from, "nil", 3)) {

			}
			else if (str_equal(t.from, "true", 4)) {

			}
			else if (str_equal(t.from, "false", 4)) {

			}

			if (reader->peek().type == TokenType::sym) {
				// ( . [ 
				char ch = *(reader->peek().from);
				switch (ch)
				{
				case '(':	// function call
					break;
				case '[':	// index data
					break;
				case '.':	// field or .. 
					break;
				default:
					// error
					break;
				}
			}
			else {
				// error
			}
		}
		else if (reader->peek().type == TokenType::sym) {
			char ch = *(reader->peek().from);
			if (ch == '(') {		// subexp

			}
			else if (ch == '[') {	// array construct

			}
			else if (ch == '{'){	// table construct
				
			}
			else {
				// error
			}
		}
		else {} // error
	}
	return node;
}

static OperationExpression * parse_opertor(TokenReader *reader)
{
	OperationExpression *node = NULL;
	OperationExpression *child1 = subexpr(reader, 0);
	while (child1) {
		// 到这里应该是操作符
		OperatorType op = getbinopr(get_operator_type(reader));
		if (op != OperatorType::op_none) {
			OperationExpression *child2 = subexpr(reader, priority[(int)op].left);
			if (child2) {
				node = new OperationExpression;
				node->op_type = op;
				if (child2->right = NULL) {
					child2->right = child2->left;
					child2->left = new Operation;
					child2->left->type = OpType::op;
					child2->left->op->op_oper = child1;
					child1 = child2;
					continue;
				}

				node->left = new Operation;
				node->left->type = OpType::op;
				node->left->op->op_oper = child1;
				node->right = new Operation;
				node->right->type = OpType::op;
				node->right->op->op_oper = child2;
				child1 = node;
			}
			else {
				// TODO error
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			}
		}
		else break; // 只有一个
	}
	return node;
}

// subexpr -> (simpleexp | unop subexpr) { binop subexpr }
static void parse_assignment_operations(TokenReader *reader, AssignmentExpression *as)
{
	// 单个标识符，函数声明（匿名），数组声明（匿名），表声明（匿名），表达式计算（一元，二元）
	if (str_equal(reader->peek().from, "[", 1)) {

	}
}

static void parse_assignment(TokenReader *reader)
{
	const Token &token = reader->peek();
	if (token.type == TokenType::iden && is_identity(token.from, token.len)) {
		AssignmentExpression *as = new AssignmentExpression;
		as->id = new IdExpression;
		if (token.type == TokenType::keyword && token.len == 5 && str_equal(token.from, "local", 5)) {
			as->id->is_local = true;
			reader->consume();
			if (!is_identity(reader->peek().from, reader->peek().len)) {
				reader->unread();
				return;
			}
		}

		char *name = new char[reader->peek().len + 1];
		memcpy(name, reader->peek().from, reader->peek().len);
		name[reader->peek().len] = '\0';
		as->id->name = name;
		
		reader->consume();
		if (reader->peek().type == TokenType::sym && str_equal(reader->peek().from, "=", 1)) {
			reader->consume();

			const Token &val = reader->peek();
			if (val.type == TokenType::iden || val.type == TokenType::num || val.type == TokenType::str || val.type == TokenType::sym) {
				parse_opertor(reader);
			}
			else error_tok(val, reader->get_file_name(), reader->get_content(), "%s", "unkown type ");
		}
		else {
			delete as;
			reader->unread();
		}
	}
}

static void enter_scope()
{

}

static void leave_scope()
{
    
}

static void parse_if(TokenReader *reader)
{

}

static void parse_do_while(TokenReader *reader)
{

}

static void parse_while(TokenReader *reader)
{

}

static void parse_for(TokenReader *reader)
{

}

static void parse_switch_case(TokenReader *reader)
{

}

static void parse_fn(TokenReader *reader)
{

}

static void parse_expression(TokenReader *reader)
{

}

void parse(unordered_map<string, TokenReader *> &files)
{
	for(auto &it : files) {
		parse_assignment(it.second);
	}
}


