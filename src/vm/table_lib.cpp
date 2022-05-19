#include "table_lib.h"
#include "types.h"
#include "yuan.h"
#include "state.h"

static int remove(State *st)
{
    Value *val = st->pop();
    Value *val1 = st->pop();

    TableVal *tb = dynamic_cast<TableVal *>(val1);
    if (tb) {
        if (val->get_type() == ValueType::t_number || val->get_type() == ValueType::t_string) {
            tb->remove(val);
        }
    }
    return 0;
}

static int clear()
{

    return 0;
}


void load_table_lib(TableVal *tb)
{
    TableVal *_tbLib = new TableVal;
    StringVal *tbLib = new StringVal;
    tbLib->set_val("table");

    StringVal *rm = new StringVal;
    rm->set_val("grow");
    FunctionVal *_rm = new FunctionVal;
    _rm->isC = true;
    _rm->nparam = 2;
    _rm->nreturn = 0;
    _rm->cfun = remove;
    _tbLib->set(rm, _rm);

    tb->set(tbLib, _tbLib);
}
