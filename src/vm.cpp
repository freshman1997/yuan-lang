
#include "vm.h"
#include "stack.h"
#include "state.h"
#include "types.h"

/*
    每个作用域都需要包含一个表，用来记录变量，这样子就不用记录变量的名字了，运行再根据取得的值计算
*/
#define isnum(a, b) (a->get_type() == ValueType::t_number && b->get_type() == ValueType::t_number)
#define add(v1, v2) ((static_cast<Number *>(v1))->value() + (static_cast<Number *>(v2))->value())
#define sub(v1, v2) ((static_cast<Number *>(v1))->value() - (static_cast<Number *>(v2))->value())
#define mul(v1, v2) ((static_cast<Number *>(v1))->value() * (static_cast<Number *>(v2))->value())
#define div(v1, v2) ((static_cast<Number *>(v1))->value() / (static_cast<Number *>(v2))->value())

static State *state = NULL;

static void panic(const char *reason) 
{
    cout << "vm was crashed by: " << reason << endl;
    exit(0);
}

static void assign(int type)
{
    Value *val = state->pop();
    // 从局部变量表里面拿，然后赋值

}
static vector<int> opers;

static void operate(int type, int op) // type 用于区分是一元还是二元
{
    if (opers.empty()) {
        // + - * / % && || < > <= >= == != >> << | & ! ~ 
        for(int i = 0; i < 20; ++i) {
            opers.push_back(i);
        }
    }
    Value *val1 = state->pop();
    Value *val2 = state->pop();
    if (isnum(val1, val2)) {
        Number *ret = new Number;
        switch (op)
        {
        case 0: ret->set_val(add(val1, val2)); break;
        case 1: ret->set_val(sub(val1, val2)); break;
        case 2: ret->set_val(mul(val1, val2)); break;
        case 3: ret->set_val(div(val1, val2)); break;
        default:
            break;
        }
        state->push(ret);
        // free 
    }
    else {
        panic("can not do this operation!");
    }
}

static void open_func()
{
    // 初始化 局部变量表和 upvalue 表 ref + 1
    // 保存现场
    
}

static void close_func()
{

}

static void string_concat()
{

}

static void substring(int from, int to)
{

}



static void do_execute(const std::vector<int> &pcs, int from, int to)
{
    for (int i = from; i < to; ++i) {
        int it = pcs[i];
        int param = (it << 8) >> 8;
        OpCode op = (OpCode)(it >> 24);
        switch (op)
        {
        case OpCode::op_add: {
            operate(0, int(op));
            break;
        }
        case OpCode::op_sub: {
            operate(0, int(op));
            break;
        }
        case OpCode::op_mul: {
            operate(0, int(op));
            break;
        }
        case OpCode::op_div: {
            operate(0, int(op));
            break;
        }
        case OpCode::op_mod: {
            operate(0, int(op));
            break;
        }
        case OpCode::op_add_eq: {
            operate(0, int(op));
            break;
        }
        case OpCode::op_sub_eq: {
            operate(0, int(op));
            break;
        }
        case OpCode::op_mul_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_div_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_mod_eq: {
            operate(0, 0);
            break;
        }
        
        /* 逻辑运算符 */
        case OpCode::op_bin_or: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_xor: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_and: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_not: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_lm: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_rm: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_xor_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_and_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_or_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_lme: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_rme: {
            operate(0, 0);
            break;
        }
        
        case OpCode::op_equal: {
            operate(0, 0);
            break;
        }
        case OpCode::op_not_equal: {
            operate(0, 0);
            break;
        }
        case OpCode::op_gt: {
            operate(0, 0);
            break;
        }
        case OpCode::op_lt: {
            operate(0, 0);
            break;
        }
        case OpCode::op_gt_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_lt_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_or: {
            operate(0, 0);
            break;
        }
        case OpCode::op_and: {
            operate(0, 0);
            break;
        }

        /* 一元运算符  从栈中弹出一个 */
        case OpCode::op_not: {
            operate(1, 0);
            break;
        }
        case OpCode::op_len: {
            operate(1, 0);
            break;
        }
        case OpCode::op_unary_sub: {
            operate(1, 0);
            break;
        }
        case OpCode::op_add_add: {
            operate(1, 0);
            break;
        }
        case OpCode::op_sub_sub: {
            operate(1, 0);
            break;
        }


        case OpCode::op_concat: {
            operate(1, 0);
            break;
        }
        

        case OpCode::op_test: {
            operate(1, 0);
            break;
        }

        case OpCode::op_storel:
            assign(0);
            break;
        case OpCode::op_pushl:{

            break;
        }
        default:
            break;
        }
    }
}

void VM::execute(const std::vector<int> &pcs, State *_state, int argc, char **argv)
{
    state = _state;
    do_execute(pcs, 0, pcs.size());
}
