#ifndef __VM_H__
#define __VM_H__
#include <vector>
#include "instruction.h"

class State;

class VM
{
public:
    void execute(const std::vector<int> &pcs, State *state, int argc, char **argv);

};


#endif