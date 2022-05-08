
#include "vm.h"
#include "stack.h"
#include "state.h"
#include "types.h"
#include "yuan.h"

/*
    每个作用域都需要包含一个表，用来记录变量，这样子就不用记录变量的名字了，运行再根据取得的值计算
*/
#define isnum(a, b) (a->get_type() == ValueType::t_number && b->get_type() == ValueType::t_number)
#define add(v1, v2) ((static_cast<Number *>(v1))->value() + (static_cast<Number *>(v2))->value())
#define sub(v1, v2) ((static_cast<Number *>(v1))->value() - (static_cast<Number *>(v2))->value())
#define mul(v1, v2) ((static_cast<Number *>(v1))->value() * (static_cast<Number *>(v2))->value())
#define div(v1, v2) ((static_cast<Number *>(v1))->value() / (static_cast<Number *>(v2))->value())
#define mod(v1, v2) (int((static_cast<Number *>(v1))->value()) % int((static_cast<Number *>(v2))->value()))

static State *state = NULL;

static void panic(const char *reason) 
{
    cout << "vm was crashed by: " << reason << endl;
    exit(0);
}

static bool is_basic_type(Value *val)
{
    return val->get_type() == ValueType::t_boolean || val->get_type() == ValueType::t_null || val->get_type() == ValueType::t_number;
}

static void check_variable_liveness(Value *val)
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
            Number *num1 = dynamic_cast<Number *>(val1);
            Number *num2 = dynamic_cast<Number *>(val2);
            switch (op)
            {
            case 0: { 
                Number *ret = new Number;
                ret->set_val(add(val2, val1)); 
                state->push(ret);
                break;
            }
            case 1: {
                Number *ret = new Number;
                ret->set_val(sub(val2, val1)); 
                state->push(ret);
                break;
            }
            case 2: {
                Number *ret = new Number;
                ret->set_val(mul(val2, val1)); 
                state->push(ret);
                break;
            }
            case 3: {
                Number *ret = new Number;
                ret->set_val(div(val2, val1)); 
                state->push(ret);
                break;
            }
            case 4: {
                Number *ret = new Number;
                ret->set_val(mod(val2, val1)); 
                state->push(ret);
                break;
            }
            case 5: {  // |
                Number *ret = new Number;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary or operator needs integer number!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary or operator needs integer number!" << endl;
                }
                ret->set_val((int)num2->value() | (int)num1->value());
                state->push(ret);
                break;
            }
            case 6: {  // ^ 
                Number *ret = new Number;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary xor operator needs integer number!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary xor operator needs integer number!" << endl;
                }
                ret->set_val((int)num2->value() ^ (int)num1->value());
                state->push(ret);
                break;
            }
            case 7: {  // & 
                Number *ret = new Number;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary and operator needs integer number!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary and operator needs integer number!" << endl;
                }
                ret->set_val((int)num2->value() & (int)num1->value());
                state->push(ret);
                break;
            }
            case 9 : { // >> 
                Number *ret = new Number;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary left move operator needs integer number!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary left move operator needs integer number!" << endl;
                }
                ret->set_val((int)num2->value() << (int)num1->value());
                state->push(ret);
                break;
            }
            case 10 : { // <<
                Number *ret = new Number;
                int v1 = (int)num1->value();
                int v2 = (int)ceil(num1->value());
                if (v1 != v2) {
                    cout << "warning: binary right move operator needs integer number!" << endl;
                }
                v1 = (int)num2->value();
                v2 = (int)ceil(num2->value());
                if (v1 != v2) {
                    cout << "warning: binary right move operator needs integer number!" << endl;
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
                    String *str = new String;
                    str->value()->append(*static_cast<String*>(val2)->value()).append(*static_cast<String*>(val1)->value());
                    state->push(str);
                }
                else if (val1->get_type() == ValueType::t_string && val2->get_type() == ValueType::t_number)
                {
                    double v = dynamic_cast<Number *>(val2)->value();
                    int v1 = (int)floor(v);
                    int v2 = (int)ceil(v);
                    String *str = new String;
                    if (v1 == v2) str->value()->append(to_string(v1)).append(*static_cast<String*>(val2)->value());
                    else str->value()->append(to_string(v)).append(*static_cast<String*>(val2)->value());
                    
                    state->push(str);
                }
                else if (val1->get_type() == ValueType::t_number && val2->get_type() == ValueType::t_string)
                {
                    String *str = new String;
                    double v = dynamic_cast<Number *>(val1)->value();
                    int v1 = (int)floor(v);
                    int v2 = (int)ceil(v);
                    if (v1 == v2) str->value()->append(*static_cast<String*>(val2)->value()).append(to_string(v1));
                    else str->value()->append(*static_cast<String*>(val2)->value()).append(to_string(v));
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
            Boolean *b = new Boolean;
            if (val->get_type() == ValueType::t_null) {
                b->set(true);
            }
            else if (val->get_type() == ValueType::t_number) {
                Number *num = static_cast<Number *>(val);
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
            Number *len = new Number;
            switch (val->get_type())
            {
            case ValueType::t_string:
            {
                len->set_val(static_cast<String *>(val)->value()->size());
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
            Number *num = static_cast<Number *>(val);
            num->set_val(-num->value());
            if (unaryPush) state->push(num);
            break;
        }
        case 3:     // ++
        {
            if (val->get_type() != ValueType::t_number) {
                panic("add add operator can not do on not numberic type!");
            }
            Number *num = static_cast<Number *>(val);
            num->set_val(num->value() + 1);
            if (unaryPush) state->push(num);
            break;
        }
        case 4:     // --
        {
            if (val->get_type() != ValueType::t_number) {
                panic("sub sub operator can not do on not numberic type!");
            }
            Number *num = static_cast<Number *>(val);
            num->set_val(num->value() - 1);
            if (unaryPush) state->push(num);
            break;
        }
        case 5: {
            if (val->get_type() != ValueType::t_number) {
                panic("sub sub operator can not do on not numberic type!");
            }
            Number *num = static_cast<Number *>(val);
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
    Boolean *b = new Boolean;
    if (lhs->get_type() == ValueType::t_number && rhs->get_type() == ValueType::t_number) {
        switch (op)
        {
        case OpCode::op_gt:
            b->set(static_cast<Number *>(lhs)->value() > static_cast<Number *>(rhs)->value());
            break;
        case OpCode::op_lt:
            b->set(static_cast<Number *>(lhs)->value() < static_cast<Number *>(rhs)->value());
            break;
        case OpCode::op_gt_eq:
            b->set(static_cast<Number *>(lhs)->value() >= static_cast<Number *>(rhs)->value());
            break;
        case OpCode::op_lt_eq:
            b->set(static_cast<Number *>(lhs)->value() <= static_cast<Number *>(rhs)->value());
            break;
        case OpCode::op_equal:
            b->set(static_cast<Number *>(lhs)->value() == static_cast<Number *>(rhs)->value());
            break;
        case OpCode::op_not_equal:
            b->set(static_cast<Number *>(lhs)->value() != static_cast<Number *>(rhs)->value());
            break;
        default:
            panic("not support this compair type!");
            break;
        }
    }
    else if (lhs->get_type() == ValueType::t_string && rhs->get_type() == ValueType::t_string){
        // 字符串对比
        String *str1 = dynamic_cast<String*>(lhs);
        String *str2 = dynamic_cast<String*>(rhs);
        if (op == OpCode::op_equal) b->set(*str1->value() == *str2->value());
        else {
            cout << "warning: string operator only support ==" << endl;
        }
    }
    else if (lhs->get_type() == ValueType::t_boolean && rhs->get_type() == ValueType::t_boolean){
        if (op == OpCode::op_and) b->set(static_cast<Boolean *>(lhs)->value() && static_cast<Boolean *>(rhs)->value());
        else if (op == OpCode::op_or) b->set(static_cast<Boolean *>(lhs)->value() || static_cast<Boolean *>(rhs)->value());
        else panic("boolean not support such operator!");
    }
    else {
        panic("can not compair by type array or table!");
    }
    state->push(b);
    check_variable_liveness(lhs);
    check_variable_liveness(rhs);
}

static void string_concat()
{
    Value *val1 = state->pop();
    Value *val2 = state->pop();
    // 右边的链接到左边

}

static void packVarargs(State *st, FunctionVal *fun)
{
    int nparam = fun->nparam;
    int stSz = st->get_stack_size();
    int paramStart = st->param_start;
    int amount = stSz - nparam - paramStart + 1;
    if (amount < 0) {
        panic("vm internal error!!!");  // should not happen
    }
    if (fun->nreturn) fun->param_stack++; 
    ArrayVal *arr = new ArrayVal;
    vector<Value *> *members = arr->member();
    members->resize(amount);
    while (amount--) {
        members->at(amount) = state->pop();
    }
    state->push(arr);
    st->param_start = 0;
}

static Value * find_env_param(String *key, FunctionVal *cur)
{
    if (!cur->chunk->upvals || cur->chunk->upvals->empty()) return NULL;
    TableVal *envTb = dynamic_cast<TableVal *>(cur->chunk->upvals->at(0)->val);
    if (!envTb) return NULL;
    for (auto &it : *envTb->members()) {
        Value *k = it.second.first;
        if (k->get_type() == ValueType::t_string && static_cast<String *>(k)->hash() == key->hash()) {
            return it.second.second;
        }
    }
    return NULL;
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
    if (fun->ncalls >= MAX_RECURSE_NUM) {
        panic("stack overflow");
    }
    if (fun->chunk->is_varags) {
        packVarargs(state, fun);
    }
    // 执行完，栈中应该有对应的返回值
    do_execute(*file_main_fun->chunk->fun_body_ops, fun->from_pc, fun->to_pc);
    fun->ncalls--;
    state->end_call();
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
            String *key = dynamic_cast<String *>(val);
            cout << "key: " << *key->value() << endl;
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
                if (static_cast<Number *>(cond)->value())
                    ++i;   // to jump op
                
                check_variable_liveness(cond);
                break;
            }
            if (cond->get_type() != ValueType::t_boolean) {
                panic("no boolean variable found");
            }
            if (static_cast<Boolean *>(cond)->value()) {
                ++i;   // to jump op 
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

        case OpCode::op_index:
        {
            Value *key = state->pop();
            Value *val = state->pop();
            if (val->get_type() == ValueType::t_string) {
                if (key->get_type() != ValueType::t_number) {
                    panic("indexing string variable error");
                }
                Number *num = dynamic_cast<Number *>(key);
                if (!num) {
                    panic("no numberic variable found");
                }
                int v1 = (int)num->value();
                int v2 = (int)ceil(num->value());
                if (v1 != v2) {
                    panic("only interger can index string variable!");
                }
                String *str = dynamic_cast<String *>(val);
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
                Number *num = dynamic_cast<Number *>(key);
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
                    panic("only interger can index string variable!");
                }
                Value *v = arr->get(v1);
                if(!v) {
                    panic("indexing overflow!");
                }
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
                String *key = dynamic_cast<String*>(state->getc(-param - 1));
                if (!key) panic("unexpected!!!");
                Value *v = find_env_param(key, state->get_cur());
                if (!v) {
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
                cfun->cfun(state);  // call c function
                cfun->ncalls--;
            }
            else {
                Value *val = state->getu(param + 1);
                if (!val || val->get_type() != ValueType::t_function) {
                    panic("not a function upvalue");
                }
                do_call(val);
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
            do_call(val);
            state->pop();   // 把栈里面的函数出栈
            break;
        }

        case OpCode::op_return:
        {
            return;  // do nothing
        }

        case OpCode::op_load_bool:
        {
            Boolean *b = new Boolean;
            if (param) b->set(true);
            else b->set(false);
            state->push(b);
            break;
        }
        case ::OpCode::op_load_nil:
        {
            state->push(new Nil);
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
    if (val->get_type() == ValueType::t_string) {
        cout << *static_cast<String *>(val)->value() << endl;
    }
    else if (val->get_type() == ValueType::t_number) {
        cout << static_cast<Number *>(val)->value() << endl;
    }
    check_variable_liveness(val);
    return 0;
}

void VM::load_lib(TableVal *tb)
{
    String *key = new String;
    key->set_val("print");
    FunctionVal *p = new FunctionVal;
    p->isC = true;
    p->cfun = print;
    tb->set(key, p);
}

void VM::execute(const std::vector<int> &pcs, State *_state, int argc, char **argv)
{
    state = _state;
    do_execute(pcs, 0, pcs.size());
}
