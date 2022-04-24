
#include "vm.h"
#include "state.h"
#include "stack.h"

/*
    每个作用域都需要包含一个表，用来记录变量，这样子就不用记录变量的名字了，运行再根据取得的值计算

*/

void execute(std::vector<Instruction *> pcs, int argc, char **argv)
{

}
