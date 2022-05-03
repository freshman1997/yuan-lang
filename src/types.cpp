#include "types.h"

ValueType Nil::get_type() 
{
    return ValueType::t_null;
}

string Nil::name()
{
    return "";
}

size_t Nil::hash()
{
    return 0;
}

Value * Nil::copy()
{
    return NULL;
}

ValueType Boolean::get_type() const
{
    return ValueType::t_boolean;
}

std::string Boolean::name() const
{
    return _name;
}
void Boolean::set_name(const string &name)
{
    this->_name = std::move(name);
}
std::size_t Boolean::hash() const
{
    return 0;
}

void Boolean::set(bool val)
{
    this->_val = val;
}

bool Boolean::value() const
{
    return _val;
}

Value * Boolean::copy()
{
    Boolean *b = new Boolean;
    b->set(this->_val);
    return b;
}

ValueType Number::get_type() const
{
    return ValueType::t_number;
}

std::string Number::name() const
{
    return _name;
}
std::size_t Number::hash() const
{
    return size_t(std::hash<double>()(_val));
}
double Number::value() const
{
    return _val;
}
void Number::set_name(const string &name)
{
    this->_name = std::move(name);
}

void Number::set_val(double val)
{
    this->_val = val;
}

Value * Number::copy()
{
    Number *b = new Number;
    b->set_val(this->_val);
    return b;
}

ValueType Byte::get_type() const
{
    return ValueType::t_byte;
}
std::string Byte::name() const
{
    return "";
}

std::size_t Byte::hash() const
{
    return 0;
}

Value * Byte::copy()
{
    Byte *b = new Byte;
    b->_val = this->_val;
    return b;
}

ValueType String::get_type() const
{
    return ValueType::t_string;
}
std::string String::name() const
{
    return _name;
}
std::size_t String::hash() const
{
    return std::hash<string>()(_val);
}
string * String::value() 
{
    return &this->_val;
}

void String::set_val(const string &v)
{
    this->_val = v;
}

void String::set_val(const char *k)
{
    this->_val = k;
}


void String::set_name(const string &name)
{
    this->_name = std::move(name);
}

Value * String::get(int i)
{
    return NULL;
}

void String::set(int i, Value *val)
{
    if (i >= _val.size()) return;
    _val[i] = (static_cast<Byte *>(val))->_val;
}

void String::push(char c)
{
    _val.push_back(c);
}

Value * String::copy()
{
    String *s = new String;
    s->_name = this->_name;
    s->set_val(*s->value());
    return s;
}

ValueType Array::get_type() const {return ValueType::t_array;}
std::string Array::name() const {return _name;}
std::size_t Array::hash() const {return 0;}
vector<Value *> * Array::member(){return &this->members;}
Value * Array::get(int i){return this->members[i];}
void Array::add(Value *val){this->members.push_back(val);}
Array * Array::operator +(const Array *rhs)
{
    return NULL;
}

void Array::remove(int i)
{
    if (members.size() <= i) return;
    this->members.erase(members.begin() + i);
}
bool Array::set(int i, Value *val) 
{
    if (members.size() <= i) return false;
    delete members[i];
    members[i] = val;
    return true;
}
void Array::set_name(const string &name)
{
    this->_name = std::move(name);
}

Value * Array::copy()
{
    Array *arr = new Array;
    for (auto &it : *this->member()) {
        arr->add(it->copy());
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
            if ((*values)[hash].second->ref_count <= 0)
                delete (*values)[hash].second;
            if ((*values)[hash].first->ref_count <= 0)
                delete (*values)[hash].first;
        }
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

double TableVal::size()
{
    return this->values->size();
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

ValueType FunctionVal::get_type() const {return ValueType::t_function;}
std::string FunctionVal::name() const {return *chunk->_name;}
std::size_t FunctionVal::hash() const {return 0;}
bool FunctionVal::isClosure() const
{
    return  chunk->_name->empty();
}

void FunctionVal::set_name(const string &name)
{
    if (!chunk->_name) chunk->_name = new string;
    *chunk->_name = std::move(name);
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
    return this->subFuncs->at(i);
}

vector<FunctionVal *> * FunctionVal::get_subfuns()
{  
    return this->subFuncs;
}

Value * FunctionVal::copy()
{
    FunctionVal *val = new FunctionVal;
    val->chunk = new FunctionChunk;
    *val->chunk = *this->chunk;
    *val = *this;
    val->set_subfuns(this->get_subfuns());
    return val;
}

