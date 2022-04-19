﻿#include <cstdarg>

#include "parser.h"
#include "lex.h"

static void verror_at(const char *filename, char *input, int line_no,
                      char *loc, char *fmt, va_list ap) {
    // Find a line containing `loc`.
	line_no += 1;
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
	OperationExpression *child1 = NULL;
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
			Operation *oper = parse_primary(reader);
			if (oper) {
				child1 = new OperationExpression;
				child1->left = oper;
				child1->op_type = OperatorType::op_none;
			}
		}

		OperatorType op = getbinopr(get_operator_type(reader));
		if (op != OperatorType::op_none && priority[(int)op].left > limit) {
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
		} else {
			if (child1) node = child1;
			else {
				node = new OperationExpression;
				node->left = parse_primary(reader);
			}
		}
	}
	return node;
}

static OperationExpression * parse_operator(TokenReader *reader);

static vector<Operation *> * parse_parameter(TokenReader *reader, char close)
{
	vector<Operation *> *parameters = new vector<Operation *>;
	while (reader->peek().type != TokenType::eof) {
		OperationExpression *oper = parse_operator(reader);
		if (reader->peek().type != TokenType::sym) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
		}
		
		// fill the parameters
		if (oper->op_type == OperatorType::op_none) {
			parameters->push_back(oper->left);
			delete oper;
		}
		else {
			Operation *op = new Operation;
			op->type = OpType::op;
			op->op = new Operation::oper;
			op->op->op_oper = oper;
			parameters->push_back(op);
		}

		if (*(reader->peek().from) == close) break;
		else if (*(reader->peek().from) == ',') reader->consume();
		else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
	}
	return parameters;
}

static CallExpression * parse_function_call(TokenReader *reader)
{
	if (reader->peek().type != TokenType::iden) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
	}
	CallExpression *call = new CallExpression;
	call->function_name = new IdExpression;
	call->function_name->name = reader->peek().from;
	call->function_name->name_len = reader->peek().len;
	reader->consume(); // consume function name
	reader->consume(); // consume (
	call->parameters = parse_parameter(reader, ')');
	reader->consume(); // )
	return call;
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
			else if (str_equal(t.from, "false", 5)) {

			}

			const Token &id_token = reader->get_and_read();
			if (reader->peek().type == TokenType::sym) {
				
				// ( . [ 
				char ch = *(reader->peek().from);
				switch (ch)
				{
				case '(':	// function call
				{
					node = new Operation;
					node->op = new Operation::oper;
					node->type = OpType::call;
					CallExpression *call = parse_function_call(reader);
					call->function_name = new IdExpression;
					call->function_name->name = id_token.from;
					call->function_name->name_len = id_token.len;
					node->op->call_oper = call;
					break;
				}
				case '[':	// index data
				{
					node = new Operation;
					node->op = new Operation::oper;
					node->type = OpType::arr;
					Array *array = new Array;
					array->name = new IdExpression;
					array->name->name = id_token.from;
					array->name->name_len = id_token.len;
					array->fields = parse_parameter(reader, ']');
					node->op->array_oper = array;
					reader->consume();
					break;
				}
					
				case '.':	// field or .. 
					break;
				default:
					// error
					error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
					break;
				}
			}
			else {
				// error
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			}
		}
		else if (reader->peek().type == TokenType::sym) {
			char ch = *(reader->peek().from);
			if (ch == '(') {		// subexp
				node = new Operation;
				node->op = new Operation::oper;
				node->type = OpType::op;
				node->op->op_oper = parse_operator(reader);
				if (reader->peek().type != TokenType::sym || *(reader->peek().from) != ')') {
					error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
				}
				reader->consume();
			}
			else if (ch == '[') {	// array construct
				node = new Operation;
				node->op = new Operation::oper;
				node->type = OpType::arr;
				Array *array = new Array;
				array->name = NULL;
				array->fields = parse_parameter(reader, ']');
				node->op->array_oper = array;
				reader->consume();
			}
			else if (ch == '{'){	// table construct
				
			}
			else {
				// error
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			}
		}
		else if (reader->peek().type == TokenType::keyword) {
			return node;
		}
		else {
			// error
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
		} 
	}
	return node;
}

static OperationExpression * parse_operator(TokenReader *reader)
{
	OperationExpression *node = NULL;
	OperationExpression *child1 = subexpr(reader, 0);
	while (child1) {
		// 到这里应该是操作符
		OperatorType op = getbinopr(get_operator_type(reader));
		if (op != OperatorType::op_none) {
			reader->consume();
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
				node->left->op = new Operation::oper;
				node->left->op->op_oper = child1;

				node->right = new Operation;
				node->right->op = new Operation::oper;
				node->right->type = OpType::op;
				node->right->op->op_oper = child2;
				child1 = node;
			}
			else {
				// TODO error
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			}
		}
		else {
			if (child1) node = child1;
			break; // 只有一个
		}
	}
	return node;
}

static AssignmentExpression * parse_assignment(TokenReader *reader)
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
				return NULL;
			}
		}

		as->id->name = token.from;
		as->id->name_len = token.len;
		
		reader->consume();
		if (reader->peek().type == TokenType::sym && str_equal(reader->peek().from, "=", 1)) {
			reader->consume();

			const Token &val = reader->peek();
			if (val.type == TokenType::iden || val.type == TokenType::num || val.type == TokenType::str || val.type == TokenType::sym) {
				as->assign = new AssignmentExpression::assignment;
				as->assign->ass = new AssignmentExpression::assignment::assignment_union;
				OperationExpression *oper = parse_operator(reader);
				if (oper->op_type == OperatorType::op_none) {
					if (oper->left->type == OpType::call) {
						as->assign->assignment_type = AssignmentExpression::AssignmenType::call;
						as->assign->ass->call_val = oper->left->op->call_oper;
					}
					else if (oper->left->type == OpType::id) {
						as->assign->assignment_type = AssignmentExpression::AssignmenType::id;
						as->assign->ass->id_val = oper->left->op->id_oper;
					}
					else if (oper->left->type == OpType::index) {
						as->assign->assignment_type = AssignmentExpression::AssignmenType::index;
						as->assign->ass->index_val = oper->left->op->index_oper;
					}
					else {
						as->assign->assignment_type = AssignmentExpression::AssignmenType::basic;
						as->assign->ass->basic_val = new BasicValue;
						as->assign->ass->basic_val->value = new BasicValue::Value;
						switch (oper->left->type)
						{
							case OpType::num:
								as->assign->ass->basic_val->type = VariableType::t_number;
								as->assign->ass->basic_val->value->number = oper->left->op->number_oper;
								break;
							case OpType::arr:
								as->assign->ass->basic_val->type = VariableType::t_array;
								as->assign->ass->basic_val->value->array = oper->left->op->array_oper;
								break;
							case OpType::str:
								as->assign->ass->basic_val->type = VariableType::t_string;
								as->assign->ass->basic_val->value->string = oper->left->op->string_oper;
								break;
							case OpType::table:
								as->assign->ass->basic_val->type = VariableType::t_table;
								as->assign->ass->basic_val->value->table = oper->left->op->table_oper;
								break;
							case OpType::boolean:
								as->assign->ass->basic_val->type = VariableType::t_boolean;
								as->assign->ass->basic_val->value->boolean = oper->left->op->boolean_oper;
								break;
							default:
								// error
								break;
						}
					}
				}
				else {
					as->assign->ass->oper_val = oper;
					as->assign->assignment_type = AssignmentExpression::AssignmenType::operation;
				}
			}
			else error_tok(val, reader->get_file_name(), reader->get_content(), "%s", "unkown type ");
		}
		else {
			delete as;
			reader->unread();
		}
		return as;
	}
	return NULL;
}

static void enter_scope()
{

}

static void leave_scope()
{
    
}

// 严格要求 {} 
static OperationExpression * parse_if_prefix(TokenReader *reader, bool is_if)
{
	if(!is_if) {
		if (reader->peek().type != TokenType::keyword && reader->peek().len != 4 && !str_equal(reader->peek().from, "else", 4)) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid condition key word");
		}
	}
	if (reader->peek().type != TokenType::keyword && reader->peek().len != 2 && !str_equal(reader->peek().from, "if", 2)) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid condition key word");
	}

	reader->consume(); // consume if key word
	if (reader->peek().type != TokenType::sym || *(reader->peek().from) != '(') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "condition statement needs starting with ( ");
	}

	OperationExpression *oper = parse_operator(reader);
	if (!oper) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
	}

	if (reader->peek().type != TokenType::sym || *reader->peek().from != ')') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "condition statement needs ending with ) ");
	}
	reader->consume();
	if (reader->peek().type == TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "condition body needs starting with { ");
	}
	reader->consume();
	return oper;
}

static vector<BodyStatment *> * parse_expressions(TokenReader *reader);

static void parse_if_statement(TokenReader *reader, int start)
{
	IfExpStatement *cond = new IfExpStatement;
	// if else if ... else
	if (start == 0) { // if
		cond->condition = parse_if_prefix(reader, true);
	}
	else if (start == 1) { // else if
		cond->condition = parse_if_prefix(reader, false);
	}
	else if (start == 2) { // else
		if (reader->peek().type != TokenType::keyword || reader->peek().len != 4 || !str_equal(reader->peek().from, "else", 4)) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid condition key word");
		}
		reader->consume(); // consume else 
		if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "condition body needs starting with { ");
		}
		reader->consume();
		cond->condition = NULL; // init
	}
	// parse if body
	// a = 100 b = a + c func(sss) b = tb[1] do {} while();

	reader->consume();
}

static void parse_do_while_expression(TokenReader *reader)
{
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "loop expression needs { to start");
	}
	reader->consume();
	// loop body
	

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '}') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "loop expression needs { to close");
	}
	reader->consume();
	if (reader->peek().type != TokenType::keyword || !str_equal(reader->peek().from, "while", 5)) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid loop expression key word");
	}
	reader->consume();
	if (reader->peek().type != TokenType::sym || *reader->peek().from != ';') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "loop expression needs ; to close");
	}
	reader->consume();
}

static void parse_while_expression(TokenReader *reader)
{
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "loop expression needs { to start");
	}
	reader->consume();
	// while body
	// 没办法，只能一一尝试，赋值，if，while，函数调用，do while，for

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '}') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "loop expression needs } to close");
	}
	reader->consume();

}

static ForExpression * parse_for(TokenReader *reader)
{
	ForExpression *forExp = new ForExpression;


	return forExp;
}

static void parse_switch_case(TokenReader *reader)
{

}

static Function * parse_fn(TokenReader *reader)
{
	bool isLocal = false;
	int pos = reader->get_pos();
	if (reader->peek().type == TokenType::keyword && str_equal(reader->peek().from, "local", 5)) {
		isLocal = true;
		reader->consume();
	}

	if (reader->peek().type != TokenType::keyword || !str_equal(reader->peek().from, "fn", 2))
	{
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "funcion declare needs fn ke word.");
	}
	reader->consume();

	// 下划线开头先不考虑
	if (reader->peek().type != TokenType::iden) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid funcion name");
	}
	reader->consume();

	if (reader->peek().type != TokenType::sym || *reader->peek().from != '(') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "funcion declare needs ( to start.");
	}
	reader->consume();

	Function *fun = new Function;
	IdExpression *funcName = new IdExpression;
	funcName->name = reader->peek().from;
	funcName->name_len = reader->peek().len;
	fun->function_name = funcName;
	fun->is_local = isLocal;
	
	// parse parameter
	int maxParam = 30; // 最多30 个参数
	int count = 0;
	fun->parameters = new vector<IdExpression *>;
	while (reader->peek().type != TokenType::eof) {
		if (reader->peek().type == TokenType::sym && *reader->peek().from == ')') break;
		if (reader->peek().type != TokenType::iden) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid funcion parameter");
		}
		IdExpression *param = new IdExpression;
		param->name = reader->peek().from;
		param->name_len = reader->peek().len;
		fun->parameters->push_back(param);
		reader->consume();
		if (reader->peek().type == TokenType::sym)
		{
			if (*reader->peek().from != ',') {
				reader->consume();
			}
			else if (*reader->peek().from != ')') 
				break;
			else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "unkown symbol");
		}
		++count;
		if (count >= maxParam) {
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "parameter's number exceed 30");
		}
	}
	reader->consume();
	if (reader->peek().type != TokenType::sym || *reader->peek().from != '{') {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "funcion declare needs { to start");
	}
	reader->consume();
	// parse function body
	fun->body = parse_expressions(reader);
	return fun;
}

static void build_call(vector<BodyStatment *> *statements, TokenReader *reader)
{
	CallExpression *call = parse_function_call(reader);
	if (!call) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
	}
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->call_exp = call;
	statement->type = ExpressionType::call_statement;
	statements->push_back(statement);
}

static void build_assign(vector<BodyStatment *> *statements, TokenReader *reader)
{
	AssignmentExpression *ass = parse_assignment(reader);
	if (!ass) {
		error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
	}
	BodyStatment *statement = new BodyStatment;
	statement->body = new BodyStatment::body_expression;
	statement->body->assign_exp = ass;
	statement->type = ExpressionType::assignment_statement;
	statements->push_back(statement);
}

static vector<BodyStatment *> * parse_expressions(TokenReader *reader)
{
	vector<BodyStatment *> *statements = new vector<BodyStatment *>;
	while (reader->peek().type != TokenType::eof) {
		switch (reader->peek().type)
		{
		case TokenType::iden:
		{
			reader->consume();
			if (reader->peek().type == TokenType::sym) 	{
				if (*reader->peek().from == '(') {
					reader->unread();
					build_call(statements, reader);
				}
				else if (*reader->peek().from == '[') {

				}
				else if (*reader->peek().from == '=') {
					reader->unread();
					build_assign(statements, reader);
				}
				else error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			}
			else {
				error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			}
			break;
		}
		case TokenType::keyword:
		{
			if (str_equal(reader->peek().from, "if", 2)) {
				parse_if_statement(reader, 0);
			}
			else if (str_equal(reader->peek().from, "fn", 2)) {
				Function *fun = parse_fn(reader);
				if (!fun) {
					error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
				}
			}
			else if (str_equal(reader->peek().from, "local", 2)){
				int pos = reader->get_pos();
				reader->consume();
				if (str_equal(reader->peek().from, "fn", 2)) {

				}
				else if (reader->peek().type == TokenType::iden) {
					reader->consume();
					if (*reader->peek().from == '=') {
						reader->set_pos(pos);
						build_assign(statements, reader);
					}
					else if (*reader->peek().from == '(') {
						reader->set_pos(pos);
						build_call(statements, reader);
					}
					else 
						error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
				}
				else 
					error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			}
			break;
		}
		default:
			error_tok(reader->peek(), reader->get_file_name(), reader->get_content(), "%s", "invalid statement");
			break;
		}
		
		//parse_do_while_expression(reader);
		//parse_while_expression(reader);
		//parse_for(reader);
		
	}
	return statements;
}

void parse(unordered_map<string, TokenReader *> &files)
{
	for(auto &it : files) {
		parse_expressions(it.second);
	}
}


