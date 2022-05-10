#include "utils.h"
#include "lex.h"

#include <cstdarg>

static void verror_at(const char *filename, char *input, int line_no, char *loc, char *fmt, va_list ap) {
    // Find a line containing `loc`.
	  line_no += 1;
    char *line = loc;
    while (input < line && line[-1] != '\n')
      line--;

    char *end = loc;
    while (*end && *end != '\n')
      end++;

    // Print out the line.
    int indent = fprintf(stderr, "%s:%d ", filename, line_no);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // Show the error message.
    int pos = int((loc - line)) + indent;

    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void error_tok(const Token &tok, const char *filename, char * content, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verror_at(filename, content, tok.row, tok.from, fmt, ap);
    exit(1);
}

static string _cwd;

void to_cwd(const char *exe)
{
    int len = strlen(exe) - 1;
    while (!(exe[len] == '/' || exe[len] == '\\')){
       --len;
    }
    for (int i = 0; i < len; i++) {
        _cwd.push_back(exe[i]);
    }
}

string getcwd()
{
    return _cwd;
}