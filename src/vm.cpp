
#include "vm.h"
#include "stack.h"
#include "state.h"

/*
    每个作用域都需要包含一个表，用来记录变量，这样子就不用记录变量的名字了，运行再根据取得的值计算
*/
#define isnum(a, b) (a->get_type == ValueType::t_number && b->get_type == ValueType::t_number)

#define add(a, b) (a->value() + b->value())


static State *state = NULL;

static void assign(int type)
{
    Value *val1 = state->pop();
    Value *val2 = state->pop();
    // 从局部变量表里面拿，然后赋值

}
static vector<int> opers;

static void operate(int type, int op) // type 用于区分是一元还是二元
{
    if (opers.empty()) {
        // + - * / % && || < > <= >= == != >> << | & ! ~ 
        for(int i = 0; i < 19; ++i) {
            opers.push_back(i);
        }
    }
    Value *val1 = state->pop();
    Value *val2 = state->pop();
    if (isnum(val1, val2)) {
        Number *ret = new Number;
        switch ()
        {
        case 0: ret->set_val(add(val1, val2)); break;
        default:
            break;
        }
        state->push(ret);
    }
}

void execute(std::vector<Instruction *> pcs, State *_state, int argc, char **argv)
{
    state = _state;
    for(auto &it : pcs) {
        switch (it->op)
        {
        case OpCode::op_storel:
            assign(0);
            break;
        case OpCode::op_add:
        {
            operate(0, 0);
            break;
        }
        default:
            break;
        }
    }
}
