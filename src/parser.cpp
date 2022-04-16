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

static void enter_scope()
{

}

static void leave_scope()
{
    
}

static void parse_assignment(TokenReader *reader)
{
	// 三部曲
	const Token &token = reader->peek();
	if (is_identity(token.from, token.len)) {
		AssignmentExpression as;
		as.id = new IdExpression;
		if (token.type == TokenType::keyword && token.len == 5 && str_equal(token.from, "local", 5)) {
			as.id->is_local = true;
			reader->consume();
			if (!is_identity(reader->peek().from, reader->peek().len)) {
				reader->unread();
				return;
			}
		}

		char *name = new char[reader->peek().len + 1];
		memcpy(name, reader->peek().from, reader->peek().len);
		name[reader->peek().len] = '\0';
		as.id->name = name;
		
		reader->consume();
		if (reader->peek().type == TokenType::sym && str_equal(reader->peek().from, "=", 1)) {
			reader->consume();
			const Token &val = reader->peek();
			if (val.type == TokenType::num || val.type == TokenType::str || (val.type == TokenType::sym && (*(val.from) == '[' || *(val.from) == '{'))) {
				if (val.type == TokenType::num) {
					as.assignment_type = VariableType::t_number;
					as.as = new AssignmentExpression::assignment;
					as.as->basic_val = new BasicValue;
					as.as->basic_val->value = new BasicValue::Value;
					as.as->basic_val->value->number = new Number;

					as.as->basic_val->type = VariableType::t_number;
					as.as->basic_val->value->number->raw = val.from;
					as.as->basic_val->value->number->raw_len = val.len;
					char *num_str = new char[val.len + 1];
					memcpy(num_str, val.from, val.len);
					num_str[val.len] = '\0';
					as.as->basic_val->value->number->val = atoi(num_str);
					delete[] num_str;
				}
			}
			else error_tok(val, reader->get_file_name(), reader->get_content(), "%s", "unkown type ");
		}
		else {
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


