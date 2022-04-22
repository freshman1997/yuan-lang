#include "codegen.h"


// 文件 -> 多个同级的 -> 每个作用域下有多个同级的
// <file, <>>
static unordered_map<string, vector<Scope>> global_scopes;

static void enter_scope(const char *file, int lv, BodyStatment *blockStatements)
{

}

static void leave_scope(const char *file, int lv)
{
    
}

