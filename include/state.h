#ifndef __STATE_H__
#define  _STATE_H__

#include <iostream>
#include <unordered_map>

#include "vm.h"
#include "stack.h"

class Value;
class FunctionVal;
class ArrayVal;

class State
{
public:
    int param_start = 0;

    Value * pop();
    void push(Value *val);
    void pushg(int i);
    void pushl(int i);
    void pushu(int i);
    void pushc(int i);

    Value * getc(int i);
    Value * getg(int i);
    Value * getl(int i);
    Value * getu(int i);

    Value * get_subfun(int i);
    
    Value * get(int pos);
    VM * get_vm();
    State() = delete;
    State(size_t sz);
    FunctionVal * get_by_file_name(const char *);
    FunctionVal * get_cur();
    void set_cur(FunctionVal *fun);
    void end_call();
    
    int get_stack_size();
    int cur_calls();
    void clearTempData();
    bool tryClearOpenedFuns(FunctionVal *fun);

    
    FunctionVal * run(const char *, ArrayVal *args);
    bool require(const char *file, ArrayVal *args);

private:
    void load(const char *start_file_name);

    vector<FunctionVal *> *openedFuns = NULL;
    FunctionVal *cur = NULL;
    vector<FunctionVal *> *calls = NULL;
    unordered_map<string, FunctionVal *> *files = NULL;
    VM *vm = NULL;
    VmStack *stack = NULL;
};

#endif