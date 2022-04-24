#ifndef __VM_H__
#define __VM_H__
#include <vector>

#include "instruction.h"
#include "types.h"



// 指令该怎么保存起来？
void execute(std::vector<Instruction *> pcs, int argc, char **argv);


#endif