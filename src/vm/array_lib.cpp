#include "array_lib.h"
#include "state.h"
#include "utils.h"
#include "types.h"
#include "yuan.h"


// arr, val
static int push_back(State *st)
{
    Value *val = st->pop();
    Value *val1 = st->pop();
    ArrayVal *arr = dynamic_cast<ArrayVal *>(val1);
    if (arr) {
        arr->add_item(val);
    }
    check_variable_liveness(val1);
    return 0;
}

// arr, sz
static int set_size(State *st)
{
    Value *val = st->pop();
    Value *val1 = st->pop();
    NumberVal *sz = dynamic_cast<NumberVal *>(val);
    ArrayVal *arr = dynamic_cast<ArrayVal *>(val1);
    if (arr && sz) {
        for (int i = 0; i < sz->value(); ++i) {
            arr->add_item(new NilVal);
        }
    }
    check_variable_liveness(val);
    return 0;
}

static int remove(State *st)
{
    Value *val = st->pop();
    Value *val1 = st->pop();
    NumberVal *index = dynamic_cast<NumberVal *>(val);
    ArrayVal *arr = dynamic_cast<ArrayVal *>(val1);
    if (arr && index) {
        arr->remove((int)index->value());
    }
    check_variable_liveness(val);
    return 0;
}

static int merge_array(State *st)
{
    Value *val = st->pop();
    Value *val1 = st->pop();
    ArrayVal *arr1 = dynamic_cast<ArrayVal *>(val);
    ArrayVal *arr2 = dynamic_cast<ArrayVal *>(val1);
    if (arr1 && arr2) {
        ArrayVal *newArr = new ArrayVal;
        for (auto &it : *arr1->member()) {
            newArr->add_item(it);
        }
        for (auto &it : *arr2->member()) {
            newArr->add_item(it);
        }
        st->push(newArr);
    }
    else st->push(new NilVal);
    check_variable_liveness(val);
    check_variable_liveness(val1);
    return 0;
}

static int sub_array(State *st)
{

    return 0;
}

void load_array_lib(TableVal *tb)
{
    TableVal *_arrLib = new TableVal;
    StringVal *arrLibName = new StringVal;
    arrLibName->set_val("array");

    StringVal *app = new StringVal;
    app->set_val("append");
    FunctionVal *_app = new FunctionVal;
    _app->isC = true;
    _app->nparam = 2;
    _app->nreturn = 0;
    _app->cfun = push_back;
    _arrLib->set(app, _app);

    StringVal *grow = new StringVal;
    grow->set_val("grow");
    FunctionVal *_grow = new FunctionVal;
    _grow->isC = true;
    _grow->nparam = 2;
    _grow->nreturn = 0;
    _grow->cfun = set_size;
    _arrLib->set(grow, _grow);

    StringVal *rm = new StringVal;
    rm->set_val("remove");
    FunctionVal *_rm = new FunctionVal;
    _rm->isC = true;
    _rm->nparam = 2;
    _rm->nreturn = 0;
    _rm->cfun = remove;
    _arrLib->set(rm, _rm);

    StringVal *ma = new StringVal;
    ma->set_val("merge_array");
    FunctionVal *_ma = new FunctionVal;
    _ma->isC = true;
    _ma->nparam = 2;
    _ma->nreturn = 1;
    _ma->cfun = merge_array;
    _arrLib->set(ma, _ma);

    tb->set(arrLibName, _arrLib);
}   
