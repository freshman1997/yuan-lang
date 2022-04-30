#ifndef __STATE_H__
#define  _STATE_H__

#include <iostream>
#include <unordered_map>

#include "vm.h"
#include "stack.h"

class Value;
class FunctionVal;

class State
{
public:
    Value * pop();
    void push(Value *val);
    void pushg(int i);
    void pushl(int i);
    void pushu(int i);
    void pushc(int i);
    void pusht(Value *val);
    
    Value * get(int pos);
    VM * get_vm();
    void load(const char *start_file_name);
    State() = delete;
    State(size_t sz);
    FunctionVal * get_by_file_name(const char *);
    void run();

private:
    FunctionVal *cur = NULL;
    unordered_map<string, FunctionVal *> *files = NULL;
    VM *vm = NULL;
    VmStack *stack = NULL;
};

#endif