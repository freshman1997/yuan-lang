#ifndef __CODE_WRITER_H__
#define __CODE_WRITER_H__
#include <vector>

#include "codegen.h"
#include "instruction.h"

using namespace std;

class CodeWriter
{
public:
    void set_file_name(const char *file_name)
    {
        this->file_name = file_name;
        pc = 0;
    }
    const char * get_file_name()
    {
        return file_name;
    }

    void add(OpCode code)
    {
        Instruction *ins = new Instruction;
        instructions->push_back(ins);
    }

    int get_pc()
    {
        return pc;
    }

private:
    vector<Instruction *> *instructions = new vector<Instruction *>;
    const char *file_name = NULL;
    int pc = 0;
};


#endif