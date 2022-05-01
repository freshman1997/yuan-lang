#include "stack.h"
#include "types.h"

VmStack::VmStack(size_t initSize)
{
    this->_stack.reserve(initSize);
}

void VmStack::push(Value *val)
{
    this->_stack.push_back(val);
}

Value * VmStack::pop()
{
    if (this->_stack.empty()) return NULL;
    Value *val = this->_stack.back();
    this->_stack.pop_back();
    val->ref_count--;
    return val;
}

Value * VmStack::get(int i)
{
    if (this->_stack.size() < i) return NULL;
    return this->_stack[i];
}