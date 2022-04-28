#include "types.h"

static bool is_value_equal(const Value *lhs, const Value *rhs)
{
    if (lhs->get_type() == ValueType::t_number) {
        if (rhs->get_type() == ValueType::t_number) {
            return true;
        }
        return false;
    }
    return false;
}

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


ValueType Table::get_type() const {return ValueType::t_table;}
std::string Table::name() const {return _name;}
std::size_t Table::hash() const {return 0;}
Value * Table::get(Value *key)
{
    if ((key->get_type() == ValueType::t_number || key->get_type() == ValueType::t_string) && values.count(key->hash())) {
        return values[key->hash()].second;
    }
    return NULL;
}
bool Table::set(Value *key, Value *value)
{
    if ((key->get_type() == ValueType::t_number || key->get_type() == ValueType::t_string)) {
        if (values.count(key->hash())) {
            delete values[key->hash()].second;
            delete values[key->hash()].first;
        }
        values[key->hash()] = std::make_pair(key, value);
        return true;
    }
    return false;
}
void Table::remove(Value *key)
{
    if ((key->get_type() == ValueType::t_number || key->get_type() == ValueType::t_string) && values.count(key->hash())) {
        delete values[key->hash()].second;
        delete values[key->hash()].first;
        values.erase(key->hash());
    }
}
void Table::set_name(const string &name)
{
    _name = std::move(name);
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
    *chunk->_name = std::move(name);
}
UpValue * FunctionVal::get_upvalue(int i)
{
    if (chunk->upvals->size() <= i) return NULL;
    return (*chunk->upvals)[i];
}
void FunctionVal::add_upvalue(UpValue *upv)
{
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

