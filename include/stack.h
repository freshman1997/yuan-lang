#ifndef __STACK_H__
#define __STACK_H__
#include <vector>

using namespace std;

class Value;

class VmStack
{
public:
    VmStack(size_t initSize);
    ~VmStack();
    void push(Value *val);
    Value * pop();
    Value * get(int i);
    int get_size();
    void rm(int i);

private:
    vector<Value *> _stack;
};

#endif