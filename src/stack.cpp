#include "stack.h"
#include "types.h"

VmStack::VmStack(size_t initSize)
{
    this->_stack.reserve(initSize);
}

void VmStack::push(Value *val)
{
    this->_stack.push_back(val);
    val->ref_count++;
}

Value * VmStack::pop()
{
    if (this->_stack.empty()) return new NilVal;
    Value *val = this->_stack.back();
    this->_stack.pop_back();
    val->ref_count--;
    return val;
}

Value * VmStack::get(int i)
{
    if (i < 0 || this->_stack.size() <= i) return new NilVal;
    return this->_stack[i];
}

int VmStack::get_size()
{
    return this->_stack.size();
}

VmStack::~VmStack()
{
    for (auto &it : _stack) {
        if (it) {
            delete it;
        }
    }
}
