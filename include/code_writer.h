#ifndef __CODE_WRITER_H__
#define __CODE_WRITER_H__
#include <vector>

#include "instruction.h"

using namespace std;

class CodeWriter
{
public:
    void set_file_name(const char *file_name);
   
    const char * get_file_name();

    void add(OpCode op, int param);

    void set(int i, OpCode op, int param);
   
    int get_pc();

    vector<int> & get_instructions();
    
private:
    vector<int> instructions;
    const char *file_name = NULL;
    int pc = 0;
};


#endif