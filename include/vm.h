#ifndef __VM_H__
#define __VM_H__
#include <vector>
#include "instruction.h"

class State;
class TableVal;

class VM
{
public:
    void execute(const std::vector<int> &pcs, State *state, int argc, char **argv);

    void load_lib(TableVal *tb);
};




#endif