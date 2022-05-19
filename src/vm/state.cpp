#include <fstream>

#include "state.h"
#include "types.h"
#include "yuan.h"

static bool is_basic_type(Value *val)
{
    return val->get_type() == ValueType::t_boolean || val->get_type() == ValueType::t_null || val->get_type() == ValueType::t_number || val->get_type() == ValueType::t_function;
}

Value * State::pop()
{
    if (cur->param_stack - cur->nparam == get_stack_size()) return new NilVal;
    return this->stack->pop();
}

void State::push(Value *val)
{
    this->stack->push(val);
}

VM * State::get_vm()
{
    return this->vm;
}

State::State(size_t sz)
{
    this->stack = new VmStack(sz);
    this->files = new unordered_map<string, FunctionVal *>;
    this->calls = new vector<FunctionVal *>;
    this->openedFuns = new vector<FunctionVal *>;
    this->vm = new VM;
    if (!this->stack) {
        cout << "initial fail!" << endl;
        exit(1);
    }
}

void State::pushg(int i)
{
    FunctionVal *global = cur;
    while (global->pre) {
        global = global->pre;
    }
    if (i >= global->chunk->global_vars->size()) {
        cout << "invalid index: " << i << endl;
        exit(0);
        return;
    }

    Value *val = (*global->chunk->global_vars)[i];
    if (!val) {
        cout << "null val: " << __LINE__ << endl;
        exit(0);
    }
    push(val);
}


void State::pushc(int i) 
{
    FunctionVal *global = cur;
    while (global->pre) {
        global = global->pre;
    }
    if (i >= global->chunk->const_datas->size()) {
        cout << "invalid index: " << i << endl;
        exit(0);
        return;
    }

    Value *val = (*global->chunk->const_datas)[i];
    if (!val) {
        cout << "null val: " << __LINE__ << endl;
        exit(0);
    }
    push(val->copy());
}

void State::pushl(int i)
{
    Value *val = cur->get_localvar(i);
    if (is_basic_type(val))
        push(val->copy());
    else 
        push(val);
}

void State::pushu(int i)
{
    Value *val = getu(i);
    if (val->get_type() == ValueType::t_function) 
        push(val->copy());
    else push(val);
}

Value * State::getc(int i)
{
    return cur->get_global_const(i);
}

Value * State::getg(int i)
{
    return cur->get_global_var(i);
}

Value * State::getl(int i)
{
    return cur->get_localvar(i);
}

Value * State::getu(int i)
{
    UpValue *upv = cur->get_upvalue(i);
    if (!upv) return NULL;
    else {
        int stack = upv->in_stack;
        int index = upv->index;
        int funStack = cur->in_stack;
        FunctionVal *tar = cur;
        while (stack < funStack) {
            tar = tar->pre;
            if (!tar) return NULL;
            --funStack;
        }
        vector<Value *> *locVars = tar->chunk->local_variables;
        if (locVars->size() <= index) return NULL;
        return locVars->at(index);
    }
}

Value * State::get(int pos)
{
    if (pos >= stack->get_size()) return NULL;
    if (pos <= 0) {
        pos = stack->get_size() + pos - 1;
    }
    return stack->get(pos);
}

Value * State::get_subfun(int i)
{
    if (!cur->chunk->upvals->at(0)) cur->chunk->upvals->at(0) = cur->pre->chunk->upvals->at(0);
    FunctionVal *subfun = dynamic_cast<FunctionVal *>(cur->get_subfun(i)->copy());
    if (!subfun->chunk->upvals->at(0)) subfun->chunk->upvals->at(0) = subfun->pre->chunk->upvals->at(0);
    return subfun;
}

FunctionVal * State::get_by_file_name(const char *filename)
{
    if (this->files->count(filename)) return (*this->files)[filename];
    load(filename);
    return (*this->files)[filename];
}

FunctionVal * State::get_cur()
{
    return this->cur;
}

void State::set_cur(FunctionVal *fun)
{
    this->cur = fun;
    this->calls->push_back(fun);
    cur->ref_count++;
}

bool State::tryClearOpenedFuns(FunctionVal *fun)
{
    bool hasUse = false;
    for (auto &it : *fun->chunk->local_variables) {
        if (it && it->ref_count > 1) {
            hasUse = true;
            break;
        }
    }
    if (!hasUse) {
        if (fun->ref_count <= 0) delete fun;
        return true;
    }
    return false;
}

void State::end_call(bool clearRet)
{
    if (cur->isC) {
        cur->ncalls--;
        calls->pop_back();
        if (!calls->empty()) cur = calls->back();
        return;
    }

    clearTempData(clearRet);
    if (!tryClearOpenedFuns(cur)) openedFuns->push_back(cur);
    
    if (!this->calls->empty()){
        calls->pop_back();
        // 最后退出的是main chunk
        if (!calls->empty()) cur = calls->back();
    }
    for (auto it = openedFuns->begin(); it != openedFuns->end(); ) {
        bool ret = this->tryClearOpenedFuns(*it);
        if (ret) it = openedFuns->erase(it);
        else ++it;
    }
    if (calls->empty()) {
        delete openedFuns;
        delete calls;
        delete stack;
        delete vm;
        delete cur;
    }
}

int State::get_stack_size()
{
    return this->stack->get_size();
}

int State::cur_calls()
{
    return calls->size();
}

void State::clearTempData(bool clear)
{
    Value *ret = NULL;
    if (cur->nreturn) {
        // 检查函数开始时的栈的位置，如果没有返回推入一个nil
        if (this->get_stack_size() == cur->param_stack) {
            if (!clear) ret = new NilVal;
        }
        else ret = pop();
        if (clear) { 
            check_variable_liveness(ret);
            ret = NULL;
        }
    }
    int start = cur->param_stack;
    int end = stack->get_size();
    if (start != end) { // 仍然有未出栈的
        for (int i = start; i < end; ++i) {
            Value *val = pop();
            if (val->ref_count <= 0) check_variable_liveness(val);
        }
    }
    if (ret) push(ret);
}

static void read_function(FunctionVal *funChunk, ifstream &in)
{
    int name_len = 0;
    in.read((char *)&name_len, sizeof(int));
    string funName;
    if (name_len > 0) {
        funName.reserve(name_len);
        for (int i = 0; i < name_len; i++) funName.push_back(in.get());
        funChunk->set_name(funName);
    }
    int in_stack = 0;
    in.read((char *)&in_stack, sizeof(int));
    funChunk->in_stack = in_stack;

    int nparam = 0, nreturn = 0, localVars = 0;
    in.read((char *)&nparam, sizeof(int));
    in.read((char *)&nreturn, sizeof(int));
    funChunk->nparam = nparam;
    funChunk->nreturn = nreturn;

    char isVarargs = 0;
    in.read(&isVarargs, sizeof(char));
    funChunk->chunk->is_varags = isVarargs;

    in.read((char *)&localVars, sizeof(int));
    funChunk->chunk->local_variables = new vector<Value *>(localVars, NULL);

    char isLocal = false;
    in.read((char *)&isLocal, sizeof(char));

    int from_pc = 0, to_pc = 0;
    in.read((char *)&from_pc, sizeof(int));
    in.read((char *)&to_pc, sizeof(int));
    funChunk->from_pc = from_pc;
    funChunk->to_pc = to_pc;
    funChunk->is_local = isLocal;

    int nupvalue = 0;
    in.read((char *)&nupvalue, sizeof(int));
    funChunk->add_upvalue(NULL); // env
    funChunk->upvOwner = true;
    
    for (int i = 0; i < nupvalue; i++) {
        UpValue *upv = new UpValue;
        in.read((char *)&upv->upval_index, sizeof(int));
        in.read((char *)&upv->in_stack, sizeof(int));
        in.read((char *)&upv->index, sizeof(int));
        funChunk->add_upvalue(upv);
    }
    int subFunSz = 0;
    in.read((char *)&subFunSz, sizeof(int));
    vector<FunctionVal *> *subFuns = new vector<FunctionVal *>;
    subFuns->reserve(subFunSz);
    funChunk->set_subfuns(subFuns);
    for (int i = 0; i < subFunSz; i++) {
        FunctionVal *subFun = new FunctionVal;
        subFun->set_file_name(funChunk->get_file_name()->c_str());
        read_function(subFun, in);
        subFun->pre = funChunk;
        subFuns->push_back(subFun);
    }
}

static bool get_target_bin_file_name(const char* origin, string &target);

void State::load(const char *file_name)
{
    string targetFile;
    if (!get_target_bin_file_name(file_name, targetFile)) 
    {
        exit(0);
    }

    ifstream in;
    in.open(targetFile.c_str(), ios_base::binary);
    if (!in.good()) {
        cout << "no such file or directory: " << file_name << endl;
        exit(0);
    }
    
    FunctionVal *mainFun = new FunctionVal;
    mainFun->set_file_name(file_name);
    FunctionChunk *mainChunk = new FunctionChunk;
    mainFun->set_chunk(mainChunk);
    mainChunk->const_datas = new vector<Value*>;
    int gConsts = -1;
    in.read((char *)&gConsts, sizeof(int));
    mainChunk->const_datas->reserve(gConsts);
    for (int i = 0; i < gConsts; i++) {
        char type = 0;
        Value *val = NULL;
        in.read(&type, sizeof(char));
        if (type == 0) {
            NumberVal *num = new NumberVal;
            double value = 0;
            in.read((char *)&value, sizeof(double));
            num->set_val(value);
            val = num;
        }
        else {
            StringVal *str = new StringVal;
            val = str;
            int len = 0;
            in.read((char *)&len, sizeof(int));
            for (int j = 0; j < len; j++) {
                str->push(in.get());
            }
        }
        val->ref_count = 1;
        mainChunk->const_datas->push_back(val);
    }

    int gVars = 0;
    in.read((char *)&gVars, sizeof(int));
    ++gVars;
    mainChunk->global_vars = new vector<Value *>(gVars, NULL);
    mainChunk->global_vars->reserve(gVars);
    mainChunk->global_var_names_map = new unordered_map<string, int>;
    string t;
    for (int i = 0; i < gVars - 1; i++) {
        int len = 0;
        in.read((char *)&len, sizeof(int));
        for (int j = 0; j < len; j++) {
            t.push_back(in.get());
        }
        (*mainChunk->global_var_names_map)[t] = i;
    }
    int pcs = 0;
    in.read((char *)&pcs, sizeof(int));
    vector<int> *opcodes = new vector<int>;
    opcodes->reserve(pcs);
    int code = 0;
    for (int i = 0; i < pcs; i++) {
        in.read((char *)&code, sizeof(int));
        opcodes->push_back(code);
    }
    mainChunk->fun_body_ops = opcodes;
    mainFun->isMain = true;

    read_function(mainFun, in);

    (*this->files)[file_name] = mainFun;

    in.close();
}

static TableVal * init_env(VM *vm)
{
    static TableVal *tb = new TableVal;
    if (tb->size() == 0) vm->load_lib(tb);
    return tb;
}

static bool get_target_bin_file_name(const char* origin, string &target)
{
    int i = strlen(origin) - 1;
    while (origin[i] != '.' && i > 0) --i;
    if (i == 0) {
        cout << "not yuan script file!" << endl;
        return false;
    }

    for (int j = 0; j <= i; ++j) {
        target.push_back(origin[j]);
    }
    target.push_back('b');
    return true;
}

bool State::require(const char *file, ArrayVal *args)
{
    if (!this->files->count(file)) {
        // 先不考虑容错，require 失败直接 crash 
        string targetFile;
        if (!get_target_bin_file_name(file, targetFile)) return false;
    }
    FunctionVal *fileChunk = run(file, args);
    return true;
}

FunctionVal * State::run(const char *entryFile, ArrayVal *args)
{
    bool isEntry = files->empty();
    FunctionVal *entry = get_by_file_name(entryFile);
    if (!entry) {
        cout << "entry not found !!" << endl;
        return NULL;
    }

    // 入参？
    if (entry->chunk->global_vars->at(0)) {
        delete entry->chunk->global_vars->at(0);
    }

    entry->isEntry = isEntry;
    args->ref_count++;
    entry->chunk->global_vars->at(0) = args;
    entry->chunk->upvals->at(0) = new UpValue;
    entry->chunk->upvals->at(0)->val = init_env(this->vm);
    entry->ncalls++;
    set_cur(entry);
    vm->execute(*entry->get_pcs(), this, 0, NULL);
    end_call(isEntry);
    entry->ncalls--;
    return entry;
}