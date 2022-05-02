
#include "vm.h"
#include "stack.h"
#include "state.h"
#include "types.h"

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
    if (val->ref_count <= 0) delete val;
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
        if (gVars->size() <= i) {
            panic("fatal error");
        }
        if (gVars->at(i)) {
            if (!is_basic_type(gVars->at(i))) {
                if (gVars->at(i) && gVars->at(i)->ref_count == 1) delete gVars->at(i);
                gVars->at(i) = val;
            }
            else {
                // 基础类型应该拷贝
                if (val->get_type() == ValueType::t_number) {
                    Number *num = new Number;
                    num->set_val(static_cast<Number *>(val)->value());
                    gVars->at(i) = num;
                }
            }
        }
        else {
            gVars->at(i) = val;
        }
    }
    else if (type == 1) {
        // 从局部变量表里面拿，然后赋值

    }
    else if (type == 2) {
        // upvalue
        
    }
    else panic("unknow operation!");
}

static void operate(int type, int op) // type 用于区分是一元还是二元
{
    if (type == 0) {
        static vector<int> opers;
        if (opers.empty()) {
            // + - * / % && || < > <= >= == != >> << | & ! ~ 
            for(int i = 0; i < 20; ++i) {
                opers.push_back(i);
            }
        }
        Value *val1 = state->pop();
        Value *val2 = state->pop();
        // 入栈是第一个先入，出栈是第二个先出
        if (isnum(val1, val2)) {
            Number *ret = new Number;
            switch (op)
            {
            case 0: ret->set_val(add(val2, val1)); break;
            case 1: ret->set_val(sub(val2, val1)); break;
            case 2: ret->set_val(mul(val2, val1)); break;
            case 3: ret->set_val(div(val2, val1)); break;
            case 4: ret->set_val(mod(val2, val1)); break;
            default:
                break;
            }
            state->push(ret);
            // free 
        }
        else {
            panic("can not do this operation!");
        }
    }
    else if (type == 1){
        Value *val = state->pop();
        if (!val) {
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
            else panic("can not do this operation!");
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
                len->set_val(static_cast<Array *>(val)->member()->size());
                break;
            }
            case ValueType::t_table:
            {
                len->set_val(static_cast<Table *>(val)->size());
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
            break;
        }
        case 3:     // ++
        {
            if (val->get_type() != ValueType::t_number) {
                panic("minus operator can not do on not numberic type!");
            }
            Number *num = static_cast<Number *>(val);
            num->set_val(num->value() + 1);
            break;
        }
        case 4:     // --
        {
            if (val->get_type() != ValueType::t_number) {
                panic("minus operator can not do on not numberic type!");
            }
            Number *num = static_cast<Number *>(val);
            num->set_val(num->value() - 1);
            cout << "val: " << num->value() << endl;
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
    if (!rhs || !lhs) {
        panic("compair operator must be positive");
    }
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
            panic("no number found!");
            break;
        }
    }
    else if (lhs->get_type() == ValueType::t_string && rhs->get_type() == ValueType::t_string){

    }
    else if (lhs->get_type() == ValueType::t_boolean && rhs->get_type() == ValueType::t_boolean){
        if ((OpCode)op == OpCode::op_and) b->set(static_cast<Boolean *>(lhs)->value() && static_cast<Boolean *>(rhs)->value());
        else if ((OpCode)op == OpCode::op_or) b->set(static_cast<Boolean *>(lhs)->value() || static_cast<Boolean *>(rhs)->value());
        else panic("boolean not support such operator!");
    }
    else {
        panic("can not compair by type array or table!");
    }
    state->push(b);
}

static void open_func()
{
    // 初始化 局部变量表和 upvalue 表 ref + 1
    // 保存现场
    
}

static void close_func()
{

}

static void call_function(int type, int param)
{

}

static void string_concat()
{
    Value *val1 = state->pop();
    Value *val2 = state->pop();
    // 右边的链接到左边

}

static void substring(int from, int to)
{
    Value *val = state->pop();
    if (val->get_type() != ValueType::t_string) {
        panic("not a string variable");
    }
    string *str = static_cast<String *>(val)->value();
    
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
        case OpCode::op_add_eq: 
        case OpCode::op_sub_eq: {
            operate(0, int(op));
            break;
        }

        case OpCode::op_mul_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_div_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_mod_eq: {
            operate(0, 0);
            break;
        }
        
        /* 逻辑运算符 */
        case OpCode::op_bin_or: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_xor: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_and: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_not: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_lm: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_rm: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_xor_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_and_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_or_eq: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_lme: {
            operate(0, 0);
            break;
        }
        case OpCode::op_bin_rme: {
            operate(0, 0);
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
            operate(1, 0);
            break;
        }
        case OpCode::op_len: {
            operate(1, 1);
            break;
        }
        case OpCode::op_unary_sub: {
            operate(1, 2);
            break;
        }
        case OpCode::op_add_add: {
            operate(1, 3);
            break;
        }
        case OpCode::op_sub_sub: {
            operate(1, 4);
            break;
        }


        case OpCode::op_concat: {
            operate(1, 0);
            break;
        }
        

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

        
        case OpCode::op_storeg:
            assign(0, param);
            break;
        case OpCode::op_storel:
            assign(0, param);
            break;
        case OpCode::op_storeu:
            assign(0, param);
            break;

        
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
            cout << "do jump cur: " << i << ", target: " << param << endl;
            i = param;
            break;
        
        default:
            panic("unsupport operation !!!");
            break;
        }
    }
}

void VM::execute(const std::vector<int> &pcs, State *_state, int argc, char **argv)
{
    state = _state;
    do_execute(pcs, 0, pcs.size());
}
