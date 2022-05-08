#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

enum class OpCode
{
    /* 运算符部分 */
    /* 二元运算符 */
    op_add,             // +
    op_sub,             // -
    op_mul,             // *
    op_div,             // /
    op_mod,             // %

    /* 逻辑运算符 */
    op_bin_or,          // |
    op_bin_xor,         // ^
    op_bin_and,         // &
    op_bin_not,         // ~
    op_bin_lm,          // <<
    op_bin_rm,          // >> 

    /* 比较，结果会入栈供 test 指令使用 */
    op_equal,           // ==
    op_not_equal,       // !=
    op_lt,              // <
    op_gt,              // >
    op_gt_eq,           // >=
    op_lt_eq,           // <=
    op_or,              // ||
    op_and,             // &&

    /* 一元运算符 */
    op_not,             // !
    op_len,             // # 
    op_unary_sub,       // 前置 - 负号
    op_add_add,         // ++
    op_sub_sub,         // --

    op_test,            // 

    op_concat,          // ..  参数二并到参数一上，二者必须为字符串

    op_pushg,           // 入栈全局变量 参数是第几个且是当前文件的
    op_pushl,           // 入栈局部变量，当前函数的
    op_pushu,           // 入栈，当前函数的 upvalue
    op_pushc,           // 入栈常量

    op_pop,             // 出栈       参数为次数，出栈后需要释放内存
    op_storeg,          // 存储 全局变量
    op_storel,          // 存储 局部变量
    op_storeu,          // 存储 upvalue

    /* jump */
    op_jump,            // 跳

    /* 操作表达式 */
    op_for_in,          // for in

    /* 函数调用部分 */
    op_call,            // call
    op_call_upv,        // call upvalue
    op_call_t,          // call temp val []()
    op_param_start,     // 函数调用参数开始时栈的位置
    op_return,          // return 参数为返回的个数，后续再pusht结果进栈

    op_get_env,         // 查找内置表、变量、函数

    op_enter_func,      // {

    op_set_self,        // 如果是模块，需要self

    op_table_new,       // a = {}
    op_table_set,       // a["name"] = "tomcat"

    op_array_new,       // a = []
    op_array_set,       // a[1] = 100,      // 这个可以设置多个键值对，用于

    op_index,           // a["name"], arr[1] a.b

    op_load_bool,
    op_load_nil,
    
};

#endif