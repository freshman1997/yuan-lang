
#include <vector>
#include <unordered_map>

#include "parser.h"
#include "visitor.h"

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
    char *name = NULL;
    int name_len;
    int varIndex;
    vector<ScopeItem *> *members = NULL;       // 模块的子标识符
};

// 必须要能够快速定位声明的变量
struct Scope
{
    ScopeType type;
    vector<ScopeItem *> *items = NULL;     // 当前作用域的标识符
    vector<Scope *> *scopes = NULL;        // 子作用域，最后那个为目前的作用域
    int varIndex = 0;                   // 现在是第几个变量
    int moduleVarIndex = 0;
};


// 文件 -> 多个同级的 -> 每个作用域下有多个同级的
// <file, <>>
// 每进去一个作用域 push_back 一次
static unordered_map<string, Scope *> global_scopes;

static void enter_scope(const char *file, ScopeType type)
{
    Scope * &fileScope = global_scopes[file];
    // 继承所有父作用域的标识符 和 require 引入的部分；
    if (!fileScope->scopes) {
        fileScope->scopes = new vector<Scope *>;
        fileScope->items = new vector<ScopeItem *>;
        return;
    }
    fileScope->scopes->push_back(new Scope);
    fileScope->scopes->back()->type = type;
}

static void leave_scope(const char *file, int lv)
{
    // 清除当前作用域的所有声明的变量
    Scope *fileScope = global_scopes[file];
    if (fileScope->scopes->back()->items) {
        for(auto &it : *fileScope->scopes->back()->items) {
            delete it;
        }
        delete fileScope->scopes->back()->items;
    }
    delete fileScope->scopes->back();
    fileScope->scopes->pop_back();
}

static bool is_same_id(char *s1, int len1, char *s2, int len2)
{
    if (len1 != len2) return false;
    int i = 0;
    while (i < len1) {
        if (s1[i] != s2[i]) return false;
        i++;
    }
    return true;
}

// father 第一个id，child 第二个id，op 操作类型，索引，取值
static bool has_identifier(const char *file, char *father, int len1, char *child, int len2)
{
    Scope *fileScope = global_scopes[file];
    if (!father) {
        for (auto &it : *fileScope->scopes->back()->items) {
            if (is_same_id(child, len2, it->name, it->name_len)) {
                return true;
            }
        }
        return false;
    }

    for (auto &it : *fileScope->scopes->back()->items) {
        if (is_same_id(father, len1, it->name, it->name_len)) {
            if (it->type == VariableType::t_table) it->type = VariableType::t_module;
            for (auto &it1 : *it->members) {
                if (is_same_id(child, len2, it1->name, it1->name_len)) {
                    return true;
                }
            }
            return true;
        }
    }
    return false;
}

// father 第一个id，child 第二个id，模块才会有father
static void add_identifier(const char *file, char *father, int len1, char *child, int len2, VariableType type)
{
    if (has_identifier(file, father, len1, child, len2)) return;

    Scope *fileScope = global_scopes[file];
    if (!fileScope->scopes->back()->items) fileScope->scopes->back()->items = new vector<ScopeItem *>;
    if (!father) {
        ScopeItem *item = new ScopeItem;
        item->name = child;
        item->name_len = len2;
        item->type = type;
        fileScope->scopes->back()->items->push_back(item);
    }
    else {
        ScopeItem *fa = NULL;
        for (auto &it : *fileScope->scopes->back()->items) {
            if (is_same_id(father, len1, it->name, it->name_len)) {
                fa = it;
                break;
            }
        }
        if (!fa) {
            fa = new ScopeItem;
            fa->name = father;
            fa->name_len = len1;
            fa->type = VariableType::t_module;
            fa->members = new vector<ScopeItem *>;
            fileScope->scopes->back()->items->push_back(fa);
        }
        ScopeItem *item = new ScopeItem;
        item->name = child;
        item->name_len = len2;
        item->type = type;
        fa->members->push_back(item);
    }
}



static void visit_operation(OperationExpression *operExp, CodeWriter &writer)
{
    switch (operExp->op_type)
    {
    case OperatorType::op_none:
    {
        Operation *op = operExp->left;
        // 这里有多种类型的

        break;
    }
    case OperatorType::op_dot:
    {

        break;
    }
    case OperatorType::op_concat:
    {

        break;
    }
    case OperatorType::op_not:
    {

        break;
    }
    case OperatorType::op_len:
    {

        break;
    }
    case OperatorType::op_unary_sub:
    {

        break;
    }
    case OperatorType::op_add_add:
    {

        break;
    }
    case OperatorType::op_sub_sub:
    {

        break;
    }
    default:
        // 普通二元操作符，遍历是从左往右
        
        break;
    }
}


static void visit_assign()
{

}

static void visit_if()
{

}


void visit(unordered_map<string, Chunck *> *chunks, CodeWriter &writer)
{

}