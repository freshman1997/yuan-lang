
#include <vector>
#include <unordered_map>

#include "parser.h"
#include "visitor.h"
#include "codegen.h"

using namespace std;

struct UpValueDesc
{
    int stack_lv = 0;
    char *name;
    int name_len;
    bool isfun = false;
    int index = 0;
    UpValueDesc *pre = NULL;
    vector<UpValueDesc *> *upvalues;
};

enum class ScopeType
{
    main_scope,             // 整个文件
    funtion_scope,          // 函数
    if_scope,               // if, else if, else
    for_scope,              // for
    while_scope,            // while
    do_while_scope,         // do while
};

// local 部分
struct ScopeItem 
{
    VariableType type;                         // 这个需要推导其他表达式类型 并得出结果
    const char *name = NULL;
    int name_len = 0;
    int varIndex = 0;
    vector<ScopeItem *> *members = NULL;       // 模块的子标识符
};

// 必须要能够快速定位声明的变量
struct Scope
{
    ScopeType type;
    vector<ScopeItem *> *items = NULL;     // 当前作用域的标识符
    vector<Scope *> *scopes = NULL;        // 子作用域，最后那个为目前的作用域
    int actVars = 0;                       // 现在是第几个变量  local 
    int moduleVars = 0;
    int nupval = 0;
    UpValueDesc *upvalue = NULL;
};

struct Const
{
    VariableType type;
    union 
    {
        int val;
        const char *str;
        int len;
    } *value;
    //value *v = NULL;
};

// 文件 -> 多个同级的 -> 每个作用域下有多个同级的
// <file, <>>
// 每进去一个作用域 push_back 一次
static unordered_map<string, Scope *> global_scopes;
static UpValueDesc *env_upvlaue = new UpValueDesc;
static unordered_map<int, Const*> global_const;
static int global_const_index = 0;

static void init_global_upvlaue()
{
    env_upvlaue->name = "_ENV";
    env_upvlaue->name_len = 4;
    env_upvlaue->upvalues = new vector<UpValueDesc *>;
    env_upvlaue->upvalues->push_back(new UpValueDesc);
    env_upvlaue->upvalues->back()->name = "print";
    env_upvlaue->upvalues->back()->name_len = 5;
}


static void enter_scope(const char *file, ScopeType type)
{
    Scope * &fileScope = global_scopes[file];
    // 继承所有父作用域的标识符 和 require 引入的部分；
    if (!fileScope->scopes) {
        fileScope->scopes = new vector<Scope *>;
        fileScope->items = new vector<ScopeItem *>;
        fileScope->upvalue = new UpValueDesc;
        fileScope->upvalue->pre = env_upvlaue;
        fileScope->upvalue->upvalues = new vector<UpValueDesc *>;
        fileScope->upvalue->upvalues->push_back(env_upvlaue);
        return;
    }
    Scope *last = fileScope->scopes->back();
    fileScope->scopes->push_back(new Scope);
    fileScope->scopes->back()->type = type;
    fileScope->scopes->back()->upvalue = new UpValueDesc;
    fileScope->scopes->back()->upvalue->pre = last->upvalue;
    fileScope->scopes->back()->upvalue->upvalues = new vector<UpValueDesc *>;
    fileScope->scopes->back()->upvalue->upvalues->push_back(env_upvlaue);
    fileScope->scopes->back()->nupval++;
}

static void leave_scope(const char *file)
{
    // 清除当前作用域的所有声明的变量
    Scope *fileScope = global_scopes[file];
    if (fileScope->scopes->back()->items) {
        for(auto &it : *fileScope->scopes->back()->items) {
            delete it;
        }
        delete fileScope->scopes->back()->items;
    }
    delete fileScope->scopes->back()->upvalue;
    delete fileScope->scopes->back();
    fileScope->scopes->pop_back();
}

static bool is_same_id(char *s1, int len1, const char *s2, int len2)
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
        for (auto &it : *fileScope->scopes->front()->items) {
            if (is_same_id(child, len2, it->name, it->name_len)) {
                return true;
            }
        }
        return false;
    }

    for (auto &it : *fileScope->scopes->front()->items) {
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
        fa->varIndex++;
    }
}

static void visit_operation(Operation *opera, Instruction *ins, CodeWriter &writer)
{
    // 这里有多种类型的
    switch (opera->type)
    {
    case OpType::id:
    {
        // 全局变量
        if (has_identifier(writer.get_file_name(), NULL, 0, opera->op->id_oper->name, opera->op->id_oper->name_len)) {
            
        }
        break;
    }
    case OpType::num:
    {
        
        break;
    }
    case OpType::boolean:
    {
        
        break;
    }
    case OpType::nil:
    {
        
        break;
    }
    case OpType::str:
    {
        
        break;
    }
    case OpType::substr:
    {
        
        break;
    }
    case OpType::arr:
    {
        
        break;
    }
    case OpType::table:
    {
        
        break;
    }
    case OpType::assign:
    {
        
        break;
    }
    case OpType::index:
    {
        
        break;
    }
    case OpType::op:
    {
        
        break;
    }
    case OpType::call:
    {
        
        break;
    }
    case OpType::function_declear:
    {
        
        break;
    }
    default:
        break;
    }
}

static void visit_operation_exp(OperationExpression *operExp, CodeWriter &writer)
{
    Scope *fileScope = global_scopes[writer.get_file_name()];
    Scope *scope = fileScope->scopes->back();
    Instruction *ins = new Instruction;
    switch (operExp->op_type)
    {
    case OperatorType::op_none:
    {

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

    case OperatorType::op_add:
    {

        break;
    }
    default:

        break;
    }
}


static void visit_assign(AssignmentExpression *assignExp, CodeWriter &writer)
{

}

static void visit_if(IfExpression *ifExp, CodeWriter &writer)
{

}

static void visit_for(ForExpression *forExp, CodeWriter &writer)
{

}

static void visit_while(WhileExpression *whileExp, CodeWriter &writer)
{

}

static void visit_do_while(DoWhileExpression *doWhileExp, CodeWriter &writer)
{

}

static void visit_return(ReturnExpression *retExp, CodeWriter &writer)
{

}

static void visit_call(ReturnExpression *retExp, CodeWriter &writer)
{

}

static void visit_function_decl(ReturnExpression *retExp, CodeWriter &writer)
{

}

static void visit_block(ReturnExpression *retExp, CodeWriter &writer)
{

}

void visit(unordered_map<string, Chunck *> *chunks, CodeWriter &writer)
{
    for(auto &it : *chunks) {
       enter_scope(it.first.c_str(), ScopeType::main_scope);
       for (auto &it1 : *it.second->statements) {
           switch (it1->type)
           {
            case ExpressionType::oper_statement:
                break;
            case ExpressionType::if_statement:
                break;
            case ExpressionType::for_statement:
                break;
            case ExpressionType::do_while_statement:
                break;
            case ExpressionType::while_statement:
                break;
            case ExpressionType::return_statement:
                break;
            case ExpressionType::call_statement:
                break;
            case ExpressionType::block_statement:
                break;
            case ExpressionType::assignment_statement:
                break;
            case ExpressionType::function_declaration_statement:
                break;
           
           default:
               break;
           }
       }
       leave_scope(it.first.c_str());
    }
}