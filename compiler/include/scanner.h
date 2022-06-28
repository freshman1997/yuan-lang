#ifndef __SCANNER_H__
#define __SCANNER_H__

#include <string>
#include <vector>
#include <unordered_map>

using std::string;
using std::vector;
using std::unordered_map;


struct Position
{
    int begin = 0;
    int end = 0;
    int line = 0;
    int column = 0;
};

enum class TokenType
{
    _integer_literal = 0,           // 整形字面量
    _double_literal,                // 浮点字面量
    _string_literal,                // 字符串字面量
    _identifier,                    // 标识符，定义的变量等
    _keyword,                       // 关键字
    _operator,                      // 操作符 
    _seperator,                     // 符号 [] () {} ; :
    _eof,
};

enum class KeyWord
{
    _int8_ = 0,
    _uint8_,
    _int16_,
    _uint16_,
    _int32_, 
    _uint32_,
    _int64_,
    _uint64_,
    _float32_,
    _float64_,
    _void_,
    _let_,
    _return_,
    _class_,
    _extends_,
    _implements_,
    _interface_,
    _static_,
    _goto_,
    _for_,
    _switch_,
    _case_,
    _default_,
    _do_,
    _while_,
    _enum_,
    _if_,
    _else_,
    _break_,
    _continue_,
    _true_,
    _false_,
    _null_,
    _new_,
    _try_,
    _catch_,
    _throw_,
    _finally_,
    _export_,
    _import_,
    _from_,
    _public_,
    _protected_,
    _private_,
    _this_,
    _super_,
    _instanceof_,
};

enum class Seperator
{
    OpenBracket = 0,                //[
    CloseBracket,                   //]
    OpenParen,                      //(
    CloseParen,                     //)
    OpenBrace,                      //{
    CloseBrace,                     //}
    Colon,                          //:
    SemiColon,                      //;
};

enum class Op
{
    QuestionMark = 100,             //?   让几个类型的code取值不重叠
    Ellipsis,                       //...
    Dot,                            //.
    Comma,                          //,
    At,                             //@
    
    RightShiftArithmetic,           //>>
    LeftShiftArithmetic,            //<<
    RightShiftLogical,              //>>>
    IdentityEquals,                 //===
    IdentityNotEquals,              //!==

    BitNot,                         //~
    BitAnd,                         //&
    BitXOr,                         //^
    BitOr,                          //|

    Not,                            //!   
    And,                            //&&
    Or,                             //||

    Assign,                         //=
    MultiplyAssign,                 //*=
    DivideAssign,                   ///=
    ModulusAssign,                  //%=
    PlusAssign,                     //+=
    MinusAssign,                    //-=
    LeftShiftArithmeticAssign,      //<<=
    RightShiftArithmeticAssign,     //>>=
    RightShiftLogicalAssign,        //>>>=
    BitAndAssign,                   //&=
    BitXorAssign,                   //^=
    BitOrAssign,                    //|=
    
    ARROW,                          //=>

    Inc,                            //++
    Dec,                            //--

    Plus,                           //+
    Minus,                          //-
    Multiply,                       //*
    Divide,                         ///
    Modulus,                        //%
    
    EQ,                             //==
    NE,                             //!=
    G,                              //>
    GE,                             //>=
    L,                              //<
    LE,                             //<=
};

// op utils
bool isAssignOp(Op op);
bool isRelationOp(Op op);
bool isArithmeticOp(Op op);
bool isLogicalOp(Op op);

enum class CodeType
{
    _op,
    _sep,
    _kw,
    _null,
};

typedef union 
{
    CodeType type;
    Op op;
    Seperator sep;
    KeyWord kw;
    char null;
} Code;

struct Token
{
    TokenType type;
    string text;
    Position pos;
    Code code;
};

class CharStream
{
public:
    char peek();
    char next();
    bool eof();
    Position get_posistion();
    CharStream(const char *_data);
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
    Token getAToken();
    void skipWhiteSpace();
    void skipSingleLineComment();
    void skipMultiLineComment();

    bool isDigit(char ch);
    bool isLetter(char ch);
    bool isLetterDigitOrUnderScore(char ch);
    

    Token parseStringLiteral();
    Token parseNumberLiteral();
    Token parseIdentifer();

private:
    CharStream *stream = NULL;
    Position lastPos;

    // 用于支持预读多个 Token 的
    vector<Token> peekTokens;

    unordered_map<string, KeyWord> keywords = {
        {"int8", KeyWord::_int8_},
        {"uint8", KeyWord::_uint8_},
        {"int16", KeyWord::_int16_},
        {"uint16", KeyWord::_uint16_},
        {"int32", KeyWord::_int32_},
        {"uint32", KeyWord::_uint32_},
        {"int64", KeyWord::_int64_},
        {"uint64", KeyWord::_uint64_},
        {"float32", KeyWord::_float32_},
        {"float64", KeyWord::_float64_},
        {"return", KeyWord::_return_},
    };
};

#endif