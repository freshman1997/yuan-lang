﻿#ifndef __PARSER_H__
#define __PARSER_H__

#include <unordered_map>

#include "lex.h"

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

    op_bin_xor,     // ^
    op_bin_and,     // &
    op_bin_lm,      // <<
    op_bin_rm,      // >> 
    op_bin_lme,     // <<=
    op_bin_rme,     // >>=

    op_not,         // !
    op_len,         // #
    op_unary_sub,   // 前置 - 负号
    op_add_add,     // ++
    op_sub_sub,     // --

    op_in,          // in 用在 for 循环
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
};

struct CallExpression;

// 标识符
struct IdExpression
{
    bool is_local;
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

struct BasicValue;

// 函数
struct Function
{
    IdExpression *function_name;                // 函数名称
    bool is_local;                              // 是否为 local 修饰
    vector<IdExpression *> *parameters;         // 参数
    vector<BodyStatment *> *body;               // 函数体
    int from;
    int to;
};

struct Operation;

// 数组
struct Array
{
    IdExpression *name;
    vector<Operation *> *fields;
    int from;
    int to;
};

struct TableInitItem;

// 表
struct Table
{
    IdExpression *name;
    vector<TableInitItem *> *members;
    int from;
    int to;
};

// 表声明初始化，键值对
struct TableInitItem
{
    VariableType key_type;
    union Key
    {
        Number *number_key;
        String *string_key;
    };
    Key *k;
    Operation *v;
};

struct BasicValue
{
    VariableType type;
    union Value{
        IdExpression *id;   // 变量，函数、数组或表的引用
        String *string;
        Number *number;
        Boolean *boolean;
        Array *array;
        Table *table;
        Function *function;
    };
    Value *value;
};

struct IndexExpression
{
    IdExpression *id;
    VariableType key_type;
    union Key
    {
        Number *number_key;
        String *string_key;
    };
    vector<Key *> *keys;
};

struct OperationExpression;

struct AssignmentExpression
{
    IdExpression *id;
    enum class AssignmenType
    {
        id,
        operation,
        index,
        basic,
        call,
    };

    struct assignment {
        AssignmenType assignment_type;
        union assignment_union
        {
            BasicValue *basic_val;
            CallExpression *call_val;   // 函数调用
            OperationExpression *oper_val;
            IndexExpression *index_val;
            IdExpression *id_val;
        };

        assignment_union *ass;

        ~assignment(){
            if (ass) {
                delete ass;
            }
        }
    };

    assignment *assign;

    ~AssignmentExpression() 
    {
       if(assign) delete assign;
    }
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
};

struct OperationExpression;

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
    };
    oper *op;
};

// 表示单个操作符 
struct OperationExpression
{
    OperatorType op_type; // 这个可以确定是一元运算符或二元运算符
    Operation *left;
    Operation *right;
    int from;
    int to;
};

// 单个 if 、else if 、 else 这些表达式
struct IfStatement
{
    OperationExpression *condition;
    vector<BodyStatment *> *body;
    int from;
    int to;
};

struct IfExpression 
{
    vector<IfStatement *> *if_statements; // 多个，if，else if, else
    int from;
    int to;
};

struct WhileExpression
{
    OperationExpression *condition;
    vector<BodyStatment *> *body;
    int from;
    int to;
};

struct DoWhileExpression
{
    OperationExpression *condition;
    vector<BodyStatment *> *body;
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
    vector<AssignmentExpression *> *first_statement;
    OperationExpression *second_statement;          // condition or in 
    vector<OperationExpression *> *third_statement;

    vector<BodyStatment *> *body;

    int from;
    int to;
};

struct SwitchCaseExpression
{
    
};


struct CallExpression
{
    IdExpression *function_name;
    vector<Operation *> *parameters;
    int from;
    int to;
};

struct ReturnExpression
{
    OperationExpression *statement;
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
        SwitchCaseExpression *swtich_case_exp;
        Function *function_exp;
        CallExpression *call_exp;
    };
    body_expression *body;
};

/*********************** do parse **********************/

struct Chunck
{
    vector<BodyStatment *> *statements;
};

// 一次只处理一个文件，遇到依赖其他文件的符号，则暂停去编译它在返回继续
vector<Chunck *> * parse(unordered_map<string, TokenReader *> &files);

#endif
