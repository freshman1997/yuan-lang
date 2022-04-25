#ifndef __STATE_H__
#define  _STATE_H__

#include <iostream>

#include "vm.h"
#include "stack.h"
#include "types.h"

class State
{
public:
    Value * pop();
    void push(Value *val);
    VM * get_vm();
    State() = delete;
    State(size_t sz);

private:
    
    VM *vm = NULL;
    VmStack *stack = NULL;
};

static State * newState();

#endif