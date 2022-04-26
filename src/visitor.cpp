
#include <vector>
#include <unordered_map>

#include "parser.h"
#include "visitor.h"
#include "codegen.h"
#include "yuan.h"

using namespace std;

struct UpValueDesc
{
    int stack_lv = 0;
    int stack_index = 0;
    char *name;
    int name_len;
    bool isfun = false;
    int index = 0;
    vector<UpValueDesc *> *upvalues;
};

// local 部分
struct FuncInfoItem 
{
    char *name = NULL;
    int name_len = 0;
    int varIndex = 0;
    vector<FuncInfoItem *> *members = NULL;         // 模块的子标识符
};

// 必须要能够快速定位声明的变量
struct FuncInfo
{
    vector<FuncInfoItem *> *items = NULL;           // 当前作用域的标识符
    vector<FuncInfo *> *subFuncInfos = NULL;        // 子作用域，最后那个为目前的作用域
    int actVars = 0;                                // 现在是第几个变量  local 
    int moduleVars = 0;
    int nupval = 0;
    char *func_name = NULL;
    int name_len = 0;
    int nparam = 0;
    int nreturn = 0;
    int in_stack = 0;
    FuncInfo *pre = NULL;
    UpValueDesc *upvalue = NULL;
};

struct ConstString
{
    char *str;
    int len;
};

struct Const
{
    VariableType type;      // 只能是数字，字符串，bool，nil 四种类型
    union 
    {
        double val;
        ConstString *str = NULL;
        bool b;
    } *value;
    //value *v = NULL;
};

// 文件 -> 多个同级的 -> 每个作用域下有多个同级的
// <file, <>>
// 每进去一个作用域 push_back 一次
static unordered_map<string, FuncInfo *> global_subFuncInfos;
static unordered_map<int, FuncInfoItem> global_vars;    //  <编号，信息>
static unordered_map<int, Const*> global_const;     // 常量表 编号，值
static int global_const_index = 0;
static int global_var_index = 0;

static void add_global_const(Const *c)
{
    if (c->type == VariableType::t_number) {
        global_const[std::hash<double>()(c->value->val)] = c;
    }
    else if (c->type == VariableType::t_string) {
        static string t;
        t.clear();
        for(int i = 0; i < c->value->str->len; ++i) {
            t.push_back(c->value->str->str[i]);
        }
        global_const[std::hash<string>()(t)] = c;
    }
}

static UpValueDesc * init_global_upvlaue()
{
    UpValueDesc *env_upvlaue = new UpValueDesc;
    env_upvlaue->name = "_ENV";
    env_upvlaue->name_len = 4;
    env_upvlaue->upvalues = new vector<UpValueDesc *>;
    env_upvlaue->upvalues->push_back(new UpValueDesc);
    env_upvlaue->upvalues->back()->name = "print";
    env_upvlaue->upvalues->back()->name_len = 5;
    return env_upvlaue;
}


static void enter_func(const char *file)
{
    FuncInfo * fileFuncInfo = global_subFuncInfos[file];
    // 继承所有父作用域的标识符 和 require 引入的部分；
    if (!fileFuncInfo->subFuncInfos) {
        fileFuncInfo->subFuncInfos = new vector<FuncInfo *>;
        fileFuncInfo->items = new vector<FuncInfoItem *>;
        fileFuncInfo->upvalue = new UpValueDesc;
        fileFuncInfo->upvalue->upvalues = new vector<UpValueDesc *>;
        fileFuncInfo->upvalue->upvalues->push_back(init_global_upvlaue());
        fileFuncInfo->func_name = "main";
        fileFuncInfo->name_len = 4;
        fileFuncInfo->in_stack = 0;
        // 最后离开文件时需要检查返回的值
        return;
    }
    FuncInfo *last = fileFuncInfo->subFuncInfos->back();
    fileFuncInfo->subFuncInfos->push_back(new FuncInfo);
    fileFuncInfo->subFuncInfos->back()->upvalue = new UpValueDesc;
    //fileFuncInfo->subFuncInfos->back()->upvalue->pre = last->upvalue;
    fileFuncInfo->subFuncInfos->back()->upvalue->upvalues = new vector<UpValueDesc *>;
    fileFuncInfo->subFuncInfos->back()->upvalue->upvalues->push_back(init_global_upvlaue());
    fileFuncInfo->subFuncInfos->back()->nupval++;
    fileFuncInfo->subFuncInfos->back()->in_stack =  last->in_stack + 1;
}

static void leave_func(const char *file)
{
    // 清除当前作用域的所有声明的变量，记录需要清除的数据
    FuncInfo *fileFuncInfo = global_subFuncInfos[file];
    if (fileFuncInfo->subFuncInfos->back()->items) {
        for(auto &it : *fileFuncInfo->subFuncInfos->back()->items) {
            delete it;
        }
        delete fileFuncInfo->subFuncInfos->back()->items;
    }
    delete fileFuncInfo->subFuncInfos->back()->upvalue;
    delete fileFuncInfo->subFuncInfos->back();
    fileFuncInfo->subFuncInfos->pop_back();
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

static bool has_identifier(FuncInfo *info, char *name, int len, pair<int, int> &p) // p 为 <在函数栈中的位置，第几个变量>
{
    FuncInfo *cur = info;
    while (cur->pre) {
        int i = 0;
        for (auto &it : *cur->items) {
            if (is_same_id(it->name, it->name_len, name, len)) {
                p.first = cur->in_stack;
                p.second = i;
                return true;
            }
            ++i;
        }
        cur = cur->pre;
    }
    return false;
}

static int is_blobal_var(char *name, int len)
{
    for (auto &it : global_vars) {
        if (is_same_id(it.second.name, it.second.name_len, name, len)) {
            return it.first;
        }
    }
    return -1;
}

static void syntax_error()
{

}

static void visit_operation(Operation *opera, FuncInfo *info, CodeWriter &writer)
{
    FuncInfo * fileFuncInfo = global_subFuncInfos[writer.get_file_name()];
    // 这里有多种类型的
    switch (opera->type)
    {
    case OpType::id:
    {
        char *id = opera->op->id_oper->name;
        int len = opera->op->id_oper->name_len;
        int gVarid = is_blobal_var(id, len);
        if (gVarid >= 0) {
            writer.add(OpCode::op_pushl, gVarid);
            return;
        }

        // 如果有 local 修饰，那么就是当前函数的变量
        pair<int, int> p;
        if (opera->op->id_oper->is_local) {
            if (has_identifier(info, id, len, p)) {
                // error redeclare 
                syntax_error();
            }

            FuncInfoItem *item = new FuncInfoItem;
            item->name = id;
            item->name_len = len;
            item->varIndex = info->actVars;
            info->items->push_back(item);
            writer.add(OpCode::op_pushl, info->actVars);
            info->actVars++;
            return;
        }
        // 如果在当前函数找不到，往前找，直到全局变量，若是找不到且没有local那就是全局变量
        // 如果在前面的函数中找到了，那么就是 upvalue ，匿名函数就是 upvalue 
        if (!has_identifier(info, id, len, p)) {
            FuncInfoItem item;
            item.name = id;
            item.name_len = len;
            item.varIndex = global_var_index;
            writer.add(OpCode::op_pushg, global_var_index);
            global_vars[global_var_index] = item;
            return;
        }
        else {
            if (p.first == info->in_stack) {
                writer.add(OpCode::op_pushg, p.second);
                return;
            }
            // upvalue
            UpValueDesc *up = new UpValueDesc;
            up->name = id;
            up->name_len = len;
            up->index = info->nupval;
            up->stack_index = p.second;
            up->stack_lv = p.first;
            up->index = info->nupval;
            info->upvalue->upvalues->push_back(up);
            writer.add(OpCode::op_pushg, info->nupval);
            info->nupval++;
        }
        break;
    }
    case OpType::num:
    {
        Const *c = new Const;
        c->type = VariableType::t_number;
        c->value->val = opera->op->number_oper->val;
        add_global_const(c);
        writer.add(OpCode::op_pushg, global_const_index - 1);
        break;
    }
    case OpType::boolean:
    {
        writer.add(OpCode::op_load_bool, 0);
        break;
    }
    case OpType::nil:
    {
        writer.add(OpCode::op_load_nil, 0);
        break;
    }
    case OpType::str:
    {
        Const *c = new Const;
        c->type = VariableType::t_string;
        c->value->str = new ConstString;
        c->value->str->str = opera->op->string_oper->raw;
        c->value->str->len = opera->op->string_oper->raw_len;
        add_global_const(c);
        writer.add(OpCode::op_pushg, global_const_index - 1);
        break;
    }
    case OpType::substr:
    {
        // [2:-1]
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
        int pc = writer.get_pc();
        CallExpression *call = opera->op->call_oper;
        pair<int, int> p;
        char *name = call->function_name->name;
        int len = call->function_name->name_len;
        int gVarid = is_blobal_var(name, len);
        if (gVarid >= 0) {
            // 参数  fun(fun(12, 2), 3);
            for (auto &it : *call->parameters) {
                visit_operation(*it, info, writer);
            }
            writer.add(OpCode::op_call, gVarid);
            return;
        }
        if (!has_identifier(info, name, len, p)) {
            // error
            syntax_error();
        }
        if (p.first == info->in_stack) {
            for (auto &it : *call->parameters) {
                visit_operation(*it, info, writer);
            }
            writer.add(OpCode::op_call, p.second);
        }
        else {
            // upvalue call
            
        }
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
    FuncInfo *fileFuncInfo = global_subFuncInfos[writer.get_file_name()];
    FuncInfo *FuncInfo = fileFuncInfo->subFuncInfos->back();
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
       enter_func(it.first.c_str());
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
       leave_func(it.first.c_str());
    }
}