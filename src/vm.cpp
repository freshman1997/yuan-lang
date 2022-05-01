
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
#define mod(v1, v2) ((static_cast<Number *>(v1))->value() % (static_cast<Number *>(v2))->value())

static State *state = NULL;

static void panic(const char *reason) 
{
    cout << "vm was crashed by: " << reason << endl;
    exit(0);
}

static void assign(int type, int i)
{
    FunctionVal *cur = state->get_cur();
    Value *val = state->pop();
    if (type == 0) {
        // 全局
        FunctionVal *global = cur;
        while (global->pre) {
            global = global->pre;
        }
        vector<Value *> *gVars = global->chunk->global_vars;
        if (gVars->size() <= i) {
            panic("fatal error");
        }
        if (gVars->at(i) && gVars->at(i)->ref_count == 1) delete gVars->at(i);
        gVars->at(i) = val;
        
    }
    
    // 从局部变量表里面拿，然后赋值

}

static void operate(int type, int op) // type 用于区分是一元还是二元
{
    static vector<int> opers;
    if (opers.empty()) {
        // + - * / % && || < > <= >= == != >> << | & ! ~ 
        for(int i = 0; i < 20; ++i) {
            opers.push_back(i);
        }
    }
    Value *val1 = state->pop();
    Value *val2 = state->pop();
    // 入栈是第一个先入，出栈是第二个先出
    if (isnum(val1, val2)) {
        Number *ret = new Number;
        switch (op)
        {
        case 0: ret->set_val(add(val2, val1)); break;
        case 1: ret->set_val(sub(val2, val1)); break;
        case 2: ret->set_val(mul(val2, val1)); break;
        case 3: ret->set_val(div(val2, val1)); break;
        default:
            break;
        }
        state->push(ret);
        cout << "ret: " << ret->value() << endl;
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

static void call_function(int type, int param)
{

}

static void string_concat()
{
    Value *val1 = state->pop();
    Value *val2 = state->pop();
    // 右边的链接到左边
    
}

static void substring(int from, int to)
{
    Value *val = state->pop();
    if (val->get_type() != ValueType::t_string) {
        panic("not a string variable");
    }
    string *str = static_cast<String *>(val)->value();
    
}



static void do_execute(const std::vector<int> &pcs, int from, int to)
{
    for (int i = from; i < to; ++i) {
        int it = pcs[i];
        int param = (it >> 8);
        OpCode op = (OpCode)((it << 24) >> 24);
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

       
        case OpCode::op_pushc:{

            state->pushc(param);
            break;
        }
        case OpCode::op_pushl:{

            break;
        }

        
        case OpCode::op_storeg:
            assign(0, 0);
            break;
        case OpCode::op_storel:
            assign(0, 1);
            break;
        case OpCode::op_storeu:
            assign(0, 2);
            break;
        
        default:
            cout << "unsupport operation." << endl;
            exit(0);
            break;
        }
    }
}

void VM::execute(const std::vector<int> &pcs, State *_state, int argc, char **argv)
{
    state = _state;
    do_execute(pcs, 0, pcs.size());
}
