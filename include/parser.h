#ifndef __PARSER_H__
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

    op_ne,          // !=
    op_equal,       // ==

   
    op_not,         // !
    op_len,         // #
    op_unary_sub,   // 前置 - 负号

    op_gt,          // >
    op_gt_eq,       // >=
    op_lt,          // <
    op_lt_eq,       // <=
    op_or,          // ||
    op_and,         // &&
    op_bin_xor,     // ^
    op_bin_and,     // &
    op_bin_lm,      // <<
    op_bin_rm,      // >> 
    op_bin_lme,     // <<=
    op_bin_rme,     // >>=

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
    const char *name;
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
    vector<BasicValue *> *parameters;         // 参数
    vector<BodyStatment *> *body;       // 函数体
    int from;
    int to;
};

// 数组声明初始化，有三种类型：数字，字符串，id(函数名称，数组引用，表引用)
struct ArrayInit
{
    VariableType type;
    union variable
    {
        Number *number;
        String *string;
        Boolean *boolean;
        IdExpression *id_exp;
        CallExpression *call_exp;
    };
};

// 数组
struct Array
{
    IdExpression *name;
    vector<ArrayInit *> *fields;
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

    VariableType value_type;
    union Value
    {
        Number *number_val;
        Boolean *boolean_val;
        String *string_val;
        IdExpression *id_val;       // 函数、数组或表的引用
        Array *array_val;
        Table *table_val;
        CallExpression *call_val;   // 直接调函数
    };
    Value *v;
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
        };

        assignment_union *assign;

        ~assignment(){
            switch (assignment_type)
            {
            case AssignmenType::operation: 
                delete assign->oper_val;
                break;
            case AssignmenType::basic: 
                delete assign->basic_val;
                break;
            case AssignmenType::call: 
                delete assign->call_val;
                break;
            case AssignmenType::index: 
                delete assign->index_val;
                break;
            default:
                break;
            }
            delete assign;
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
    assign,
    call,
    arr,
    op,
    table,
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
struct IfExpStatement
{
    vector<OperationExpression *> *condition;
    vector<BodyStatment *> *body;
};

struct IfExpression
{
    vector<IfExpStatement *> *statements;  // if {} else if{} else {}, 这里包含以上所有
    int from;
    int to;
};

struct WhileExpression
{
    OperationExpression *condition;
    vector<IfExpStatement *> *statements;
    int from;
    int to;
};

struct DoWhileExpression
{
    vector<IfExpStatement *> *statements;
    OperationExpression *condition;
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
    union first
    {
        vector<IdExpression *> *id_vars;
        vector<AssignmentExpression *> assign_vars;
    };

    first *first_statement;

    vector<Operation *> *second_statement;
    vector<OperationExpression *> *third_statement;

    int from;
    int to;
};

struct SwitchCaseExpression
{
    
};

struct CallExpression
{
    IdExpression *function_name;
    vector<BasicValue *> *parameters;
    int from;
    int to;
};

enum class ReturnType
{
    value,          // [], {}
    call,           // func()
    index,          // a[1], tb["name"]
};

struct ReturnExpression
{
    ReturnType type;
    union statement
    {
        BasicValue *baisc_statement;
        CallExpression *call_statement;
        OperationExpression *oper_statement;
    };

    vector<statement *> *statements;

    int from;
    int to;
};

// 函数体
struct BodyStatment
{
    ExpressionType type;
    union body
    {
        AssignmentExpression *assign_exp;
        IfExpression *if_exp;
        WhileExpression *while_exp;
        DoWhileExpression *do_while_exp;
        ForExpression *for_exp;
        ReturnExpression *return_exp;
        SwitchCaseExpression *swtich_case_exp;
        CallExpression *call_exp;
    };
};

/*********************** do parse **********************/

struct Chuck
{
    vector<BodyStatment *> *statements;
};

// 一次只处理一个文件，遇到依赖其他文件的符号，则暂停去编译它在返回继续
void parse(unordered_map<string, TokenReader *> &files);

#endif
