#include <cassert>
#include "scanner.h"

CharStream::CharStream(const char *_data) : data(_data){}

char CharStream::next()
{
    char ch = this->data[pos++];
    if(ch == '\n') {
        this->line ++;
        this->col = 1;
    }else {
        this->col ++;
    }
    return ch;
}

char CharStream::peek()
{
    return this->data[pos];
}

bool CharStream::eof()
{
    return this->peek() == '\0';
}

Position CharStream::get_posistion()
{
    return {this->pos+1, this->pos+1, this->line, this->col};
}

Token Scanner::next()
{
    if (peekTokens.empty()) {
        const Token &t = getAToken();
        peekTokens.push_back(getAToken());
        return t;
    }
    else {
        const Token &t = peekTokens.front();
        peekTokens.pop_back();
        lastPos = t.pos;
        return t;
    }
}

const Token & Scanner::peek()
{
    return this->peekTokens.front();
}

const Token & Scanner::peek(int i)
{
    if (peekTokens.size() < i) {
        int c = int(peekTokens.size()) - 1;
        while (c <= i) {
            peekTokens.push_back(getAToken());
            ++c;
        }
    }
    return this->peekTokens[i];
}

/*
 * 跳过空白字符
 */
void Scanner::skipWhiteSpace()
{
    char ch = stream->peek();
    while (ch == ' ' || ch == '\t' || ch == '\n') {
        ch = stream->next();
    }
}

/*
 * 跳过单行注释
 */
void Scanner::skipSingleLineComment()
{
    stream->next();
    //往后一直找到回车或者eof
    while(this->stream->peek() !='\n' && !this->stream->eof()){
        this->stream->next();
    }
}

/*
 * 跳过多行注释
 */
void Scanner::skipMultiLineComment()
{
    //跳过*，/之前已经跳过去了。
    this->stream->next();

    if (!this->stream->eof()){
        char ch1 = this->stream->next();
        //往后一直找到回车或者eof
        while(!this->stream->eof()){
            char ch2 = this->stream->next();
            if (ch1 == '*' && ch2 == '/'){
                return;
            }
            ch1 = ch2;
        }
    }

    // TODO error report

}

bool Scanner::isDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

bool Scanner::isLetter(char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

bool Scanner::isLetterDigitOrUnderScore(char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
}

Token Scanner::parseStringLiteral()
{
    return {};
}

Token Scanner::parseNumberLiteral()
{
    return {};
}  

Token Scanner::parseIdentifer()
{
    Position pos = stream->get_posistion();
    Token token = {TokenType::_identifier, "", pos, {}};
    
    //第一个字符不用判断，因为在调用者那里已经判断过了
    token.text.push_back(stream->next());
    
    //读入后序字符
    while(!stream->eof() &&
            isLetterDigitOrUnderScore(stream->peek())){
        token.text.push_back(stream->next());
    }

    pos.end = stream->pos + 1;

    //识别出关键字（从字典里查，速度会比较快）
    if (this->keywords.count(token.text)){
        token.type = TokenType::_keyword;
        token.code.type = CodeType::_kw;
        token.code.kw = this->keywords[token.text];
    }

    return token;
}

Token Scanner::getAToken()
{
    skipWhiteSpace();
    Position pos = stream->get_posistion();
    if (stream->eof()) {
        return {TokenType::_eof, "", {}};
    }

    char ch = stream->peek();
    Code code;

    if (this->isLetter(ch) || ch == '_'){
        return this->parseIdentifer();
    }
    else if (ch == '('){
        stream->next();
        code.type = CodeType::_sep;
        code.sep = Seperator::OpenParen;
        return {TokenType::_seperator, "(", pos, code};
    }
    else if (ch == ')'){
        stream->next();
        code.type = CodeType::_sep;
        code.sep = Seperator::CloseParen;
        return {TokenType::_seperator, ")", pos, code};
    }
    else if (ch == '{'){
        stream->next();
        code.type = CodeType::_sep;
        code.sep = Seperator::OpenBrace;
        return {TokenType::_seperator, "{", pos, code};
    }
    else if (ch == '}'){
        stream->next();
        code.type = CodeType::_sep;
        code.sep = Seperator::CloseBrace;
        return {TokenType::_seperator, "}", pos, code};
    }
    else if (ch == '['){
        stream->next();
        code.type = CodeType::_sep;
        code.sep = Seperator::OpenBracket;
        return {TokenType::_seperator, "[", pos, code};
    }
    else if (ch == ']'){
        stream->next();
        code.type = CodeType::_sep;
        code.sep = Seperator::CloseBracket;
        return {TokenType::_seperator, "]", pos, code};
    }
    else if (ch == ':'){
        stream->next();
        code.type = CodeType::_sep;
        code.sep = Seperator::Colon;
        return {TokenType::_seperator, ":", pos, code};
    }
    else if (ch == ';'){
        stream->next();
        code.type = CodeType::_sep;
        code.sep = Seperator::SemiColon;
        return {TokenType::_seperator, ";", pos, code};
    }
    else if (ch == ','){
        stream->next();
        code.type = CodeType::_op;
        code.op = Op::Comma;
        return {TokenType::_seperator, ",", pos, code};
    }
    else if (ch == '?'){
        stream->next();
        code.type = CodeType::_op;
        code.op = Op::QuestionMark;
        return {TokenType::_seperator, "?", pos, code};
    }
    //解析数字字面量，语法是：
    // DecimalLiteral: IntegerLiteral '.' [0-9]* 
    //   | '.' [0-9]+
    //   | IntegerLiteral 
    //   ;
    // IntegerLiteral: '0' | [1-9] [0-9]* ;
    else if (isDigit(ch)){
        stream->next();
        char ch1 = stream->peek();
        string literal;
        if(ch == '0'){//暂不支持八进制、二进制、十六进制
            if (!(ch1>='1' && ch1<='9')){
                literal.push_back('0');
            }
            else {
                //console.log("0 cannot be followed by other digit now, at line: " + stream->line + " col: " + stream->col);
                //暂时先跳过去
                stream->next();
                return getAToken();
            }
        }
        else if(ch >= '1' && ch <= '9'){
            literal.push_back(ch);
            while(isDigit(ch1)){
                ch = stream->next();
                literal.push_back(ch);
                ch1 = stream->peek();
            }
        }
        //加上小数点.
        if (ch1 == '.'){
            //小数字面量
            literal.push_back('.');
            stream->next();
            ch1 = stream->peek();
            while(isDigit(ch1)){
                ch = stream->next();
                literal.push_back(ch);
                ch1 = stream->peek();
            }
            pos.end = stream->pos + 1;
            return {TokenType::_double_literal, literal, pos, {}};
        }
        else{
            //返回一个整型字面量
            return {TokenType::_integer_literal, literal, pos, {}};
        }
    }
    else if (ch == '.'){
        stream->next();
        char ch1 = stream->peek();
        if (isDigit(ch1)){
            //小数字面量
            string literal = ".";
            while(isDigit(ch1)){
                ch = stream->next();
                literal.push_back(ch);
                ch1 = stream->peek();
            }
            pos.end = stream->pos+1;
            return {TokenType::_double_literal, literal, pos, {}};
        }
        //...省略号
        else if (ch1 == '.'){
            stream->next();
            //第三个.
            ch1 = stream->peek();
            if (ch1 == '.'){
                pos.end = stream->pos+1;
                code.type = CodeType::_op;
                code.op = Op::Ellipsis;
                return {TokenType::_seperator, "...", pos, code};
            }
            else{
                //console.log('Unrecognized pattern : .., missed a . ?');
                return getAToken();
            }
        }
        //.号分隔符
        else{
            code.type = CodeType::_op;
            code.op = Op::Dot;
            return {TokenType::_operator, ".", pos, code};
            //return new Token(TokenType::_operator, '.', pos, Op.Dot);
        }
    }
    else if (ch == '/'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '*'){
            skipMultiLineComment();
            return getAToken();
        }
        else if (ch1 == '/'){
            return getAToken();
            skipSingleLineComment();
        }
        else if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::DivideAssign;
            return {TokenType::_operator, "/=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::Divide;
            return {TokenType::_operator, "/", pos, code};
        }
    }
    else if (ch == '+'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '+'){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::Inc;
            return {TokenType::_operator, "++", pos, code};
        }else if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::PlusAssign;
            return {TokenType::_operator, "+=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::Plus;
            return {TokenType::_operator, "+", pos, code};
        }
    }
    else if (ch == '-'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '-'){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::Dec;
            return {TokenType::_operator, "--", pos, code};
        }else if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::MinusAssign;
            return {TokenType::_operator, "-=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::Minus;
            return {TokenType::_operator, "-", pos, code};
        }
    }
    else if (ch == '*'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::MultiplyAssign;
            return {TokenType::_operator, "*=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::Multiply;
            return {TokenType::_operator, "*", pos, code};
        }
    }
    else if (ch == '%'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::ModulusAssign;
            return {TokenType::_operator, "%=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::Modulus;
            return {TokenType::_operator, "%", pos, code};
        }
    }
    else if (ch == '>'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::GE;
            return {TokenType::_operator, ">=", pos, code};
        }
        else if (ch1 == '>'){
            stream->next();
            char ch1 = stream->peek();
            if (ch1 == '>'){
                stream->next();
                ch1 = stream->peek();
                if (ch1 == '='){
                    stream->next();
                    pos.end = stream->pos+1;
                    code.type = CodeType::_op;
                    code.op = Op::RightShiftLogicalAssign;
                    return {TokenType::_operator, ">>>=", pos, code};
                }
                else{
                    pos.end = stream->pos+1;
                    code.type = CodeType::_op;
                    code.op = Op::RightShiftLogical;
                    return {TokenType::_operator, ">>>", pos, code};
                }
            }
            else if (ch1 == '='){
                stream->next();
                pos.end = stream->pos+1;
                code.type = CodeType::_op;
                code.op = Op::LeftShiftArithmeticAssign;
                return {TokenType::_operator, ">>=", pos, code};
            }
            else{
                pos.end = stream->pos+1;
                code.type = CodeType::_op;
                code.op = Op::RightShiftArithmetic;
                return {TokenType::_operator, ">>", pos, code};
            }
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::G;
            return {TokenType::_operator, ">", pos, code};
        }
    }
    else if (ch == '<'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::LE;
            return {TokenType::_operator, "<=", pos, code};
        }
        else if (ch1 == '<'){
            stream->next();
            ch1 = stream->peek();
            if (ch1 == '='){
                stream->next();
                pos.end = stream->pos + 1;
                code.type = CodeType::_op;
                code.op = Op::LeftShiftArithmeticAssign;
                return {TokenType::_operator, "<<=", pos, code};
            }
            else{
                pos.end = stream->pos+1;
                code.type = CodeType::_op;
                code.op = Op::LeftShiftArithmetic;
                return {TokenType::_operator, "<<", pos, code};
            }
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::L;
            return {TokenType::_operator, "<", pos, code};
        }
    }
    else if (ch == '='){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::EQ;
            return {TokenType::_operator, "==", pos, code};
        }
        //箭头=>
        else if (ch1 == '>'){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::ARROW;
            return {TokenType::_operator, "=>", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::Assign;
            return {TokenType::_operator, "=", pos, code};
        }
    }
    else if (ch == '!'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::NE;
            return {TokenType::_operator, "!=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::Not;
            return {TokenType::_operator, "!", pos, code};
        }
    }
    else if (ch == '|'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '|'){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::Or;
            return {TokenType::_operator, "||", pos, code};
        }
        else if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::BitOrAssign;
            return {TokenType::_operator, "|=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::BitOr;
            return {TokenType::_operator, "|", pos, code};
        }
    }
    else if (ch == '&'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '&'){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::And;
            return {TokenType::_operator, "&&", pos, code};
        }
        else if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::BitAndAssign;
            return {TokenType::_operator, "&=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::BitAnd;
            return {TokenType::_operator, "&", pos, code};
        }
    }
    else if (ch == '^'){
        stream->next();
        char ch1 = stream->peek();
        if (ch1 == '='){
            stream->next();
            pos.end = stream->pos+1;
            code.type = CodeType::_op;
            code.op = Op::BitXorAssign;
            return {TokenType::_operator, "^=", pos, code};
        }
        else{
            code.type = CodeType::_op;
            code.op = Op::BitXOr;
            return {TokenType::_operator, "^", pos, code};
        }
    }
    else if (ch == '~'){
        stream->next();
        code.type = CodeType::_op;
        code.op = Op::BitNot;
        return {TokenType::_operator, "~", pos, code};
    }
    else{ 
        //暂时去掉不能识别的字符
        //console.log("Unrecognized pattern meeting ': " +ch+"', at ln:" + stream->line + " col: " + stream->col);
        stream->next();
        return getAToken();
    }
}

