#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

enum InstructionParamType
{
    global_param,
    local_param,
    upvalu_param
};


struct InstructionParam
{
    InstructionParamType type;
    int index = -1;    
};

enum class OpCode
{
    op_pushg,           // 入栈全局变量 参数是第几个且是当前文件的
    op_pushl,           // 入栈局部变量，当前函数的
    op_pushu,           // 入栈，当前函数的 upvalue
    op_pushc,           // 入栈常量
    op_pop,             // 出栈
    op_storeg,          // 存储 全局变量
    op_storel,          // 存储 局部变量
    op_storeu,          // 存储 upvalue

    /* 运算符部分 */
    /* 二元运算符 */
    op_add,             // +
    op_sub,             // -
    op_mul,             // *
    op_div,             // /
    op_mod,             // %
    op_equal,           // ==
    op_not_equal,       // !=
    op_gt,              // >
    op_lt,              // <
    op_gt_eq,           // >=
    op_lt_eq,           // <=
    op_or,              // ||
    op_and,             // &&
    op_add_eq,          // +=
    op_sub_eq,          // -=
    op_mul_eq,          // *=
    op_div_eq,          // /=
    op_mod_eq,          // %= 

    op_dot,             // .
    op_concat,          // ..


    /* 一元运算符 */
    op_not,             // !
    op_len,             // # 
    op_unary_sub,       // 前置 - 负号
    op_add_add,         // ++
    op_sub_sub,         // --


    /* 逻辑运算符 */
    op_bin_or,          // |
    op_bin_xor,         // ^
    op_bin_and,         // &
    op_bin_not,         // ~
    op_bin_lm,          // <<
    op_bin_rm,          // >> 
    op_bin_xor_eq,      // ^=
    op_bin_and_eq,      // &=
    op_bin_or_eq,       // |=
    op_bin_lme,         // <<=
    op_bin_rme,         // >>=

    /* jump */
    op_break,           // break
    op_continue,        // continue
    op_jump,            // 跳

    /* 操作表达式 */
    op_for_normal,      // 普通 for
    op_for_in,          // for in
    op_do_while,        // do while
    op_while,           // while
    op_call,            // call
    op_if,              // if
    op_return,          // return

    op_enter_block,     // {
    op_leave_block,     // }

    op_set_self,        // 如果是模块，需要self


    op_table_new,       // a = {}
    op_table_get,       // b = a["name"]
    op_table_del,       // a["name"] = nil
    op_table_set,       // a["name"] = "tomcat"

    op_array_new,       // a = []
    op_array_get,       // a[0]
    op_array_set,       // a[1] = 100
    op_array_del,       // a = nil

    op_load_bool,       // ?
    op_load_nil,        // ?
};

struct Instruction
{
    OpCode op;
};

#endif