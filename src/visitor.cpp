
#include <vector>
#include <unordered_map>
#include <cstdlib>

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
    Function *fun = NULL;
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
    union value
    {
        double val;
        ConstString *str = NULL;
        bool b;
    };
    value *v = NULL;
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
        global_const[std::hash<double>()(c->v->val)] = c;
    }
    else if (c->type == VariableType::t_string) {
        static string t;
        t.clear();
        for(int i = 0; i < c->v->str->len; ++i) {
            t.push_back(c->v->str->str[i]);
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
    FuncInfo * fileFuncInfo = NULL;
    if (!global_subFuncInfos.count(file)) {
        global_subFuncInfos[file] = new FuncInfo;
    }
    fileFuncInfo = global_subFuncInfos[file];
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
    FuncInfo *last = NULL;
    if (!fileFuncInfo->subFuncInfos->empty()) last = fileFuncInfo->subFuncInfos->back();
    fileFuncInfo->subFuncInfos->push_back(new FuncInfo);
    fileFuncInfo->subFuncInfos->back()->upvalue = new UpValueDesc;
    //fileFuncInfo->subFuncInfos->back()->upvalue->pre = last->upvalue;
    fileFuncInfo->subFuncInfos->back()->upvalue->upvalues = new vector<UpValueDesc *>;
    fileFuncInfo->subFuncInfos->back()->upvalue->upvalues->push_back(init_global_upvlaue());
    fileFuncInfo->subFuncInfos->back()->nupval++;
    fileFuncInfo->subFuncInfos->back()->items = new vector<FuncInfoItem *>;
    fileFuncInfo->subFuncInfos->back()->in_stack = last ? last->in_stack + 1 : 1;
}

static void leave_func(const char *file)
{
    // 清除当前作用域的所有声明的变量，记录需要清除的数据
    FuncInfo *fileFuncInfo = global_subFuncInfos[file];
    if (fileFuncInfo->subFuncInfos->empty()) return;
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
    while (cur) {
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

static int is_fun_param(FuncInfo *info, char *name, int len)
{
    if (!info->fun || info->fun->parameters->empty()) return -1;
    int i = 0;
    for (auto &it : *info->fun->parameters) {
        if (is_same_id(it->name, it->name_len, name, len)) {
            return i;
        }  
        ++i;
    }
    return -1;
}

static void syntax_error(const char *msg)
{
    cout << msg << endl;
    exit(0);
}

static vector<int> breaks;
static vector<int> continues;

static void visit_statement(vector<BodyStatment *> *statements, FuncInfo *info, CodeWriter &writer);
static void visit_operation(Operation *opera, FuncInfo *info, CodeWriter &writer);

static void visit_operation_exp(OperationExpression *operExp, FuncInfo *info, CodeWriter &writer)
{
    FuncInfo *fileFuncInfo = global_subFuncInfos[writer.get_file_name()];
    switch (operExp->op_type)
    {
    case OperatorType::op_none:
    {
        // 这里是纯字面量或者id
        visit_operation(operExp->left, info, writer);
    }
    case OperatorType::op_dot:
    {

        break;
    }
    case OperatorType::op_concat:
    {
        if (!(operExp->left->type == OpType::id || operExp->left->type == OpType::str || operExp->right->type == OpType::id || operExp->right->type == OpType::str)) {
            // error
            syntax_error("illegal type found");
        }
        break;
    }
    case OperatorType::op_not:
    {
        if (operExp->left->type != OpType::id) {
            // error
            syntax_error("illegal type found");
        }
        visit_operation(operExp->left, info, writer);
        writer.add(OpCode::op_add_add, 0);
        break;
    }
    case OperatorType::op_len:
    {
        if (!(operExp->left->type == OpType::id || operExp->left->type == OpType::arr || operExp->left->type == OpType::table)) {
            // error
            syntax_error("illegal type found");
        }
        visit_operation(operExp->left, info, writer);
        writer.add(OpCode::op_len, 0);
        break;
    }
    case OperatorType::op_unary_sub:
    {
        if (!(operExp->left->type == OpType::id || operExp->left->type == OpType::num)) {
            // error
            syntax_error("illegal type found");
        }
        visit_operation(operExp->left, info, writer);
        writer.add(OpCode::op_unary_sub, 0);
        break;
    }
    case OperatorType::op_add_add:
    {
        if (operExp->left->type != OpType::id) {
            // error
            syntax_error("illegal type found");
        }
        visit_operation(operExp->left, info, writer);
        writer.add(OpCode::op_add_add, 0);
        break;
    }
    case OperatorType::op_sub_sub:
    {
        if (operExp->left->type != OpType::id) {
            // error
            syntax_error("illegal type found");
        }
        visit_operation(operExp->left, info, writer);
        writer.add(OpCode::op_sub_sub, 0);
        break;
    }
    default:
        if (operExp->left->type == OpType::table || operExp->left->type == OpType::str || operExp->right->type == OpType::table || operExp->right->type == OpType::str) {
            // error
            syntax_error("illegal type found");
        }
        visit_operation(operExp->left, info, writer);
        visit_operation(operExp->right, info, writer);
        writer.add((OpCode)operExp->op_type, 0);
        break;
    }
}


static void visit_assign(AssignmentExpression *assign, FuncInfo *info, CodeWriter &writer)
{
    visit_operation_exp(assign->assign, info, writer);
    IdExpression *name = assign->id;
    char *id = name->name;
    int len = name->name_len;
    int gVarid = is_blobal_var(id, len);
    if (gVarid >= 0) {
        writer.add(OpCode::op_storeg, gVarid);
    }

    // 如果有 local 修饰，那么就是当前函数的变量
    pair<int, int> p;
    if (name->is_local) {
        if (has_identifier(info, id, len, p)) {
            // error redeclare 
            syntax_error("redeclare identifier");
        }

        FuncInfoItem *item = new FuncInfoItem;
        item->name = id;
        item->name_len = len;
        item->varIndex = info->actVars;
        info->items->push_back(item);
        writer.add(OpCode::op_pushl, info->actVars);
        info->actVars++;
    }
    else {
        bool found = has_identifier(info, id, len, p);
        if (found) {
            if (p.first == info->in_stack) {
                writer.add(OpCode::op_storel, p.second);
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
            writer.add(OpCode::op_storeu, info->nupval);
            info->nupval++;
        }
        else {
            FuncInfoItem item;
            item.name = id;
            item.name_len = len;
            item.varIndex = global_var_index;
            writer.add(OpCode::op_storeg, global_var_index);
            global_vars[global_var_index] = item;
            global_var_index++;
        }
    }
}

static void visit_if(IfExpression *ifExp, FuncInfo *info, CodeWriter &writer)
{
    int next = -1, last = -1;
    int i = 0;
    vector<int> jends;
    for (auto &it : *ifExp->if_statements) {
        if (i > 0) {
            next = writer.get_pc();
            writer.set(last, OpCode::op_jump, next - last);
        }
        if (it->condition) {
            visit_operation_exp(it->condition, info, writer);
            writer.add(OpCode::op_test, 0);
            // if else if else if else 跳到下一个条件
            writer.add(OpCode::op_jump, 0);
            last = writer.get_pc();
        }
        visit_statement(it->body, info, writer);
        writer.add(OpCode::op_jump, 0); // 跳出 if 
        jends.push_back(writer.get_pc());
        ++i;
    }
    // 修正跳转的位置
    for (auto &e : jends) {
        writer.set(e, OpCode::op_jump, writer.get_pc() - e + 1);
    }
}

static void visit_for(ForExpression *forExp, FuncInfo *info, CodeWriter &writer)
{
    if (forExp->type == ForExpType::for_normal) {
        writer.add(OpCode::op_for_normal, 0);
        if (forExp->first_statement) {
            for (auto &it : *forExp->first_statement) {
                visit_operation_exp(it, info, writer);
            }
        }
        int cond = -1;
        if (forExp->second_statement) {
            cond = writer.get_pc() + 1;
            visit_operation_exp(forExp->second_statement, info, writer);
            writer.add(OpCode::op_test, 0);
            writer.add(OpCode::op_jump, 0); // 跳出循环体或进入循环体
        }
        else {
            cond = writer.get_pc() + 1;
            writer.add(OpCode::op_load_bool, 1);
            writer.add(OpCode::op_test, 0);
        }
        // body
        visit_statement(forExp->body, info, writer);
        if (forExp->third_statement) {
            for (auto &it : *forExp->third_statement) {
                visit_operation_exp(it, info, writer);
            }
        }
        writer.add(OpCode::op_jump, -(writer.get_pc() - cond)); // jump back
        for (auto &it : continues) {
            writer.set(it, OpCode::op_jump, cond);
        }
        for (auto &it : breaks) {
            writer.set(it, OpCode::op_jump, writer.get_pc());
        }
        continues.clear();
        breaks.clear();
    }
    else {
        // for in 
        if (forExp->first_statement->empty()) {
            syntax_error("empty enum variable in for in loop");
        }
        int start = writer.get_pc() + 1;
        for (auto &it : *forExp->first_statement) {
            visit_operation_exp(it, info, writer);
        }
        for (auto &it : *forExp->third_statement) {
            visit_operation_exp(it, info, writer);
        }
        writer.add(OpCode::op_for_in, forExp->first_statement->size()); 
        writer.add(OpCode::op_jump, 0);  // jump out
        int out = writer.get_pc() - 1;
        // body
        visit_statement(forExp->body, info, writer);
        writer.add(OpCode::op_jump, -(writer.get_pc() - start)); // jump back
        writer.set(out, OpCode::op_jump, writer.get_pc() + 1);
        for (auto &it : continues) {
            writer.set(it, OpCode::op_jump, start);
        }
        for (auto &it : breaks) {
            writer.set(it, OpCode::op_jump, writer.get_pc());
        }
        continues.clear();
        breaks.clear();
    }
}

static void visit_while(WhileExpression *whileExp, FuncInfo *info, CodeWriter &writer)
{
    int start = writer.get_pc() + 1;
    visit_operation_exp(whileExp->condition, info, writer);
    writer.add(OpCode::op_test, 0);
    writer.add(OpCode::op_jump, 0);
    int out = writer.get_pc();
    visit_statement(whileExp->body, info, writer);
    writer.add(OpCode::op_jump, start);
    writer.set(out, OpCode::op_jump, writer.get_pc() + 1);
    for (auto &it : continues) {
        writer.set(it, OpCode::op_jump, start);
    }
    for (auto &it : breaks) {
        writer.set(it, OpCode::op_jump, writer.get_pc());
    }
    continues.clear();
    breaks.clear();
}

static void visit_do_while(DoWhileExpression *doWhileExp, FuncInfo *info, CodeWriter &writer)
{
    int start = writer.get_pc() + 1; // for jump back
    visit_statement(doWhileExp->body, info, writer);
    visit_operation_exp(doWhileExp->condition, info, writer);
    writer.add(OpCode::op_test, 0);
    writer.add(OpCode::op_jump, start); // 测试失败不执行
    for (auto &it : continues) {
        writer.set(it, OpCode::op_jump, start);
    }
    for (auto &it : breaks) {
        writer.set(it, OpCode::op_jump, writer.get_pc());
    }
    continues.clear();
    breaks.clear();
}

static void visit_return(ReturnExpression *retExp, FuncInfo *info, CodeWriter &writer)
{
    // a = ss() {return 100}
    visit_operation_exp(retExp->statement, info, writer);
    writer.add(OpCode::op_return, 0);
}

static void visit_call(CallExpression *call, FuncInfo *info, CodeWriter &writer)
{
    int pc = writer.get_pc();
    pair<int, int> p;
    char *name = call->function_name->name;
    int len = call->function_name->name_len;
    int pId = is_fun_param(info, name, len);
    if (pId >= 0) {
        writer.add(OpCode::op_get_fun_param, pId);
    }
    else {
        int gVarid = is_blobal_var(name, len);
        if (gVarid >= 0) {
            // 参数  fun(fun(12, 2), 3);
            for (auto &it : *call->parameters) {
                visit_operation(it, info, writer);
            }
            writer.add(OpCode::op_call, -gVarid);
            return;
        }
        if (!has_identifier(info, name, len, p)) {
            // 看在不在 env 中
            int i = 0;
            vector<UpValueDesc *> *upvs = info->upvalue->upvalues;
            for (auto &it : *(((*upvs)[0])->upvalues)) {
                if (is_same_id(it->name, it->name_len, name, len)) {
                    for (auto &it : *call->parameters) {
                        visit_operation(it, info, writer);
                    }
                    writer.add(OpCode::op_call_env, i);
                    return;
                }
                i++;
            }
            // error
            syntax_error("unkown identifier");
        }
        for (auto &it : *call->parameters) {
            visit_operation(it, info, writer);
        }
        if (p.first == info->in_stack) {
            writer.add(OpCode::op_call, p.second);
        }
        else {
            // upvalue call, 从当前函数的upvalue表找到函数
            writer.add(OpCode::op_call_upv, p.second);
        }
    }
}

static void visit_function_decl(Function *fun, FuncInfo *info, CodeWriter &writer)
{
    FuncInfo * fileFuncInfo = global_subFuncInfos[writer.get_file_name()];
    enter_func(writer.get_file_name());
    FuncInfo *newFun = fileFuncInfo->subFuncInfos->back();
    newFun->fun = fun;
    if (fun->is_local) {
        if (!info->subFuncInfos) {
            info->subFuncInfos = new vector<FuncInfo *>;
        }
        info->pre->subFuncInfos->push_back(newFun);
        FuncInfoItem *item = new FuncInfoItem;
        item->name = fun->function_name->name;
        item->name_len = fun->function_name->name_len;
        item->varIndex = info->actVars;
        info->pre->items->push_back(item);
        info->pre->actVars++;
        newFun->func_name = fun->function_name->name;
        newFun->name_len = fun->function_name->name_len;
    }
    else {
        if (fun->function_name) {
            FuncInfoItem item;
            item.name = fun->function_name->name;
            item.name_len = fun->function_name->name_len;
            item.varIndex = global_var_index;
            global_vars[global_var_index] = item;
            global_var_index++;
            newFun->func_name = fun->function_name->name;
            newFun->name_len = fun->function_name->name_len;
        }
        else {
            // 匿名函数
            FuncInfoItem item;
            item.varIndex = global_var_index;
            global_vars[global_var_index] = item;
            global_var_index++;
        }
    }
    
    newFun->nparam = fun->parameters->size();
    newFun->in_stack = info->in_stack + 1;
    writer.add(OpCode::op_enter_func, 0);
    // check parameter's name is same ?
    pair<int, int> p;
    for (auto &it : *fun->parameters) {
        int gVarid = is_blobal_var(it->name, it->name_len);
        if (has_identifier(info, it->name, it->name_len, p) || gVarid >= 0) {
            // error
            syntax_error("unkown identifier");
        }
    }

    //TODO visit body
    visit_statement(fun->body, newFun, writer);
    leave_func(writer.get_file_name());
}

static void visit_index(IndexExpression *index, FuncInfo *info, CodeWriter &writer)
{
    IdExpression *name = index->id;
    char *id = name->name;
    int len = name->name_len;
    int pId = is_fun_param(info, id, len);
    if (pId >= 0) {
        writer.add(OpCode::op_get_fun_param, pId);
    }
    else {
        int gVarid = is_blobal_var(id, len);
        // [][][][][][]
        if (gVarid >= 0) {
            writer.add(OpCode::op_pushg, gVarid);
        }
        else {
            pair<int, int> p;
            if (!has_identifier(info, id, len, p)) {
                // error unkown variable
                syntax_error("unkown identifier");
            }
            if (p.first == info->in_stack) {
                writer.add(OpCode::op_pushl, p.second);
            }
            else {
                // upvalue
                writer.add(OpCode::op_pushu, p.second);
            }
        }
    }
    for (auto &it : *index->keys) {
        visit_operation_exp(it, info, writer);
        writer.add(OpCode::op_index, 0);
    }
}

static void visit_statement(vector<BodyStatment *> *statements, FuncInfo *info, CodeWriter &writer)
{
    for (auto &it1 : *statements) {
        switch (it1->type)
        {
        case ExpressionType::oper_statement:
            visit_operation_exp(it1->body->oper_exp, info, writer);
            break;
        case ExpressionType::if_statement:
            visit_if(it1->body->if_exp, info, writer);
            break;
        case ExpressionType::for_statement:
            visit_for(it1->body->for_exp, info, writer);
            break;
        case ExpressionType::do_while_statement:
            visit_do_while(it1->body->do_while_exp, info, writer);
            break;
        case ExpressionType::while_statement:
            visit_while(it1->body->while_exp, info, writer);
            break;
        case ExpressionType::return_statement:
            visit_return(it1->body->return_exp, info, writer);
            break;
        case ExpressionType::call_statement:
            visit_call(it1->body->call_exp, info, writer);
            break;
        case ExpressionType::assignment_statement:
            visit_assign(it1->body->assign_exp, info, writer);
            break;
        case ExpressionType::function_declaration_statement:
            visit_function_decl(it1->body->function_exp, info, writer);
            break;
        case ExpressionType::break_statement:
            writer.add(OpCode::op_jump, 0);
            breaks.push_back(writer.get_pc());
            break;
        case ExpressionType::continue_statement:
            writer.add(OpCode::op_jump, 0);
            continues.push_back(writer.get_pc());
            break;
        default:
            syntax_error("not support statement");
            break;
        }
    }
}

static void visit_operation(Operation *opera, FuncInfo *info, CodeWriter &writer)
{
    if (!opera) return;

    FuncInfo * fileFuncInfo = global_subFuncInfos[writer.get_file_name()];
    // 这里有多种类型的
    switch (opera->type)
    {
    case OpType::id:
    {
        char *id = opera->op->id_oper->name;
        int len = opera->op->id_oper->name_len;
        int pId = is_fun_param(info, id, len);
        if (pId >= 0) {
            writer.add(OpCode::op_get_fun_param, pId);
        }
        else {
            int gVarid = is_blobal_var(id, len);
            if (gVarid >= 0) {
                writer.add(OpCode::op_pushl, gVarid);
                return;
            }

            pair<int, int> p;
            if (!has_identifier(info, id, len, p)) {
                // TODO error
                syntax_error("unkown indetifier");
                return;
            }
            else {
                if (p.first == info->in_stack) {
                    writer.add(OpCode::op_pushl, p.second);
                }
                else {
                    // upvalue
                    writer.add(OpCode::op_pushu, info->nupval);
                }
            }
        }
        break;
    }
    case OpType::num:
    {
        Const *c = new Const;
        c->type = VariableType::t_number;
        c->v = new Const::value;
        c->v->val = opera->op->number_oper->val;
        add_global_const(c);
        writer.add(OpCode::op_pushg, global_const_index - 1);
        global_const_index++;
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
        c->v = new Const::value;
        c->v = new Const::value;
        c->v->str = new ConstString;
        c->v->str->str = opera->op->string_oper->raw;
        c->v->str->len = opera->op->string_oper->raw_len;
        add_global_const(c);
        writer.add(OpCode::op_pushg, global_const_index - 1);
        global_const_index++;
        break;
    }
    case OpType::substr:
    {
        // [2:-1]
        IndexExpression *substr = opera->op->index_oper;
        OperationExpression *param = (*substr->keys)[0];
        visit_operation(param->left, info, writer);
        visit_operation(param->right, info, writer);
        writer.add(OpCode::op_substr, global_const_index - 1);
        break;
    }
    case OpType::arr:
    {
        writer.add(OpCode::op_array_new, 0);
        Array *arr = opera->op->array_oper;
        for (auto &it : *arr->fields) {
            visit_operation(it, info, writer);
            writer.add(OpCode::op_array_set, 0);
        }
        break;
    }
    case OpType::table:
    {
        Table *tb = opera->op->table_oper;
        writer.add(OpCode::op_table_new, 0);
        for (auto &it : *tb->members) {
            visit_operation_exp(it->v, info, writer);
            visit_operation_exp(it->k, info, writer);
            writer.add(OpCode::op_table_set, 0);
        }
        break;
    }
    case OpType::assign:
    {
        AssignmentExpression *assign = opera->op->assgnment_oper;
        visit_assign(assign, info, writer);
        break;
    }
    case OpType::index:
    {
        IndexExpression *index = opera->op->index_oper;
        visit_index(index, info, writer);
        break;
    }
    case OpType::op:
    {
        OperationExpression *oper = opera->op->op_oper;
        visit_operation_exp(oper, info, writer);
        break;
    }
    case OpType::call:
    {
        visit_call(opera->op->call_oper, info, writer);
        break;
    }
    case OpType::function_declear:
    {
        visit_function_decl(opera->op->fun_oper, info, writer);
        break;
    }
    default:
        break;
    }
}

void visit(unordered_map<string, Chunck *> *chunks, CodeWriter &writer)
{
    for(auto &it : *chunks) {
        enter_func(it.first.c_str());
        writer.set_file_name(it.first.c_str());
        visit_statement(it.second->statements, global_subFuncInfos[it.first.c_str()], writer);
        leave_func(it.first.c_str());
    }
}