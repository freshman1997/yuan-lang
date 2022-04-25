#include "state.h"

Value * State::pop()
{

}

void State::push(Value *val)
{

}

VM * State::get_vm()
{

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