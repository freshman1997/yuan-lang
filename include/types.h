#ifndef __TYPES_H__
#define __TYPES_H__

#include <string>

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


#endif