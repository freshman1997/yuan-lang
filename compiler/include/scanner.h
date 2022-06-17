#ifndef __SCANNER_H__
#define __SCANNER_H__

#include <string>
#include <vector>
using std::string;
using std::vector;


class Position
{
    int begin = 0;
    int end = 0;
    int line = 0;
    int column = 0;
};

enum class TokenType
{
    number = 0,             // 数字
    str,                    // 字符串
    iden,                   // 标识符，定义的变量等
    keyword,                // 关键字
    symbol,                 // 符号
};

class Token
{
    TokenType type;
    string text;
    
};

class CharStream
{
public:
    char peek();
    char next();
    bool eof();
    Position get_posistion();

private:
    const char *data = NULL;
    int line;
    int col;
    int pos;
};

class Scanner
{
public:
    Token next();
    const Token & peek();
    const Token & peek(int i);
private:
    void getAToken();    

private:
    CharStream *stream = NULL;
    Position lastPos;

    // 用于支持预读多个 Token 的
    vector<Token> peekTokens;
};

#endif