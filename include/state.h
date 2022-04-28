#ifndef __STATE_H__
#define  _STATE_H__

#include <iostream>
#include <unordered_map>

#include "vm.h"
#include "stack.h"
#include "types.h"

class State
{
public:
    Value * pop();
    void push(Value *val);
    Value * get(int pos);
    VM * get_vm();
    void load(const char *start_file_name);
    State() = delete;
    State(size_t sz);

private:
    unordered_map<string, FunctionVal *> *files = NULL;
    VM *vm = NULL;
    VmStack *stack = NULL;
};

static State * newState();

#endif