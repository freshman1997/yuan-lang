#ifndef __STACK_H__
#define __STACK_H__
#include <vector>
#include "types.h"

using namespace std;

class VmStack
{
public:
    VmStack(size_t initSize);
    void push(Value *val);
    Value * pop();
    Value * get(int i);

private:
    vector<Value *> _stack;
};

#endif