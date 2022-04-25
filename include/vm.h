#ifndef __VM_H__
#define __VM_H__
#include <vector>

#include "instruction.h"

class VM
{
    
};

class State;

// 指令该怎么保存起来？
void execute(const std::vector<Instruction *> &pcs, State *state, int argc, char **argv);


#endif