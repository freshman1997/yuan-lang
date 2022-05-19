
#include "vm.h"
#include "stack.h"
#include "state.h"
#include "types.h"
#include "yuan.h"
#include "utils.h"

#include "os_lib.h"
#include "array_lib.h"

/*
    每个作用域都需要包含一个表，用来记录变量，这样子就不用记录变量的名字了，运行再根据取得的值计算
*/
#define isnum(a, b) (a->get_type() == ValueType::t_number && b->get_type() == ValueType::t_number)
#define add(v1, v2) ((static_cast<NumberVal *>(v1))->value() + (static_cast<NumberVal *>(v2))->value())
#define sub(v1, v2) ((static_cast<NumberVal *>(v1))->value() - (static_cast<NumberVal *>(v2))->value())
#define mul(v1, v2) ((static_cast<NumberVal *>(v1))->value() * (static_cast<NumberVal *>(v2))->value())
#define div(v1, v2) ((static_cast<NumberVal *>(v1))->value() / (static_cast<NumberVal *>(v2))->value())
#define mod(v1, v2) (int((static_cast<NumberVal *>(v1))->value()) % int((static_cast<NumberVal *>(v2))->value()))

static State *state = NULL;

void panic(const char *reason) 
{
    cout << "vm was crashed by: " << reason << endl << "\t\tin " << *state->get_cur()->get_file_name() << endl;
    exit(0);
}

static bool is_basic_type(Value *val)
{
    return val->get_type() == ValueType::t_boolean || val->get_type() == ValueType::t_null || val->get_type() == ValueType::t_number;
}

void check_variable_liveness(Value *val)
{
    if (val && val->ref_count <= 0) delete val;
}

static void assign(int type, int i)
{
    FunctionVal *cur = state->get_cur();
    Value *val = state->pop();
    if (type == 0) {
        // 全局
        FunctionVal *global = cur;
        while (global->pre) {
            global = global->pre;
        }
        vector<Value *> *gVars = global->chunk->global_vars;
        if (i < 0 || gVars->size() <= i) {
            panic("fatal error");
        }
        if (gVars->at(i)) {
            gVars->at(i)->ref_count--;
            check_variable_liveness(gVars->at(i));
        }
        // 基础类型应该拷贝
        if (is_basic_type(val)) gVars->at(i) = val->copy();
        else gVars->at(i) = val;
        gVars->at(i)->ref_count++;
    }
    else if (type == 1) {
        // 从局部变量表里面拿，然后赋值
        if (i < 0 || cur->chunk->local_variables->size() <= i) {
            panic("fatal error");
        }
        Value *last = cur->chunk->local_variables->at(i);
        if (last) last->ref_count--;
        if (is_basic_type(val)) cur->chunk->local_variables->at(i) = val->copy();
        else cur->chunk->local_variables->at(i) = val;
        cur->chunk->local_variables->at(i)->ref_count++;
        check_variable_liveness(last);
    }
    else if (type == 2) {
        // upvalue
        if (i < 0 || cur->chunk->upvals->size() <= i) {
            panic("fatal error");
        }
        UpValue *upv = cur->get_upvalue(i);
        if (!upv) panic("no upvalue found error");
        else {
            int stack = upv->in_stack;
            int index = upv->index;
            int funStack = cur->in_stack;
            FunctionVal *tar = cur;
            while (stack < funStack) {
                tar = tar->pre;
                if (!cur) panic("finding upvalue unexpected!!");
                --funStack;
            }
            vector<Value *> *locVars = tar->chunk->local_variables;
            if (locVars->size() <= index) panic("finding upvalue unexpected!!");
            if (locVars->at(index)) {
                locVars->at(index)->ref_count--;
                check_variable_liveness(locVars->at(index));
            }
            if (is_basic_type(val)) locVars->at(index) = val->copy();
            else locVars->at(index) = val;
            locVars->at(index)->ref_count++;
        }
    }
    else panic("unknow operation!");
    check_variable_liveness(val);
}

static void operate(int type, int op, int unaryPush) // type 用于区分是一元还是二元
{
    if (type == 0) {
        Value *val1 = state->pop();
        Value *val2 = state->pop();
        if (val1->get_type() == ValueType::t_null || val2->get_type() == ValueType::t_null) {
            panic("operator must be positive");
        }
        // 入栈是第一个先入，出栈是第二个先出
        if (isnum(val1, val2)) {
            NumberVal *num1 = dynamic_cast<NumberVal *>(val1);
            NumberVal *num2 = dynamic_cast<NumberVal *>(val2);
            switch (op)
            {
            case 0: { 
                NumberVal *ret = new NumberVal;
                ret->set_val(add(val2, val1)); 
                state->push(ret);
                break;
            }
            case 1: {
                NumberVal *ret = new NumberVal;
                ret->set_val(sub(val2, val1)); 
                state->push(ret);
                break;
            }
            case 2: {
                NumberVal *ret = new NumberVal;
                ret->set_val(mul(val2, val1)); 
                state->push(ret);
                break;
            }
            case 3: {
                NumberVal *ret = new NumberVal;
                ret->set_val(div(val2, val1)); 
                state->push(ret);
                break;
            }
            case 4: {
                NumberVal *ret = new NumberVal;
                ret->set_val(mod(val2, val1)); 
                state->push(ret);
                break;
            }
            case 5: {  // |
                NumberVal *ret = new NumberVal;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary or operator needs integer NumberVal!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary or operator needs integer NumberVal!" << endl;
                }
                ret->set_val((int)num2->value() | (int)num1->value());
                state->push(ret);
                break;
            }
            case 6: {  // ^ 
                NumberVal *ret = new NumberVal;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary xor operator needs integer NumberVal!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary xor operator needs integer NumberVal!" << endl;
                }
                ret->set_val((int)num2->value() ^ (int)num1->value());
                state->push(ret);
                break;
            }
            case 7: {  // & 
                NumberVal *ret = new NumberVal;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary and operator needs integer NumberVal!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary and operator needs integer NumberVal!" << endl;
                }
                ret->set_val((int)num2->value() & (int)num1->value());
                state->push(ret);
                break;
            }
            case 9 : { // >> 
                NumberVal *ret = new NumberVal;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary left move operator needs integer NumberVal!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary left move operator needs integer NumberVal!" << endl;
                }
                ret->set_val((int)num2->value() << (int)num1->value());
                state->push(ret);
                break;
            }
            case 10 : { // <<
                NumberVal *ret = new NumberVal;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary right move operator needs integer NumberVal!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary right move operator needs integer NumberVal!" << endl;
                }
                ret->set_val((int)num2->value() >> (int)num1->value());
                state->push(ret);
                break;
            }
            default:
                panic("invalid operator no!");
                break;
            }
        }
        else {
            if (op == 0) {
                if (val1->get_type() == ValueType::t_string && val2->get_type() == ValueType::t_string) {
                    StringVal *str = new StringVal;
                    str->value()->append(*static_cast<StringVal*>(val2)->value()).append(*static_cast<StringVal*>(val1)->value());
                    state->push(str);
                }
                else if (val1->get_type() == ValueType::t_string && val2->get_type() == ValueType::t_number)
                {
                    double v = dynamic_cast<NumberVal *>(val2)->value();
                    int v1 = (int)floor(v);
                    int v2 = (int)ceil(v);
                    StringVal *str = new StringVal;
                    if (v1 == v2) str->value()->append(to_string(v1)).append(*static_cast<StringVal*>(val2)->value());
                    else str->value()->append(to_string(v)).append(*static_cast<StringVal*>(val2)->value());
                    
                    state->push(str);
                }
                else if (val1->get_type() == ValueType::t_number && val2->get_type() == ValueType::t_string)
                {
                    StringVal *str = new StringVal;
                    double v = dynamic_cast<NumberVal *>(val1)->value();
                    int v1 = (int)floor(v);
                    int v2 = (int)ceil(v);
                    if (v1 == v2) str->value()->append(*static_cast<StringVal*>(val2)->value()).append(to_string(v1));
                    else str->value()->append(*static_cast<StringVal*>(val2)->value()).append(to_string(v));
                    state->push(str);
                }
                else panic("add operator not support this type");
            }
            else{
                panic("can not do this operation!");
            }
        }
        check_variable_liveness(val1);
        check_variable_liveness(val2);
    }
    else if (type == 1){
        Value *val = state->pop();
        if (val->get_type() == ValueType::t_null && op != 0) {
            panic("unexpected!");
        }
        switch (op)
        {
        case 0:     // not
        {
            // 0 为 false 
            BooleanVal *b = new BooleanVal;
            if (val->get_type() == ValueType::t_null) {
                b->set(true);
            }
            else if (val->get_type() == ValueType::t_number) {
                NumberVal *num = static_cast<NumberVal *>(val);
                if (num->value() != 0) b->set(false);
                else b->set(true);
            }
            else b->set(false);
            state->push(b);
            break;
        }
        case 1:     // len
        {
            if (is_basic_type(val) || ValueType::t_function == val->get_type()) {
                panic("len operator can only use in sequence data");
            }
            NumberVal *len = new NumberVal;
            switch (val->get_type())
            {
            case ValueType::t_string:
            {
                len->set_val(static_cast<StringVal *>(val)->value()->size());
                break;
            }
            case ValueType::t_array:
            {
                len->set_val(static_cast<ArrayVal *>(val)->member()->size());
                break;
            }
            case ValueType::t_table:
            {
                len->set_val(static_cast<TableVal *>(val)->size());
                break;
            }
            default:
                panic("unexpected exception!");
                break;
            }
            state->push(len);
            break;
        }
        case 2:     // -
        {
            if (val->get_type() != ValueType::t_number) {
                panic("minus operator can not do on not numberic type!");
            }
            NumberVal *num = static_cast<NumberVal *>(val);
            num->set_val(-num->value());
            if (unaryPush) state->push(num);
            break;
        }
        case 3:     // ++
        {
            if (val->get_type() != ValueType::t_number) {
                panic("add add operator can not do on not numberic type!");
            }
            NumberVal *num = static_cast<NumberVal *>(val);
            num->set_val(num->value() + 1);
            if (unaryPush) state->push(num);
            break;
        }
        case 4:     // --
        {
            if (val->get_type() != ValueType::t_number) {
                panic("sub sub operator can not do on not numberic type!");
            }
            NumberVal *num = static_cast<NumberVal *>(val);
            num->set_val(num->value() - 1);
            if (unaryPush) state->push(num);
            break;
        }
        case 5: {
            if (val->get_type() != ValueType::t_number) {
                panic("sub sub operator can not do on not numberic type!");
            }
            NumberVal *num = static_cast<NumberVal *>(val);
            int v1 = (int)num->value();
            int v2 = (int)ceil(num->value());
            if (v1 != v2) {
                cout << "";
            }
            num->set_val(~(int)num->value());
            if (unaryPush) state->push(num);
            break;
        }
        default:
            panic("unexpected exception!");
            break;
        }
    }
}

static void compair(OpCode op)
{
    Value *rhs = state->pop();
    Value *lhs = state->pop();
    BooleanVal *b = new BooleanVal;
    if (lhs->get_type() == ValueType::t_number && rhs->get_type() == ValueType::t_number) {
        switch (op)
        {
        case OpCode::op_gt:
            b->set(static_cast<NumberVal *>(lhs)->value() > static_cast<NumberVal *>(rhs)->value());
            break;
        case OpCode::op_lt:
            b->set(static_cast<NumberVal *>(lhs)->value() < static_cast<NumberVal *>(rhs)->value());
            break;
        case OpCode::op_gt_eq:
            b->set(static_cast<NumberVal *>(lhs)->value() >= static_cast<NumberVal *>(rhs)->value());
            break;
        case OpCode::op_lt_eq:
            b->set(static_cast<NumberVal *>(lhs)->value() <= static_cast<NumberVal *>(rhs)->value());
            break;
        case OpCode::op_equal:
            b->set(static_cast<NumberVal *>(lhs)->value() == static_cast<NumberVal *>(rhs)->value());
            break;
        case OpCode::op_not_equal:
            b->set(static_cast<NumberVal *>(lhs)->value() != static_cast<NumberVal *>(rhs)->value());
            break;
        default:
            panic("not support this compair type!");
            break;
        }
    }
    else if (lhs->get_type() == ValueType::t_string && rhs->get_type() == ValueType::t_string){
        // 字符串对比
        StringVal *str1 = dynamic_cast<StringVal*>(lhs);
        StringVal *str2 = dynamic_cast<StringVal*>(rhs);
        if (op == OpCode::op_equal) b->set(*str1->value() == *str2->value());
        else {
            cout << "warning: string operator only support ==" << endl;
        }
    }
    else if (lhs->get_type() == ValueType::t_boolean && rhs->get_type() == ValueType::t_boolean){
        if (op == OpCode::op_and) b->set(static_cast<BooleanVal *>(lhs)->value() && static_cast<BooleanVal *>(rhs)->value());
        else if (op == OpCode::op_or) b->set(static_cast<BooleanVal *>(lhs)->value() || static_cast<BooleanVal *>(rhs)->value());
        else panic("BooleanVal not support such operator!");
    }
    else {
        panic("can not compair by type array or table!");
    }
    state->push(b);
    check_variable_liveness(lhs);
    check_variable_liveness(rhs);
}

static void packVarargs(State *st, FunctionVal *fun, ArrayVal *args)
{
    int nparam = fun->nparam;
    int stSz = st->get_stack_size();
    int paramStart = st->param_start;
    int amount = stSz - nparam - paramStart + 1;
    if (args) amount--;
    if (amount < 0) {
        panic("vm internal error!!!");  // should not happen
    }
    if (fun->nreturn && !args) fun->param_stack++; 
    ArrayVal *arr = args;
    if (!args) {
        arr = new ArrayVal;
        fun->param_stack = st->get_stack_size();
        fun->nparam = fun->param_stack;
    }
    vector<Value *> *members = arr->member();
    members->resize(amount);
    while (amount--) {
        members->at(amount) = state->pop();
    }
    if (!args) state->push(arr);
    st->param_start = 0;
}
/*
    如，print就会找这里，
    os.open(file, mod)  这里首先会找 os 这个变量，如果已经被声明则会使用已声明的，否则使用环境中的
    二者冲突可以 用别名取代， 如 local env = _ENV(os)
*/
static Value * find_env_param(StringVal *key, FunctionVal *cur)
{
    if (!cur->chunk->upvals || cur->chunk->upvals->empty()) return NULL;
    TableVal *envTb = dynamic_cast<TableVal *>(cur->chunk->upvals->at(0)->val);
    if (!envTb) return NULL;
    for (auto &it : *envTb->members()) {
        Value *k = it.second.first;
        if (k->get_type() == ValueType::t_string && static_cast<StringVal *>(k)->hash() == key->hash()) {
            return it.second.second;
        }
    }
    return NULL;
}

static Value * fetch_file_global_variables(StringVal *file, int i)
{
    FunctionVal *fileChunk = state->get_by_file_name(file->value()->c_str());
    if (!fileChunk) {
        panic("no module found!");
    }
    Value *val = fileChunk->get_global_var(i);
    if (!val) {
        panic("no such varibale!");
    }
    return val;
}

static void do_execute(const std::vector<int> &pcs, int from, int to);

static void do_call(Value *val)
{
    FunctionVal *fun = static_cast<FunctionVal *>(val);
    state->set_cur(fun);
    // 参数前面应该已经入栈了
    FunctionVal *file_main_fun = state->get_by_file_name(fun->get_file_name()->c_str());
    if (!file_main_fun) {
        panic("load file fail");
    }
    if (fun->from_pc < 0 || fun->to_pc > file_main_fun->to_pc) {
        panic("fatal error, invalid instructions");
    }
    fun->param_stack = state->get_stack_size() - fun->nparam;
    fun->ncalls++;
    bool clearRet = state->doClear;
    if (fun->ncalls >= MAX_RECURSE_NUM) {
        panic("stack overflow");
    }
    if (fun->chunk->is_varags) {
        packVarargs(state, fun, NULL);
    }
    // 执行完，栈中应该有对应的返回值
    do_execute(*file_main_fun->chunk->fun_body_ops, fun->from_pc, fun->to_pc);
    fun->ncalls--;
    state->end_call(clearRet);
}

static void do_execute(const std::vector<int> &pcs, int from, int to)
{
    for (int i = from; i < to; ++i) {
        int it = pcs[i];
        int param = (it >> 8);
        OpCode op = (OpCode)((it << 24) >> 24);
        switch (op)
        {
        case OpCode::op_add: 
        case OpCode::op_sub: 
        case OpCode::op_mul: 
        case OpCode::op_div: 
        case OpCode::op_mod: 
         /* 逻辑运算符 */
        case OpCode::op_bin_or:
        case OpCode::op_bin_xor:
        case OpCode::op_bin_and:
        case OpCode::op_bin_lm:
        case OpCode::op_bin_rm:
        {
            operate(0, int(op), 0);
            break;
        }
        
        /* 比较 */
        case OpCode::op_equal: {
            compair(op);
            break;
        }
        case OpCode::op_not_equal: {
            compair(op);
            break;
        }
        case OpCode::op_gt: {
            compair(op);
            break;
        }
        case OpCode::op_lt: {
            compair(op);
            break;
        }
        case OpCode::op_gt_eq: {
            compair(op);
            break;
        }
        case OpCode::op_lt_eq: {
            compair(op);
            break;
        }
        case OpCode::op_or: {
            compair(op);
            break;
        }
        case OpCode::op_and: {
            compair(op);
            break;
        }

        /* 一元运算符  从栈中弹出一个 */
        case OpCode::op_not: {
            operate(1, 0, param);
            break;
        }
        case OpCode::op_len: {
            operate(1, 1, param);
            break;
        }
        case OpCode::op_unary_sub: {
            operate(1, 2, param);
            break;
        }
        case OpCode::op_add_add: {
            operate(1, 3, param);
            break;
        }
        case OpCode::op_sub_sub: {
            operate(1, 4, param);
            break;
        }
        case OpCode::op_bin_not:
        {
            operate(1, 5, param);
            break;
        }


        /*    入栈相关   */
        case OpCode::op_pushc:{
            state->pushc(param);
            break;
        }
        case OpCode::op_pushg:{
            state->pushg(param);
            break;
        }
        case OpCode::op_pushl:{
            state->pushl(param);
            break;
        }
        case OpCode::op_pushu:{
            state->pushu(param + 1);
            break;
        }

        
        case OpCode::op_storeg:
            assign(0, param);
            break;
        case OpCode::op_storel:
            assign(1, param);
            break;
        case OpCode::op_storeu:
            assign(2, param + 1);
            break;
        
        // 在环境中找
        case OpCode::op_get_env:
        {
            Value *val = state->pop();
            if (val->get_type() != ValueType::t_string) {
                panic("envirenment param only support string key!!");
            }
            StringVal *key = dynamic_cast<StringVal *>(val);
            if (!key) {
                panic("internal fatal error");
            }
            Value *envParam = find_env_param(key, state->get_cur());
            if (!envParam) {
                panic("no such envirenment parameter!!");
            }
            state->push(envParam);
            break;
        }
        
        case OpCode::op_test:
        {
            Value *cond = state->pop();
            if (cond->get_type() == ValueType::t_number) {
                if (static_cast<NumberVal *>(cond)->value())
                    ++i;   // to jump op
            }
            else if (cond->get_type() == ValueType::t_boolean) {
                if (static_cast<BooleanVal *>(cond)->value()) 
                    ++i;   // to jump op 
            }
            else if (cond->get_type() != ValueType::t_null) {
                ++i;
            }
            check_variable_liveness(cond);
            break;
        }
        
        case OpCode::op_jump:
            if (param >= to || param < from) {
                panic("can not jump out of the border!");
            }
            //cout << "do jump cur: " << i << ", target: " << param << endl;
            i = param;
            break;


        case OpCode::op_array_new:
        {
            ArrayVal *arr = new ArrayVal();
            state->push(arr);
            break;
        }
        case OpCode::op_array_set:
        {
            Value *item = state->pop();
            ArrayVal *arr = dynamic_cast<ArrayVal *>(state->pop());
            if (!arr) {
                panic("array init fail");
            }
            arr->add_item(item);
            state->push(arr);
            break;
        }
        case OpCode::op_table_new:
        {
            state->push(new TableVal);
            break;
        }
        case OpCode::op_table_set:
        {
            Value *key = state->pop();
            Value *val = state->pop();
            Value * p = state->pop();
            TableVal *tb = dynamic_cast<TableVal *>(p);
            if (!tb) {
                panic("table init fail");
            }
            bool ret = tb->set(key, val);
            if (!ret) {
                panic("attempt to set table fail");
            }
            state->push(tb);
            break;
        }

        case OpCode::op_getfield:
        {
            if (param < 0) break;
            Value *key = state->pop();
            Value *val = state->pop();
            if (val->get_type() == ValueType::t_string) {
                if (key->get_type() != ValueType::t_number) {
                    panic("indexing string variable error");
                }
                NumberVal *num = dynamic_cast<NumberVal *>(key);
                if (!num) {
                    panic("no numberic variable found");
                }
                int v1 = (int)num->value();
                int v2 = (int)ceil(num->value());
                if (v1 != v2) {
                    panic("only interger can index float variable!");
                }
                StringVal *str = dynamic_cast<StringVal *>(val);
                if (!str) {
                    panic("not a string variable!");
                }
                Value *idata = str->get(v1);
                if (!idata) {
                    panic("indexing overflow");
                }
                state->push(idata);
            }
            else if (val->get_type() == ValueType::t_table) {
                if (!(key->get_type() == ValueType::t_number || key->get_type() == ValueType::t_string)) {
                    panic("indexing table variable error");
                }
                TableVal *tb = dynamic_cast<TableVal *>(val);
                if (!tb) {
                    panic("not a table variable!");
                }
                Value *v = tb->get(key);
                if (!v) {
                    panic("table does not exits such data");
                }
                state->push(v);
            }
            else if (val->get_type() == ValueType::t_array){
                if (key->get_type() != ValueType::t_number) {
                    panic("indexing array variable error");
                }
                NumberVal *num = dynamic_cast<NumberVal *>(key);
                if (!num) {
                    panic("not a numberic variable!");
                }
                ArrayVal *arr = dynamic_cast<ArrayVal *>(val);
                if (!arr) {
                    panic("not a array variable!");
                }
                int v1 = (int)num->value();
                int v2 = (int)ceil(num->value());
                if (v1 != v2) {
                    v1 = v2;
                    //panic("only interger can index array variable!");
                }
                if (v1 < 0 || v1 >= arr->size()) {
                    panic("indexing out of array!");
                }
                Value *v = arr->get(v1);
                state->push(v);
            }
            else panic("can not index such data!!!");
            break;
        }
        
        
        // call
        case OpCode::op_enter_func:
        {
            Value *subfun = state->get_subfun(param);
            if (!subfun || subfun->get_type() != ValueType::t_function) {
                panic("init subfunction fail");
            }
            state->push(subfun);
            break;
        }
        case OpCode::op_param_start: 
        {
            state->doClear = param ? true : false;
            state->param_start = state->get_stack_size();
            break;
        }
        case OpCode::op_call:
        {
            // 负的是全局函数，否则是local函数
            Value *val = NULL;
            if (param < 0) {
                param = -param - 1;
                val = state->getg(param);
            }
            else val = state->getl(param);

            if (!val || val->get_type() != ValueType::t_function) {
                panic("no function found");
            }
            do_call(val);
            break;
        }
        case OpCode::op_call_upv:
        {
            // 负的是从全局的常量拿到键，再去环境中找，正的直接就是 upvalue 
            if (param < 0) {
                StringVal *key = dynamic_cast<StringVal*>(state->getc(-param - 1));
                if (!key) 
                    panic("unexpected!!!");
                Value *v = find_env_param(key, state->get_cur());
                if (!v || v->get_type() != ValueType::t_function) {
                    panic("not found function!!!");
                }
                FunctionVal *cfun = static_cast<FunctionVal *>(v);
                if (!cfun->isC) {
                    panic("not C function!!!");
                }
                cfun->ncalls++;
                if (cfun->ncalls >= MAX_RECURSE_NUM) {
                    panic("stack overflow");
                }
                state->set_cur(cfun);
                cfun->cfun(state);  // call c function
                state->end_call(false);
            }
            else {
                Value *val = state->getu(param + 1);
                if (!val || val->get_type() != ValueType::t_function) {
                    panic("not a function upvalue");
                }
                do_call(val->copy());
            }
            break;
        }
        case OpCode::op_call_t:     // 直接调用入栈的函数
        {
            Value *val = state->get(param);
            if (!val || val->get_type() != ValueType::t_function) {
                panic("no function found");
            }
            FunctionVal *fun = dynamic_cast<FunctionVal *>(val);
            if (!fun || fun->nparam != -param) {
                if (!fun->chunk->is_varags)
                    panic("call error, function parameter amount not match!!!");
                else {
                    if (fun->nparam > -param) {
                        panic("call error, function parameter amount not match!!!");
                    }
                }
            }
            if (fun->chunk->is_varags) {
                int nvarargs = -param - fun->nparam - 1; // 可变参数数量
                if (nvarargs < 0) {
                    panic("vm internal error!!!");  // should not happen
                }
                ArrayVal *arr = new ArrayVal;
                while (nvarargs) {
                    arr->add_item(state->pop());
                }
                state->push(arr);
            }
            if (!fun->isC) {
                do_call(val);
                state->pop();   // 把栈里面的函数出栈
            }
            else {
                fun->ncalls++;
                if (fun->ncalls >= MAX_RECURSE_NUM) {
                    panic("stack overflow");
                }
                state->set_cur(fun);
                fun->cfun(state);  // call c function
                state->end_call(false);

                // 有返回值？
                if (fun->nreturn) {
                    Value *ret = state->pop();
                    state->pop();
                    state->push(ret);
                }
                else state->pop();
            }
            break;
        }

        case OpCode::op_return:
        {
            return;  // do nothing
        }

        case OpCode::op_load_bool:
        {
            BooleanVal *b = new BooleanVal;
            if (param) b->set(true);
            else b->set(false);
            state->push(b);
            break;
        }
        case OpCode::op_load_nil:
        {
            state->push(new NilVal);
            break;
        }

        case OpCode::op_for_in: 
        {
            Value *con = state->pop();  // 容器
            Value *iter = state->pop();
            if (iter->get_type() == ValueType::t_null || con->get_type() == ValueType::t_null) {
                panic("invalid for in loop, no table or array found");
            }

            NumberVal *it = NULL;
            if (iter->get_type() == ValueType::t_boolean) {
                delete iter;
                NumberVal *num = new NumberVal;
                num->set_val(0);
                it = num;
            }
            else {
                if (iter->get_type() != ValueType::t_number) {
                    panic("internal for in loop error!!!");
                }
                it = static_cast<NumberVal *>(iter);
                it->set_val(it->value() + 1);
            }

            if (!(param == 1 || param == 2) || !(con->get_type() == ValueType::t_array || con->get_type() == ValueType::t_table)) {
                panic("for int loop just allow 1 param in array and 2 param in table");
            }

            if ((param == 2 && con->get_type() != ValueType::t_table) || (param == 1 && con->get_type() != ValueType::t_array)) {
                panic("for int loop just allow 1 param in array and 2 param in table");
            }

            if (param == 1) {
                ArrayVal *arr = static_cast<ArrayVal *>(con);
                if (it->value() < arr->size()) {
                    ++i; 
                    state->push(it);
                    state->push(arr->get((int)it->value()));
                    break;
                }
                else {
                    // jump out
                    delete it;
                }
            }
            else {
                TableVal *tb = static_cast<TableVal *>(con);
                if (it->value() < tb->size()) {
                    ++i;
                    state->push(it);
                    unordered_map<int, std::pair<Value *, Value *>>::iterator cit = tb->get_iter((int)it->value());
                    state->push(cit->second.second);
                    state->push(cit->second.first);
                    break;
                }
                else {
                    // jump out
                    delete it;
                }
            }
            break;
        }
        case OpCode::op_setfield:
        {
            Value *val = state->pop();
            Value *key = state->pop();
            Value *con = state->pop();
            if (!(con->get_type() == ValueType::t_array || con->get_type() == ValueType::t_table || con->get_type() == ValueType::t_string)) {
                panic("can not set container field because it is not a caontainer!");
            }

            if (con->get_type() == ValueType::t_array) {
                if (key->get_type() != ValueType::t_number) {
                    panic("only integer value can index array!");
                }
                ArrayVal *arr = static_cast<ArrayVal*>(con);
                NumberVal *num = static_cast<NumberVal *>(key);
                int v1 = (int)num->value();
                int v2 = (int)ceil(num->value());
                if (v1 != v2) {
                    panic("only interger can index array variable!");
                }
                if (v1 < 0 || v1 >= arr->size()) {
                    panic("indexing out of array!");
                }
                val->ref_count++;
                arr->set(v1, val);
            }
            else if (con->get_type() == ValueType::t_table) {
                if (!(key->get_type() == ValueType::t_number || key->get_type() == ValueType::t_string)) {
                    panic("only integer and string value can index table!");
                }
                TableVal *tb = static_cast<TableVal *>(con);
                tb->set(key, val);
            }
            else {
                if (key->get_type() != ValueType::t_number) {
                    panic("only integer value can index string!");
                }
                if (val->get_type() != ValueType::t_byte) {
                    panic("string value only accept byte data");
                }
                StringVal *str = static_cast<StringVal *>(con);
                NumberVal *num = static_cast<NumberVal *>(key);
                int v1 = (int)num->value();
                int v2 = (int)ceil(num->value());
                if (v1 != v2) {
                    panic("only interger can index string variable!");
                }
                if (v1 < 0 || v1 >= str->size()) {
                    panic("indexing out of string!");
                }
                str->set(v1, val);
            }

            // 其他类型只是指针赋值，这意味着左边的被赋值后可以修改右边的内容
            //check_variable_liveness(key);
            break;
        }

        default:
            panic("unsupport operation !!!");
            break;
        }
    }
}

static int print(State* st)
{
    Value *val = st->pop();
    switch (val->get_type())
    {
    case ValueType::t_string:
        cout << *static_cast<StringVal *>(val)->value() << endl;
        break;
    case ValueType::t_number:
        cout << static_cast<NumberVal *>(val)->value() << endl;
        break;
    case ValueType::t_boolean:
    {
        BooleanVal *b = static_cast<BooleanVal *>(val);
        cout << (b->value() ? "true" : "false") << endl;
        break;
    }
    case ValueType::t_null:
        cout << "nil" << endl;
        break;
    case ValueType::t_array:
    {
        ArrayVal *arr = static_cast<ArrayVal *>(val);
        cout << "array@" << arr << endl;
        break;
    }
    case ValueType::t_table:
    {
        TableVal *tb = static_cast<TableVal *>(val);
        cout << "table@" << tb << endl;
        break;
    }
    case ValueType::t_function:
    {
        FunctionVal *func = static_cast<FunctionVal *>(val);
        cout << "function@" << func << endl;
        break;
    }
    case ValueType::t_byte:
    {
        ByteVal *b = static_cast<ByteVal *>(val);
        cout << b->_val << endl;
        break;
    }
    default:
        panic("unexpected!");
        break;
    }
    check_variable_liveness(val);
    return 1;
}

static int require(State* st)
{
    ArrayVal *args = new ArrayVal;
    packVarargs(state, state->get_cur(), args);
    StringVal *path = dynamic_cast<StringVal*>(st->pop());
    if (!path) {
        panic("no correct key found!");
    }
    string filepath = getcwd() + "/" + *path->value() + ".y";
    
    bool ret = st->require(filepath.c_str(), args);
    if (!ret) {
        panic("require module fail, please check the path is correct or not");
    }
    return 1;
}

void VM::load_lib(TableVal *tb)
{
    StringVal *key = new StringVal;
    key->set_val("print");
    FunctionVal *p = new FunctionVal;
    p->nreturn = 0;
    p->nparam = 1;
    p->isC = true;
    p->cfun = print;
    tb->set(key, p);

    StringVal *req = new StringVal;
    req->set_val("require");
    FunctionVal *r = new FunctionVal;
    r->nreturn = 0;
    r->nparam = 1;
    r->isC = true;
    r->cfun = require;
    tb->set(req, r);

    load_os_lib(tb);
    load_array_lib(tb);
}

void VM::execute(const std::vector<int> &pcs, State *_state, int argc, char **argv)
{
    state = _state;
    do_execute(pcs, 0, pcs.size());
}
