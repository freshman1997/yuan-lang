#include <fstream>

#include "state.h"
#include "types.h"

Value * State::pop()
{
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
    this->vm = new VM;
    if (!this->stack) {
        cout << "initial fail!" << endl;
        exit(1);
    }
}

void State::pushg(int i)
{
    if (i >= cur->chunk->global_vars->size()) {
        cout << "invalid index: " << i << endl;
        exit(0);
        return;
    }

    Value *val = (*cur->chunk->global_vars)[i];
    if (!val) {
        cout << "null val: " << __LINE__ << endl;
        exit(0);
    }
    push(val);
}

void State::pushc(int i) 
{
    if (i >= cur->chunk->const_datas->size()) {
        cout << "invalid index: " << i << endl;
        exit(0);
        return;
    }

    Value *val = (*cur->chunk->const_datas)[i];
    if (!val) {
        cout << "null val: " << __LINE__ << endl;
        exit(0);
    }
    cout << "const data: " << (static_cast<Number *>(val))->value() << endl;
    push(val);
}

void State::pushl(int i)
{

}

void State::pushu(int i)
{

}

void State::pusht(Value *val)
{

}

FunctionVal * State::get_by_file_name(const char *filename)
{
    if (this->files->count(filename)) return (*this->files)[filename];
    return NULL;
}

FunctionVal * State::get_cur()
{
    return this->cur;
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

    in.read((char *)&localVars, sizeof(int));

    int from_pc = 0, to_pc = 0;
    in.read((char *)&from_pc, sizeof(int));
    in.read((char *)&to_pc, sizeof(int));
    funChunk->from_pc = from_pc;
    funChunk->to_pc = to_pc;

    int nupvalue = 0;
    in.read((char *)&nupvalue, sizeof(int));
    UpValue *envUpv = new UpValue;
    funChunk->add_upvalue(envUpv);
    
    for (int i = 0; i < nupvalue; i++) {
        UpValue *upv = new UpValue;
        name_len = 0;
        in.read((char *)&name_len, sizeof(int));
        for (int j = 0; j < name_len; j++) upv->name.push_back(in.get());
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
        read_function(subFun, in);
        subFun->pre = funChunk;
        subFuns->push_back(subFun);
    }
}

void State::load(const char *file_name)
{
    ifstream in;
    in.open(file_name, ios_base::binary);
    if (!in.good()) {
        cout << "no such file or directory: " << file_name << endl;
        exit(0);
    }
    
    FunctionVal *mainFun = new FunctionVal;
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
            Number *num = new Number;
            double value = 0;
            in.read((char *)&value, sizeof(double));
            num->set_val(value);
            val = num;
        }
        else {
            String *str = new String;
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
    mainChunk->global_vars = new vector<Value *>(gVars, NULL);
    mainChunk->global_vars->reserve(gVars);
    mainChunk->global_var_names_map = new unordered_map<string, int>;
    string t;
    for (int i = 0; i < gVars; i++) {
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

    read_function(mainFun, in);

    (*this->files)[file_name] = mainFun;

    in.close();
}

void State::run()
{
    FunctionVal *entry = get_by_file_name("D:/code/src/vs/yuan-lang/hello.b");
    if (!entry) {
        cout << "not found !!" << endl;
        exit(0);
    }
    cur = entry;
    vm->execute(*entry->get_pcs(), this, 0, NULL);
}