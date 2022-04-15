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
  int pos = (loc - line) + indent;

  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
}

static void error_tok(Token *tok, const char *filename, char * content, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  verror_at(filename, content, tok->row, tok->from, fmt, ap);
  exit(1);
}

static void parse_assignment()
{

}

static void parse_if()
{

}

static void parse_do_while()
{

}

static void parse_while()
{

}

static void parse_for()
{

}

static void parse_switch_case()
{

}

static void parse_fn()
{

}

static void enter_scope()
{

}

static void leave_scope()
{
    
}

static void parse_expression()
{
    
}


