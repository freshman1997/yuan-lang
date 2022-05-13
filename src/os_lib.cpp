#include <fstream>
#include <ctime>

#include "os_lib.h"
#include "state.h"
#include "types.h"
#include "yuan.h"
#include "utils.h"


/**************** IO *****************/
static const int _in     = 0;
static const int _out    = 1;
static const int _ate    = 2;
static const int _append = 3;
static const int _trunc  = 4;
static const int _binary = 5;

static const int seek_beg = 0;
static const int seek_set = 1;
static const int seek_end = 2;

static int fd = 1;
static unordered_map<int, fstream *> fds;
static vector<int> freeFd;

static ios_base::_Openmode get_mod(int v)
{
    switch (v)
    {
    case _in:        return ios_base::in;
    case _out:       return ios_base::out;
    case _ate:       return ios_base::ate;
    case _append:    return ios_base::app;
    case _trunc:     return ios_base::trunc;
    case _binary:    return ios_base::binary;
    default:
        break;
    }
    return ios_base::_Openmode::_Openmask;
}

// 返回 fd
static int open_file(State *st)
{
    Value *val1 = st->pop();
    Value *val2 = st->pop();
    if (val1->get_type() != ValueType::t_number || val2->get_type() != ValueType::t_string) {
        panic("invalid parameter to open file!");
    }

    if (fd + 1 >= INT_MAX) {
        panic("too many opened file descoptors!!!");
    }
    
    NumberVal *mod = dynamic_cast<NumberVal *>(val1);
    StringVal *path = dynamic_cast<StringVal *>(val2);
    
    ios_base::_Openmode _mod = get_mod((int)mod->value());
    NumberVal *ret = new NumberVal;
    if (_mod == ios_base::_Openmask) {
        ret->set_val(0);
    }
    else {
        fstream *fs = new fstream;
        fs->open(path->value()->c_str(), _mod);
        if (fs->fail()) ret->set_val(0);
        else {
            int rfd = 0;
            if (freeFd.empty()) {
                rfd = fd;
                fd++;
            }
            else {
                rfd = freeFd.back();
                freeFd.pop_back();
            }
            fds[rfd] = fs;
            ret->set_val(rfd);
        }
    }
    check_variable_liveness(val1);
    check_variable_liveness(val2);
    st->push(ret);
    return 1;
}

// 传入 fd
static int close_file(State *st)
{
    Value *val = st->pop();
    if (val->get_type() != ValueType::t_number) {
        panic("invalid parameter to close fd!");
    }
    NumberVal *_fd = dynamic_cast<NumberVal *>(val);
    if (fds.count((int)_fd->value())) {
        fstream *fs = fds[(int)_fd->value()];
        fs->close();
        delete fs;
        // fd 会越来越大
        fds.erase((int)_fd->value());
        freeFd.push_back((int)_fd->value());
    }
    else {
        cout << "warning: fail to close file descriptor!" << endl;
    }
    return 1;
}

// seek(fd, diff, seekop, mod)
static int seek_file(State *st)
{
    Value *op = st->pop();
    Value *diff = st->pop();
    Value *file = st->pop();
    Value *m = st->pop();
    if (op->get_type() != ValueType::t_number || diff->get_type() != ValueType::t_number || file->get_type() != ValueType::t_number || m->get_type() != ValueType::t_number) {
        panic("invalid parameter to seek file");
    }
    NumberVal *mod = dynamic_cast<NumberVal *>(m);
    NumberVal *seekop = dynamic_cast<NumberVal *>(op);
    NumberVal *seekdiff = dynamic_cast<NumberVal *>(diff);
    NumberVal *seekfd = dynamic_cast<NumberVal *>(file);

    NumberVal *ret = new NumberVal;
    if (seekdiff->value() < 0 || !fds.count((int)seekfd->value()) || !((int)mod->value() == _in || (int)mod->value() == _out)) {
        panic("seek diff must be positive!");
    }
    fstream *fs = fds[(int)seekfd->value()];
    int sop = (int)seekop->value();
    if (sop == 0) {
        if ((int)mod->value() == _in) fs->seekg(ios_base::beg);
        else fs->seekp(ios_base::beg);
    }
    else if (sop == 1) {
        if ((int)mod->value() == _in) fs->seekg((int)seekdiff->value());
        else fs->seekp((int)seekdiff->value());
    }
    else if (sop == 2)
    {
        if ((int)mod->value() == _in) fs->seekg(ios_base::end);
        else fs->seekp(ios_base::end);
    }
    else {
        ret->set_val(-1);
    }
    check_variable_liveness(op);
    check_variable_liveness(diff);
    check_variable_liveness(file);
    check_variable_liveness(m);
    st->push(ret);
    return 1;
}

// get_file_size(fd)
static int file_size(State *st)
{
    Value *val = st->pop();
    if (val->get_type() != ValueType::t_number) {
        panic("invalid parameter to close fd!");
    }
    NumberVal *_fd = dynamic_cast<NumberVal *>(val);
    NumberVal *ret = new NumberVal;
    if (fds.count((int)_fd->value())) {
        fstream *fs = fds[(int)_fd->value()];
        fs->seekg(ios_base::end);
        ret->set_val(fs->rdbuf()->pubseekoff(0, std::ios::end, std::ios::in));
    }
    st->push(ret);
    return 1;
}

// read(fd, buf)
static int read_file(State *st)
{
    // 文件描述符，读入的缓冲区（数组），
    Value *val1 = st->pop();
    Value *val2 = st->pop();
    if (val1->get_type() != ValueType::t_number || val2->get_type() != ValueType::t_array) {
        panic("invalid parameter to read file!");
    }
    // 如何才能重用？
    NumberVal *reads = new NumberVal();
    NumberVal *_fd = dynamic_cast<NumberVal *>(val1);
    ArrayVal *buf = dynamic_cast<ArrayVal *>(val2);
    if (fds.count((int)_fd->value()) == 0 || buf->size() == 0) {
        reads->set_val(-1);
    }
    else {
        fstream *fs = fds[(int)_fd->value()];
        int sz = buf->size(), i = 0;
        while(i < sz) {
            if (fs->eof()) break;
            ByteVal *byte = dynamic_cast<ByteVal *>(buf->get(i));
            if (!byte) {
                panic("can not fill the array buffer!!!");
            }
            byte->_val = (unsigned char)fs->get();
            i++;
        }
        reads->set_val(i);
    }
    st->push(reads);
    return 1;
}

// write(fd, buf, sz)
static void write_file(State *st)
{
    Value *val1 = st->pop();
    Value *val2 = st->pop();
    Value *val3 = st->pop();

    if (val1->get_type() != ValueType::t_number || val2->get_type() != ValueType::t_array || val3->get_type() != ValueType::t_number) {
        panic("invalid parameter to write file!");
    }
    NumberVal *wrote = new NumberVal();
    NumberVal *_fd = dynamic_cast<NumberVal *>(val1);
    ArrayVal *buf = dynamic_cast<ArrayVal *>(val2);
    NumberVal *count = dynamic_cast<NumberVal *>(val3);
    if (fds.count((int)_fd->value()) == 0 || buf->size() == 0 || buf->size() < count->value()) {
        wrote->set_val(-1);
    }
    else {
        fstream *fs = fds[(int)_fd->value()];
        int i = 0, c = count->value();
        while (i < c) {
            ByteVal *byte = dynamic_cast<ByteVal *>(buf->get(i));
            if (!byte) {
                panic("can not write the array buffer!!!");
            }
            fs->put(byte->_val);
            i++;
        }
        wrote->set_val(i);
    }
    check_variable_liveness(val1);
    check_variable_liveness(val2);
    check_variable_liveness(val3);
    st->push(wrote);
}   

// 默认 8 字节
static void read_number(State *st)
{

}

static void write_number(State *st)
{

}

// readline(fd)
static int readline(State *st)
{
    Value *val = st->pop();
    if (val->get_type() != ValueType::t_number) {
        panic("invalid parameter to close fd!");
    }
    NumberVal *_fd = dynamic_cast<NumberVal *>(val);
    if (fds.count((int)_fd->value())) {
        fstream *fs = fds[(int)_fd->value()];
        StringVal *str = new StringVal;
        if (!getline(*fs, *str->value())) st->push(new NilVal);
        else st->push(str);
    }
    else {
        st->push(new NilVal);
    }
    return 1;
}

static int read_byte(State *st)
{
    return 1;
}

static int write_byte(State *st)
{
    return 1;
}

/**************** file operand *****************/
static int rename_file(State *st)
{
    return 1;
}

static int move_file(State *st)
{
    return 1;
}

// listfile(dir, recursive, optionExt)
static int list_files(State *st)
{
    int nparam = st->get_stack_size() - st->param_start - 1; // 第一个是当前函数的
    ArrayVal *ret = new ArrayVal;
    if (nparam == 4) { 
        Value *val1 = st->pop();
        Value *val2 = st->pop();
        Value *val3 = st->pop();
        Value *val4 = st->pop();
        BooleanVal *full = dynamic_cast<BooleanVal *>(val1);
        StringVal *_ext = dynamic_cast<StringVal *>(val2);
        BooleanVal *r = dynamic_cast<BooleanVal *>(val3);
        StringVal *dir = dynamic_cast<StringVal *>(val4);
        if (dir && r) {
            const char *ext = _ext ? _ext->value()->c_str() : NULL;
            vector<string> *files = get_dir_files(dir->value()->c_str(), ext, r->value(), full ? full->value() : false);
            // 算了，拷贝一次吧
            for (auto &it : *files) {
                StringVal *item = new StringVal;
                *item->value() = it;
                ret->add_item(item);
            }
            delete files;
        }
        else panic("invalid parameter amount to call");
        check_variable_liveness(val1);
        check_variable_liveness(val2);
        check_variable_liveness(val3);
        check_variable_liveness(val4);
    }
    st->push(ret);
    return 1;
}

static int rm_file(State *st)
{
    return 1;
}
/*********** random ***************/
// random(limit)
static int random(State *st)
{
    unsigned int now = time(NULL);
    srand(now);
    Value *val = st->pop();
    NumberVal *limit = dynamic_cast<NumberVal *>(val);
    NumberVal *ret = new NumberVal;
    if (limit) ret->set_val(rand() % (int)limit->value());
    else ret->set_val(-1);
    st->push(ret);
    return 1;
}

/************ time ***************/
static int get_now_time(State *st)
{
    NumberVal *ret = new NumberVal;
    unsigned int now = time(NULL);
    ret->set_val(now);
    st->push(ret);
    return 1;
}

/*********** Timer ***************/


void load_os_lib(TableVal *tb)
{
    StringVal *os = new StringVal;
    os->set_val("os");
    TableVal *ostb = new TableVal;

    StringVal *o = new StringVal;
    o->set_val("open");
    FunctionVal *_open = new FunctionVal;
    _open->nreturn = 1;
    _open->nparam = 2;
    _open->isC = true;
    _open->cfun = open_file;
    ostb->set(o, _open);

    StringVal *c = new StringVal;
    c->set_val("close");
    FunctionVal *_close = new FunctionVal;
    _close->nreturn = 0;
    _close->nparam = 1;
    _close->isC = true;
    _close->cfun = close_file;
    ostb->set(c, _close);

    StringVal *i = new StringVal;
    i->set_val("input");
    NumberVal *inmod = new NumberVal;
    inmod->set_val(_in);
    ostb->set(i, inmod);

    StringVal *size = new StringVal;
    size->set_val("get_file_size");
    FunctionVal *_sz = new FunctionVal;
    _sz->nreturn = 1;
    _sz->nparam = 1;
    _sz->isC = true;
    _sz->cfun = file_size;
    ostb->set(size, _sz);

    StringVal *rl = new StringVal;
    rl->set_val("readline");
    FunctionVal *_rl = new FunctionVal;
    _rl->nreturn = 1;
    _rl->nparam = 1;
    _rl->isC = true;
    _rl->cfun = readline;
    ostb->set(rl, _rl);

    StringVal *lf = new StringVal;
    lf->set_val("listfile");
    FunctionVal *_lf = new FunctionVal;
    _lf->nreturn = 1;
    _lf->nparam = 4;
    _lf->isC = true;
    _lf->cfun = list_files;
    ostb->set(lf, _lf);

    StringVal *rand = new StringVal;
    rand->set_val("random");
    FunctionVal *_rand = new FunctionVal;
    _rand->nreturn = 1;
    _rand->nparam = 1;
    _rand->isC = true;
    _rand->cfun = random;
    ostb->set(rand, _rand);

    StringVal *now = new StringVal;
    now->set_val("now");
    FunctionVal *_now = new FunctionVal;
    _now->nreturn = 1;
    _now->nparam = 0;
    _now->isC = true;
    _now->cfun = get_now_time;
    ostb->set(now, _now);

    tb->set(os, ostb);
}