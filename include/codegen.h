#ifndef __CODE_GEN_H__
#define __CODE_GEN_H__
#include <vector>
#include <map>

#include "instruction.h"

using namespace std;

void gen_push(int type, int id);    

void gen_store(int type, int id);   // 赋值

void gen_pop();

void gen_binop(OpCode op);          // 二元运算符，运算前需要 push 两次， 8 ~ 36

void gen_unaryop(OpCode op);        // 一元运算符 37 ~ 41

void gen_test();

void gen_jump(int offset);          // offset 可能是负的 continue ，return



#endif