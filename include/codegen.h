#ifndef __CODE_GEN_H__
#define __CODE_GEN_H__
#include <vector>
#include <unordered_map>

#include "parser.h"

using namespace std;

enum class ScopeType
{
    main_scope,             // 整个文件
    funtion_scope,          // 函数
    if_scope,               // if, else if, else
    for_scope,              // for
    while_scope,            // while
    do_while_scope,         // do while
};

struct ScopeItem 
{
    VariableType type;          // 这个需要推导其他表达式类型 并得出结果
    const char *name;
    int name_len;
};

struct Scope
{
    int lv;
    ScopeType type;
    vector<ScopeItem *> *items;
    vector<Scope *> *scopes;
};

#endif