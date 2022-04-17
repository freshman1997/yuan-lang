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

static void enter_scope()
{

}

static void leave_scope()
{
    
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
				parse_assignment_operations(reader, as);
			}
			else error_tok(val, reader->get_file_name(), reader->get_content(), "%s", "unkown type ");
		}
		else {
			delete as;
			reader->unread();
		}
	}
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


