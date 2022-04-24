#ifndef __CODE_GEN_H__
#define __CODE_GEN_H__
#include <vector>
#include <map>

using namespace std;

struct UpValueDesc
{
    int stack_lv = 0;
    char *name;
    int name_len;
    UpValueDesc *pre = NULL;
    map<int, vector<UpValueDesc *> *> *upvalues;
};


#endif