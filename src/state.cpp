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
    this->files = new unordered_map<string, FunctionVal *>;
    this->vm = new VM;
    if (!this->stack) {
        cout << "initial fail!" << endl;
        exit(1);
    }
}

void State::load(const char *start_file_name)
{

}

static State * newState()
{
    State *state = new State(100);
    return state;
}