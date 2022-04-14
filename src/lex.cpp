#include "../include/lex.h"

const Token & TokenReader::peek()
{
    return tokens[idx];
}

void TokenReader::set_pos(int i)
{
    if (i < 0 || i >= tokens.size() || tokens.empty()) return;
    this->idx = i;
}

const Token & TokenReader::get_and_read()
{
    return tokens[idx++];
}

void TokenReader::unread()
{
    --idx;
}

const char * TokenReader::get_file_name() const
{
    return filename;
}

void TokenReader::set_file_handle(FILE *fd)
{
    this->file = fd;
}

void TokenReader::add_token(Token &token)
{
    tokens.push_back(token);
}

static char * read_all_the_file(const char *filename, TokenReader &reader)
{
    
}

void tokenize(const char *filename, TokenReader &reader)
{
    char *content = read_all_the_file(filename, reader);
    if (!content) {
        error("can not read the file: ", filename);
    }



}
