#ifndef __YUAN_H__
#define __YUAN_H__


#define MAX_FILE_LINE       (1000000)             // 单个文件最大行数
#define MAX_FILE_SIZE       (INT_MAX - 2)         // 单个文件最大大小

#define MAX_NUM             INT_MAX               // 数字的最大
#define MAX_STRING_LENGTH   (INT_MAX - 2)         // 字符串最大长度

#define MAX_UPVALUE_NUM     60                    // uplvaue 最大的数量
#define MAX_LOCAL_VARIABLE  200                   // 一个函数最多能定义的局部变量数目

#define MAX_RECURSE_NUM     200                   // 最大递归次数

void panic(const char *reason);

void compile(const char *file);

int compile_dir(const char *dir);


#endif