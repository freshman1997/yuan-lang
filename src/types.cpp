#include "types.h"

ValueType NilVal::get_type() const
{
    return ValueType::t_null;
}

string NilVal::name() const
{
    return "";
}

size_t NilVal::hash() const
{
    return 0;
}

Value * NilVal::copy()
{
    return new NilVal;
}

ValueType BooleanVal::get_type() const
{
    return ValueType::t_boolean;
}

std::string BooleanVal::name() const
{
    return _name;
}
void BooleanVal::set_name(const string &name)
{
    this->_name = std::move(name);
}
std::size_t BooleanVal::hash() const
{
    return 0;
}

void BooleanVal::set(bool val)
{
    this->_val = val;
}

bool BooleanVal::value() const
{
    return _val;
}

Value * BooleanVal::copy()
{
    BooleanVal *b = new BooleanVal;
    b->set(this->_val);
    return b;
}

ValueType NumberVal::get_type() const
{
    return ValueType::t_number;
}

std::string NumberVal::name() const
{
    return _name;
}
std::size_t NumberVal::hash() const
{
    return size_t(std::hash<double>()(_val));
}
double NumberVal::value() const
{
    return _val;
}
void NumberVal::set_name(const string &name)
{
    this->_name = std::move(name);
}

void NumberVal::set_val(double val)
{
    this->_val = val;
}

Value * NumberVal::copy()
{
    NumberVal *b = new NumberVal;
    b->set_val(this->_val);
    return b;
}

ValueType ByteVal::get_type() const
{
    return ValueType::t_byte;
}
std::string ByteVal::name() const
{
    return "";
}

std::size_t ByteVal::hash() const
{
    return 0;
}

Value * ByteVal::copy()
{
    ByteVal *b = new ByteVal;
    b->_val = this->_val;
    return b;
}

ValueType StringVal::get_type() const
{
    return ValueType::t_string;
}
std::string StringVal::name() const
{
    return _name;
}
std::size_t StringVal::hash() const
{
    return std::hash<string>()(_val);
}

string * StringVal::value() 
{
    return &this->_val;
}

void StringVal::set_val(const string &v)
{
    this->_val = v;
}

void StringVal::set_val(const char *k)
{
    this->_val = k;
}


void StringVal::set_name(const string &name)
{
    this->_name = std::move(name);
}

Value * StringVal::get(int i)
{
    if (i < 0 || i >= this->_val.size()) return NULL;
    ByteVal *ch = new ByteVal;
    ch->_val = (unsigned char)this->_val[i];
    return ch;
}

void StringVal::set(int i, Value *val)
{
    if (i >= _val.size()) return;
    _val[i] = (static_cast<ByteVal *>(val))->_val;
}

void StringVal::push(char c)
{
    _val.push_back(c);
}

int StringVal::size()
{
    return this->_val.size();
}

Value * StringVal::copy()
{
    StringVal *s = new StringVal;
    s->_name = this->_name;
    s->set_val(*this->value());
    return s;
}

ValueType ArrayVal::get_type() const {return ValueType::t_array;}
std::string ArrayVal::name() const { return _name;}
std::size_t ArrayVal::hash() const {return 0;}
vector<Value *> * ArrayVal::member(){return &this->members;}
Value * ArrayVal::get(int i){return this->members[i];}
void ArrayVal::add_item(Value *val){this->members.push_back(val);}

ArrayVal * ArrayVal::operator +(const ArrayVal *rhs)
{
    return NULL;
}

void ArrayVal::remove(int i)
{
    if (members.size() <= i) return;
    this->members.erase(members.begin() + i);
}

bool ArrayVal::set(int i, Value *val) 
{
    if (i < 0 || members.size() <= i) return false;
    if (members[i] && members[i]->ref_count <= 0) delete members[i];
    members[i] = val;
    return true;
}
void ArrayVal::set_name(const string &name)
{
    this->_name = name;
}

int ArrayVal::size()
{
    return this->members.size();
}

Value * ArrayVal::copy()
{
    ArrayVal *arr = new ArrayVal;
    for (auto &it : *this->member()) {
        arr->add_item(it->copy());
    }
    return arr;
}

ValueType TableVal::get_type() const {return ValueType::t_table;}
std::string TableVal::name() const {return _name;}
std::size_t TableVal::hash() const {return 0;}
TableVal::TableVal()
{
    this->values = new unordered_map<int, std::pair<Value *, Value *>>;
}
TableVal::~TableVal()
{
    for (auto &it : *this->values) {
        if (it.second.first) {
            --it.second.first->ref_count;
            if (it.second.first->ref_count <= 0) delete it.second.first;
        }
        if (it.second.second) {
            --it.second.second->ref_count;
            if (it.second.second->ref_count <= 0) delete it.second.second;
        }
    }
    delete this->values;
}

Value * TableVal::get(Value *key)
{
    if ((key->get_type() == ValueType::t_number || key->get_type() == ValueType::t_string) && values->count(key->hash())) {
        return (*values)[key->hash()].second;
    }
    return NULL;
}

bool TableVal::set(Value *key, Value *value)
{
    int hash = key->hash();
    if ((key->get_type() == ValueType::t_number || key->get_type() == ValueType::t_string)) {
        if (values->count(hash)) {
            --(*values)[hash].second->ref_count;
            if ((*values)[hash].second->ref_count <= 0)
                delete (*values)[hash].second;
            --(*values)[hash].first->ref_count;
            if ((*values)[hash].first->ref_count <= 0)
                delete (*values)[hash].first;
        }
        key->ref_count++;
        value->ref_count++;
        (*values)[hash] = std::make_pair(key, value);
        return true;
    }
    return false;
}

void TableVal::remove(Value *key)
{
    int hash = key->hash();
    if ((key->get_type() == ValueType::t_number || key->get_type() == ValueType::t_string) && (*values).count(hash)) {
        delete (*values)[hash].second;
        delete (*values)[hash].first;
        (*values).erase(hash);
    }
}
void TableVal::set_name(const string &name)
{
    _name = std::move(name);
}

int TableVal::size()
{
    return this->values->size();
}

unordered_map<int, std::pair<Value *, Value *>>::iterator TableVal::get_iter(int i)
{
    auto it = this->values->begin();
    while(i--) ++it;
    if (it != this->values->end()) return it;
    else return this->values->end();
}


const unordered_map<int, std::pair<Value *, Value *>> * TableVal::members()
{
    return this->values;
}


Value * TableVal::copy()
{
    TableVal *tb = new TableVal;
    for (auto &it : *this->members()) {
        const pair<Value*, Value *> &p = it.second;
        tb->set(p.first, p.second);
    }
    return tb;
}

FunctionVal::FunctionVal()
{
    chunk = new FunctionChunk;
}

FunctionVal::~FunctionVal()
{
    if (this->isMain) {
        for (auto &it : *chunk->const_datas) {
            if (it) delete it;
        }
        for (auto &it : *chunk->global_vars) {
            if (it) delete it;
        }
        delete chunk->const_datas;
        delete chunk->global_vars;
        delete chunk->fun_body_ops;
        delete chunk->upvals->at(0);
        
        if (chunk->global_var_names_map) delete chunk->global_var_names_map;
        if (chunk->local_var_names_map) delete chunk->local_var_names_map;
    }
    delete chunk->local_variables;
    if (this->upvOwner) {
        if (this->chunk->_name) delete this->chunk->_name;
        int i = 0;
        for (auto &it : *chunk->upvals) {
            if (i != 0 && it) {
                if (it->val) delete it->val;
                delete it;
            }
            i++;
        }
        delete chunk->upvals;
    }
    for (auto &it : *this->subFuncs) {  
        if (it) delete it;
    }
    delete chunk;
}


ValueType FunctionVal::get_type() const {return ValueType::t_function;}
std::string FunctionVal::name() const {return *chunk->_name;}
std::size_t FunctionVal::hash() const {return 0;}
bool FunctionVal::isClosure() const
{
    return  chunk->_name->empty();
}

void FunctionVal::set_name(const string &name)
{
    if (!chunk->_name) chunk->_name = new string(name);
    else *chunk->_name = name;
}
UpValue * FunctionVal::get_upvalue(int i)
{
    if (chunk->upvals->size() <= i) return NULL;
    return (*chunk->upvals)[i];
}
void FunctionVal::add_upvalue(UpValue *upv)
{
    if (!chunk->upvals) chunk->upvals = new vector<UpValue *>;
    chunk->upvals->push_back(upv);
}
void FunctionVal::set_localvar(const string &name, Value *val)
{
    (*chunk->local_variables)[chunk->local_variables->size()] = val;
    (*chunk->local_var_names_map)[name] = chunk->local_variables->size() - 1;
}
Value * FunctionVal::get_localvar(const string &name)
{
    if (chunk->local_var_names_map->count(name)) {
        return (*chunk->local_variables)[(*chunk->local_var_names_map)[name]];
    }
    return NULL;
}

Value * FunctionVal::get_localvar(int i)
{
    if (i >= chunk->local_variables->size()) return NULL;
    return (*chunk->local_variables)[i];
}

Value * FunctionVal::get_global_var(int i)
{
    FunctionVal *cur = this;
    while (cur->pre) {
        cur = cur->pre;
    }
    if (i < 0 || i >= cur->chunk->global_vars->size()) return NULL;
    return cur->chunk->global_vars->at(i);
}

Value * FunctionVal::get_global_const(int i)
{
    FunctionVal *cur = this;
    while (cur->pre) {
        cur = cur->pre;
    }
    if (i < 0 || i >= cur->chunk->const_datas->size()) return NULL;
    return cur->chunk->const_datas->at(i);
}

void FunctionVal::set_file_name(const char *name)
{
    this->_filename = new string(name);
}

string * FunctionVal::get_file_name()
{
    return this->_filename;
}


void FunctionVal::set_chunk(FunctionChunk *c)
{
    this->chunk = c;
}

void FunctionVal::set_subfuns(vector<FunctionVal *> *subFuncs)
{
    this->subFuncs = subFuncs;
}

vector<int> * FunctionVal::get_pcs()
{
    return chunk->fun_body_ops;
}

FunctionVal * FunctionVal::get_subfun(int i)
{
    if (i < 0 || i >= subFuncs->size()) return NULL;
    return static_cast<FunctionVal *>(this->subFuncs->at(i));
}

vector<FunctionVal *> * FunctionVal::get_subfuns()
{  
    return this->subFuncs;
}

Value * FunctionVal::copy()
{
    FunctionVal *val = new FunctionVal;
    *val->chunk = *this->chunk;
    val->ncalls = this->ncalls;
    val->chunk->local_variables = new vector<Value *>;
    for (auto &it : *this->chunk->local_variables) {
        if (it){
            if ((it->get_type() == ValueType::t_boolean || it->get_type() == ValueType::t_null || it->get_type() == ValueType::t_number 
            || it->get_type() == ValueType::t_byte || it->get_type() == ValueType::t_function)) {
                val->chunk->local_variables->push_back(it->copy());
            }
            else {
                val->chunk->local_variables->push_back(it);
            }
            val->chunk->local_variables->back()->ref_count++;
        }
        else val->chunk->local_variables->push_back(NULL);
    }
    val->isMain = this->isMain;
    val->pre = this->pre;
    val->is_local = this->is_local;
    val->from_pc = this->from_pc;
    val->to_pc = this->to_pc;
    val->nparam = this->nparam;
    val->nreturn = this->nreturn;
    val->in_stack = this->in_stack;
    val->set_file_name(this->get_file_name()->c_str());
    vector<FunctionVal *> *subfuns = new vector<FunctionVal *>;
    for (auto &it : *this->subFuncs) {
        subfuns->push_back(static_cast<FunctionVal *>(it->copy()));
    }
    val->set_subfuns(subfuns);
    return val;
}

