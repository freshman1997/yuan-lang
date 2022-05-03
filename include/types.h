﻿#ifndef __TYPES_H__
#define __TYPES_H__

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

// 这个文件是运行时需要的

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
    virtual Value * copy() = 0;
    virtual ~Value() {}
    int ref_count = 0;

	/*struct ValueHasher : public std::unary_function<Value *, std::size_t>  {
		std::size_t operator() (const Value *value) const  {
			return value->hash();
		}
	};

	struct ValueEqualer : public std::binary_function<Value *, Value *, bool>  {
		bool operator() (const Value *left, const Value *right) const  {
			return left == right;
		}
	};*/
};

static bool is_value_equal(const Value *lhs, const Value *rhs);

class Nil : public Value
{
public:
	virtual ValueType get_type();
	virtual std::string name();
    virtual std::size_t hash();
    virtual Value * copy();
};

class Boolean : public Value
{
public:
	virtual ValueType get_type() const;
	virtual std::string name() const;
    virtual std::size_t hash() const;
    virtual Value * copy();
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
	virtual ValueType get_type() const;
	virtual std::string name() const;
    virtual std::size_t hash() const;
    virtual Value * copy();
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
	virtual ValueType get_type() const;
	virtual std::string name() const;
    virtual std::size_t hash() const;
    virtual Value * copy();
    unsigned char _val;
};

class String : public Value
{
public:
	virtual ValueType get_type() const;
	virtual std::string name() const;
    virtual std::size_t hash() const;
    virtual Value * copy();
    string * value();
    void set_val(const string&);
    void set_name(const string &name);
    Value * get(int i);
    void set(int i, Value *val);
    void push(char c);

private:
    string _val;
    string _name;
};

class Array : public Value
{
public:
	virtual ValueType get_type()const;
	virtual std::string name() const;
    virtual std::size_t hash() const;
    virtual Value * copy();
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
	virtual ValueType get_type() const;
	virtual std::string name() const;
    virtual std::size_t hash() const;
    virtual Value * copy();
    Value * get(Value *key);
    bool set(Value *key, Value *value);
    void remove(Value *key);
    void set_name(const string &name);
    double size();
    const unordered_map<int, std::pair<Value *, Value *>> * members();

private:
    // 只能 string, number 作为 key
    unordered_map<int, std::pair<Value *, Value *>> values;
    string _name;
};

struct UpValue
{
    string name;
    int in_stack;           //   在函数栈中的位置
    int upval_index;        //   upvalue 中的位置
    int index;              //   在父作用域中的位置（local）
};

// 函数为单位
struct FunctionChunk
{   
    int from_line;                                             // 开始行号
    int to_line;                                               // 结束行号
    bool is_varags = false;                                    // 是否为可变参数
    const char *filename = NULL;                               // 文件名
    string *_name = NULL;                                      // 函数名
    vector<Value *> *const_datas = NULL;                       // 常量池
    vector<Value *> *global_vars = NULL;                       // 全局变量，文件为单位
    vector<Value *> *local_variables = NULL;                   // 局部变量，函数为单位
    vector<UpValue *> *upvals = NULL;                          // upvalue，函数为单位.
    vector<int> *fun_body_ops = NULL;                          // 函数体指令
    unordered_map<int, int> *line_info = NULL;                 // 行号信息，一个指令一个行号
    unordered_map<string, int> *global_var_names_map;          // 全局对象名称对应的位置
    unordered_map<string, int> *local_var_names_map;           // 局部对象名称对应的位置

    // 如果 require 了其他文件，则其他文件的 全局变量和常量都可以访问

};

class FunctionVal : public Value
{
public:
    FunctionVal();
	virtual ValueType get_type() const;
	virtual std::string name() const;
    virtual std::size_t hash() const;
    virtual Value * copy();
    bool isClosure() const;
    void set_name(const string &name);
    UpValue *get_upvalue(int i);
    void add_upvalue(UpValue *);
    void set_localvar(const string &name, Value *);
    Value * get_localvar(const string &name);
    Value * get_localvar(int i);
    Value * get_global_var(int i);
    Value * get_global_const(int i);

    void set_file_name(const char *);
    string * get_file_name();

    FunctionVal *pre = NULL;
    void set_chunk(FunctionChunk *c);
    void set_subfuns(vector<FunctionVal *> *subFuncs);
    vector<int> * get_pcs();

    FunctionVal * get_subfun(int i);
    vector<FunctionVal *> * get_subfuns();

    bool is_local = false;
    int from_pc = 0;
    int to_pc = 0;
    int nparam = 0;
    int nreturn = 0;
    int in_stack = 0;
    bool varargs = false;
    int ncalls = 0;                                         // 调用自身的次数，也就是递归
    int param_stack = 0;
    FunctionChunk *chunk = NULL;

private:
    vector<FunctionVal *> *subFuncs = NULL;                  // 子函数
    string *_filename = NULL;
};


#endif