#ifndef __UITILS_H__
#define __UITILS_H__

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <cstring>
#include <cstdlib>

#include <string>
#include <iostream>

using namespace std;


// 字符串是否相等
inline bool str_equal(const char *s1, const char *s2, int n)
{
    if (n == -1) n = strlen(s2);
    while(*(s1++) == *(s2++)) --n;
    return n == 0;
}

inline bool startswith(char* p, const char* q)
{
	return strncmp(p, q, strlen(q)) == 0;
}

inline void error(const char *msg, const char *s)
{
    cerr << msg << s << endl;
    exit(1);
}

#endif