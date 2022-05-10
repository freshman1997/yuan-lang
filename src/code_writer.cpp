#include "code_writer.h"

void CodeWriter::set_file_name(const char *file_name)
{
    this->file_name = file_name;
    pc = 0;
    instructions.clear();
}

const char * CodeWriter::get_file_name()
{
    return file_name;
}

void CodeWriter::add(OpCode op, int param)
{
    // 8 位指令，24位操作数
    int code = param << 8 | int(op);
    instructions.push_back(code);
    pc++;
}

void CodeWriter::set(int i, OpCode op, int param) 
{
    int code = param << 8 | int(op);
    if (i >= instructions.size()) {
        return;
    }
    instructions[i] = code;
}

int CodeWriter::get_pc()
{
    return pc;
}

vector<int> & CodeWriter::get_instructions()
{
    return instructions;
}