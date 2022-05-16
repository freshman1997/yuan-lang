#ifndef __UITILS_H__
#define __UITILS_H__

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <cstring>
#include <cstdlib>
#include <vector>

#include <string>
#include <iostream>

using namespace std;

// 字符串是否相等
inline bool str_equal(const char *s1, const char *s2, int n)
{
    int len2 = strlen(s2);
    if (len2 != n) return false;
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

struct Token;
void error_tok(const Token &tok, const char *filename, char * content, char *fmt, ...);

bool is_valid_path(const char *path);
bool is_file(const char *path);

void to_cwd(const char *exe);
string getcwd();

vector<string> * get_dir_files(const char *dir, bool r, bool f);

vector<string> * get_dir_files(const char *dir, const char *ext, bool r, bool f);

#endif