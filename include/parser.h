#ifndef __PARSER_H__
#define __PARSER_H__

#include <unordered_map>

#include "lex.h"

struct Chunck;
// 一次只处理一个文件，遇到依赖其他文件的符号，则暂停去编译它在返回继续
unordered_map<string, Chunck *> * parse(unordered_map<string, TokenReader *> &files);


/*
    目前剩余：
        str[2:], str[:-1], str[2:3] 字符串截取，str .. str1 字符串拼接
        str[1], tb["name"], 下标读取
        tb = {100: "aaa"} 表的构造
        逻辑运算符
        item:add  这种函数声明 module
        require 未增加
        switch case
        break default
*/

/*********************** 变量类型和表达式结构定义 **********************/

enum class OperatorType
{
    op_add,         // + 
    op_sub,         // -
    op_mul,         // *
    op_div,         // /
    op_mod,         // %

    op_concat,      // ..

    op_equal,       // ==
    op_ne,          // !=
   
    op_lt,          // <
    op_gt,          // >
    op_lt_eq,       // <=
    op_gt_eq,       // >=
    op_or,          // ||
    op_and,         // &&

    op_dot,         // .

    op_add_eq,      // +=
    op_sub_eq,      // -=
    op_mul_eq,      // *=
    op_div_eq,      // /=
    op_mod_eq,      // %= 
    
    op_bin_or,      // |
    op_bin_xor,     // ^
    op_bin_and,     // &
    op_bin_not,      // ~
    op_bin_lm,      // <<
    op_bin_rm,      // >> 
    op_bin_xor_eq,  // ^=
    op_bin_and_eq,  // &=
    op_bin_or_eq,   // |=
    op_bin_lme,     // <<=
    op_bin_rme,     // >>=

    op_not,         // !
    op_len,         // #
    op_unary_sub,   // 前置 - 负号
    op_add_add,     // ++
    op_sub_sub,     // --

    op_in,          // in 用在 for 循环

    op_substr,      // : 

    op_none,        
};



// 变量初始化类型
enum class VariableType
{
    t_number,         // 数字
    t_string,         // 字符串
    t_func,           // 函数
    t_array,          // 数组
    t_table,          // 表
    t_boolean,        // 布尔
    t_nil,            // nil
    t_module,         // 模块
};

// 文件中的表达式类型
enum class ExpressionType
{
    assignment_statement,                   // 赋值
    function_declaration_statement,         // 函数定义
    return_statement,                       // 返回，在需要返回模块的时候使用
    if_statement,                           // if 语句表达式
    while_statement,                        // while 语句表达式
    do_while_statement,                     // do while 语句表达式
    for_statement,                          // for 语句表达式
    call_statement,                         // 函数调用表达式
    break_statement,                        // break
    continue_statement,                     // continue
    oper_statement,                         // ++a --a 自增自减
    block_statement,                        // {} 块作用域
};

struct CallExpression;

// 标识符
struct IdExpression
{
    bool is_local = false;
    char *name;
    int name_len;
    int from;
    int to;
};

// 数字
struct Number
{
    double val;     // 值
    char *raw;      // 原始字符串开始指针
    int raw_len;    // 原始字符串长度
    int from;
    int to;
};

struct Boolean
{
    bool val;
    char *raw;      // 原始字符串开始指针
    int raw_len;    // 原始字符串长度
    int from;
    int to;
};

// 字符串
struct String
{
    char *raw;
    int raw_len;
    int from;
    int to;
};

struct BodyStatment;

// 函数
struct Function
{
    IdExpression *function_name = NULL;                // 函数名称
    bool is_local = false;                              // 是否为 local 修饰
    vector<IdExpression *> *parameters = NULL;         // 参数
    vector<BodyStatment *> *body = NULL;               // 函数体
    IdExpression *module = NULL;                       // 所属模块
    int from;
    int to;
};

struct Operation;

// 数组
struct Array
{
    vector<Operation *> *fields = NULL;
    int from;
    int to;
};

struct OperationExpression;

// 表声明初始化，键值对
struct TableItemPair
{
    OperationExpression *k = NULL;
    OperationExpression *v = NULL;
};

// 表
struct Table
{
    vector<TableItemPair *> *members = NULL;
    int from;
    int to;
};

struct IndexExpression
{
    IdExpression *id = NULL;
    vector<OperationExpression *> *keys = NULL;
};

struct AssignmentExpression
{
    IdExpression *id = NULL;
    IdExpression *module = NULL;
    OperationExpression *assign = NULL;
    
    int from;
    int to;
};

enum class OpType
{
    id,
    num,
    str,
    arr,
    table,
    boolean,
    call,
    op,
    index,
    assign,
    nil,
    substr,
    function_declear,
};

struct Operation
{
    OpType type;
    union oper
    {
        IdExpression *id_oper;
        AssignmentExpression *assgnment_oper;
        String *string_oper;
        Number *number_oper;
        Array *array_oper;
        Table *table_oper;
        Boolean *boolean_oper;
        IndexExpression *index_oper;
        CallExpression *call_oper;
        OperationExpression *op_oper;
        Function *fun_oper;         // 这个只在表声明中包含的声明函数使用
    };
    oper *op = NULL;
};

// 表示单个操作符 , 删除时需要手动
struct OperationExpression
{
    OperatorType op_type; // 这个可以确定是一元运算符或二元运算符
    Operation *left = NULL;
    Operation *right = NULL;
    int from;
    int to;
};

// 单个 if 、else if 、 else 这些表达式
struct IfStatement
{
    OperationExpression *condition = NULL;
    vector<BodyStatment *> *body = NULL;
    int from;
    int to;
};

struct IfExpression 
{
    vector<IfStatement *> *if_statements = NULL; // 多个，if，else if, else
    int from;
    int to;
};

struct WhileExpression
{
    OperationExpression *condition = NULL;
    vector<BodyStatment *> *body = NULL;
    int from;
    int to;
};

struct DoWhileExpression
{
    OperationExpression *condition = NULL;
    vector<BodyStatment *> *body = NULL;
    int from;
    int to;
};

enum class ForExpType 
{
    for_in,         // for (k, v in xxx)
    for_normal,     // for (a = x, b = 1, c = 2...; condition; exp)
};

struct ForExpression
{
    ForExpType type;
    vector<OperationExpression *> *first_statement = NULL;
    OperationExpression *second_statement = NULL;          // condition or in 
    vector<OperationExpression *> *third_statement = NULL;

    vector<BodyStatment *> *body = NULL;

    int from;
    int to;
};

struct CallExpression
{
    IdExpression *function_name = NULL;
    vector<Operation *> *parameters = NULL;
    int from;
    int to;
};

struct ReturnExpression
{
    OperationExpression *statement = NULL;
    int from;
    int to;
};

// 函数体
struct BodyStatment
{
    ExpressionType type;
    union body_expression
    {
        AssignmentExpression *assign_exp;
        IfExpression *if_exp;
        WhileExpression *while_exp;
        DoWhileExpression *do_while_exp;
        ForExpression *for_exp;
        ReturnExpression *return_exp;
        Function *function_exp;
        CallExpression *call_exp;
        OperationExpression *oper_exp;
        vector<BodyStatment *> *block_exp;
    };
    body_expression *body = NULL;
};

/*********************** do parse **********************/

struct Chunck
{
    vector<BodyStatment *> *statements = NULL;
    TokenReader *reader = NULL;
};

#endif
