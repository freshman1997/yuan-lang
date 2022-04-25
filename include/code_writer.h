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

    void add(OpCode op, int param)
    {
        // 8 位指令，24位操作数
        int code = int(op) << 24 | param;
        instructions.push_back(param);
    }

    int get_pc()
    {
        return pc;
    }

private:
    vector<int> instructions;
    const char *file_name = NULL;
    int pc = 0;
};


#endif