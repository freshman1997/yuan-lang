#include "state.h"

Value * State::pop()
{
    return this->stack->pop();
}

void State::push(Value *val)
{
     this->stack->push(val);
}

VM * State::get_vm()
{
    return this->vm;
}

State::State(size_t sz)
{
    this->stack = new VmStack(sz);
    if (!this->stack) {
        cout << "initial fail!" << endl;
        exit(1);
    }
}

static State * newState()
{
    State *state = new State(100);
    return state;
}