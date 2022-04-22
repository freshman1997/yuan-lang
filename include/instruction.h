#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__



struct InstructionParam
{

};

enum class ProgramCode
{
    pc_push,
    pc_pop,

    /* 运算符部分*/
    /* 二元运算符 */
    pc_add,             // +
    pc_sub,             // -
    pc_mul,             // *
    pc_div,             // /
    pc_mod,             // %
    pc_equal,           // ==
    pc_not_equal,       // !=
    pc_gt,              // >
    pc_lt,              // <
    pc_gt_eq,           // >=
    pc_lt_eq,           // <=
    pc_or,              // ||
    pc_and,             // &&
    pc_add_eq,          // +=
    pc_sub_eq,          // -=
    pc_mul_eq,          // *=
    pc_div_eq,          // /=
    pc_mod_eq,          // %= 

    pc_dot,             // .
    pc_concat,          // ..


    /* 一元运算符 */
    pc_not,             // !
    pc_len,             // # 
    pc_unary_sub,       // 前置 - 负号
    pc_add_add,         // ++
    pc_sub_sub,         // --


    /* 逻辑运算符 */
    pc_bin_or,          // |
    pc_bin_xor,         // ^
    pc_bin_and,         // &
    pc_bin_not,         // ~
    pc_bin_lm,          // <<
    pc_bin_rm,          // >> 
    pc_bin_xor_eq,      // ^=
    pc_bin_and_eq,      // &=
    pc_bin_or_eq,       // |=
    pc_bin_lme,         // <<=
    pc_bin_rme,         // >>=

    /* jump */
    pc_break,           // break
    pc_continue,        // continue
    pc_jump,            // 跳

    /* 操作表达式 */
    pc_assign,          // 赋值
    pc_for_normal,      // 普通 for
    pc_for_in,          // for in
    pc_do_while,        // do while
    pc_while,           // while
    pc_call,            // call
    pc_if,              // if
    pc_return,          // return

    pc_enter_block,     // {
    pc_leave_block,     // }

    pc_set_self,        // 如果是模块，需要self

    pc_load,            // 加载值

    pc_table_new,       // a = {}
    pc_table_get,       // b = a["name"]
    pc_table_del,       // a["name"] = nil
    pc_table_set,       // a["name"] = "tomcat"

    pc_array_new,       // a = []
    pc_array_get,       // a[0]
    pc_array_set,       // a[1] = 100
    pc_array_del,       // a = nil

    pc_load_bool,       // ?
    pc_load_nil,        // ?
};

struct Instruction
{
    ProgramCode pc;
    InstructionParam *param;


};

#endif