#ifndef __TYPES_H__
#define __TYPES_H__

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

enum class ValueType
{
    t_number,           // 数字
    t_boolean,          // 布尔值
    t_null,             // nil
    t_string,           // 字符串
    t_function,         // 函数
    t_array,            // 数组
    t_table,            // 表
    t_byte,             // byte
};

class Value
{
public:
    ValueType type;
    virtual ValueType get_type() const = 0;
	virtual std::string name() const = 0;
    virtual std::size_t hash() const = 0;

	struct ValueHasher : public std::unary_function<Value *, std::size_t>  {
		std::size_t operator() (const Value *value) const  {
			return value->hash();
		}
	};

	struct ValueEqualer : public std::binary_function<Value *, Value *, bool>  {
		bool operator() (const Value *left, const Value *right) const  {
			return left == right;
		}
	};
};

static bool is_value_equal(const Value *lhs, const Value *rhs);

class Nil : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
};

class Boolean : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
    void set(bool val);
    bool value() const;
    void set_name(const string &name);

private:
    bool _val;
    string _name;
};

class Number : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
    double value() const;
    void set_name(const string &name);
    void set_val(double val);

private:
    double _val;
    string _name;
};

class Byte : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
    unsigned char _val;
};

class String : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
    string * value();
    void set_name(const string &name);
    Value * get(int i);
    void set(int i, Value *val);

private:
    string _val;
    string _name;
};

class Array : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
    vector<Value *> * member();
    Value * get(int i);
    void add(Value *);
    Array * operator +(const Array *rhs);  // +
    void remove(int i);
    bool set(int i, Value *val);
    void set_name(const string &name);

private:
    vector<Value *> members;
    string _name;
};

class Table : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
    Value * get(Value *key);
    bool set(Value *key, Value *value);
    void remove(Value *key);
    void set_name(const string &name);

private:
    // 只能 string, number 作为 key
    unordered_map<int, std::pair<Value *, Value *>> values;
    string _name;
};

struct UpValue
{
    string name;
    int upval_index;        //   upvalue 中的位置
    int index;              //   在父作用域中的位置（local）
};

class Function : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
    bool isClosure() const;
    void set_name(const string &name);
    UpValue *get_upvalue(int i);
    void add_upvalue(UpValue *);
    void set_localvar(const string &name, Value *);
    Value * get_localvar(const string &name);
    Function *pre = NULL;

private:
    vector<UpValue *> upvals;
    unordered_map<string, Value *> localVars;
    string _name;
};


#endif