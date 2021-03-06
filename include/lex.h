#ifndef __LEX_H__
#define __LEX_H__
#include <iostream>
#include <cstdio>
#include <vector>

#include "utils.h"

using namespace std;

enum class TokenType
{
    iden = 0,       // 标识符
    num = 1,        // 数字
    sym = 2,        // 符号
    keyword = 3,    // 关键字
    str = 4,        // 字符串
    eof = 5,        // eof
};

struct Token
{
    TokenType type;         // 类型
    char *from;             // 开始位置
    int len;                // 长度
    int row;                // 行号
    int col;                // 列
    char *str;              // 如果存在字符串
    double val;             // 如果是数字，应该存在值
};

class TokenReader
{
public:
    const Token & peek();
    void set_pos(int i);
    const Token & get_and_read();
    void consume();
    void unread();
    int get_pos();
    const Token & get(int i);

    Token * peek_last_one();

    const char * get_file_name() const;
    void add_token(Token &token);
    int _sz() const;
    void set_sz(int sz);
    void set_file_name(const char *fn);
    Token & get_last();
    void set_content(char *c);
    char * get_content();

private:
    const char *filename = NULL;
    int sz = 0;                     // 文件内容大小
    int idx = 0;
    char *content = NULL;
    vector<Token> tokens;
};

void tokenize(const char *filename, TokenReader &reader);

#endif