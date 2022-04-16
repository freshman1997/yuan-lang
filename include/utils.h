#ifndef __UITILS_H__
#define __UITILS_H__

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <cstring>
#include <cstdlib>

#include <string>
#include <iostream>

using namespace std;

#define MAX_FILE_LINE       (1000000)             // 单个文件最大行数
#define MAX_FILE_SIZE       (INT_MAX - 2)         // 单个文件最大大小

#define MAX_NUM             INT_MAX               // 数字的最大
#define MAX_STRING_LENGTH   (INT_MAX - 2)         // 字符串最大长度

// 字符串是否相等
inline bool str_equal(const char *s1, const char *s2, int n)
{
    if (n == -1) n = strlen(s2);
    while(*(s1++) == *(s2++)) --n;
    return n == 0;
}

static bool startswith(char* p, const char* q) {
	return strncmp(p, q, strlen(q)) == 0;
}

inline void error(const char *msg, const char *s)
{
    cerr << msg << s << endl;
    exit(1);
}

static bool iden_not_allow_sym(char c)
{
    switch (c)
    {
    case '+':case '-':case '*':case '/':case '&':case '#':case '@':case '!':case '.':case ':': case '^':case '>':case '<':case '=':
        return true;
    default:
        return false;
    }
}

static bool check_iden_start(char *str, int len)
{
    int i = 0;
    while (i < len)
    {
        if (iden_not_allow_sym(str[i]) || isdigit((unsigned char)str[i])) {
            return false;
        }
        else break;
    }
    return true;
}

static bool is_identity(char *src, int len)
{
    // 开头不能是数字和操作符
    if (check_iden_start(src, len)) {
        int i = 0;
        while (i < len)
        {
            if (iden_not_allow_sym(src[i])) {
                return false;
            }
            else break;
        }
        return true;
    }
    return false;
}

#endif