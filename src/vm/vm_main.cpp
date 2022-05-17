#include <iostream>
#include <direct.h>

#include "lex.h"
#include "parser.h"
#include "codegen.h"
#include "state.h"
#include "utils.h"
#include "yuan.h"
#include "types.h"

using namespace std;

static string entry_file;

static bool check_entry(const char *entry)
{
    if (is_valid_path(entry) && is_file(entry)) return true;
    return false;
}

static Value * parse_num(const char *s, int &idx)
{
    const char *p = s;
    NumberVal *ret = new NumberVal();
    if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)p[1]))) {
        const char* q = p++;
        for (;;) {
            if (p[0] && p[1] && strchr("eEpP", p[0]) && strchr("+-", p[1]))
                p += 2;
            else if (isalnum(*p) || *p == '.')
                p++;
            else
                break;
        }
    }
    int len = int(p - s);
    char *num = new char[len + 1];
    memcpy(num, s, len);
    num[len] = '\0';
    ret->set_val(atof(num));
    delete[] num;
    idx += len;
    return ret;
}

static Value * parse_str(const char *s, int &idx)
{
    const char *p = s + 1;
    StringVal *str = new StringVal;
    while (*p != '"') {
        str->push(*p);
        ++p;
        ++idx;
    }
    idx += 2;
    return str;
}

static ArrayVal * parse_args(int argc, char** argv)
{
    ArrayVal *arr = new ArrayVal;
    for (int i = 1; i < argc; ++i) {
        if (str_equal1(argv[i], "--entry", 7)) {
            // check if is full path
            ++i;
            if (i >= argc) {
                cout << "not found entry file!" << endl;
                exit(0);
            }
            if (!check_entry((string(argv[i]) + ".b").c_str())) {
                cout << "invalid entry file path!" << endl;
                exit(0);
            }
            entry_file = (string(argv[i]) + ".b");
        }
        else if (str_equal1(argv[i], "--args", 6)) {
            i++;
            char *p = argv[i];
            int idx = 0;
            while (p[idx] != '\0') {
                if (isspace(p[idx])) {
                    idx++;
                    continue;
                }

                if (p[idx] >= '0' && p[idx] <= '9') {
                    Value *num = parse_num(p + idx, idx);
                    arr->add_item(num);
                }
                else if (p[idx] == '"') {
                    Value *str = parse_str(p + idx, idx);
                    arr->add_item(str);
                }
                else if (p[idx] == 't') {
                    if (str_equal1(p + idx, "true", 4)) {
                        BooleanVal *b = new BooleanVal;
                        b->set(true);
                        arr->add_item(b);
                        idx += 4;
                    }
                    else {
                        cout << "unregnize boolean true!" << endl;
                        exit(0);
                    }
                }
                else if (p[idx] == 'f') {
                    if (str_equal1(p + idx, "false", 5)) {
                        arr->add_item(new BooleanVal);
                        idx += 5;
                    }
                    else {
                        cout << "unregnize boolean false!" << endl;
                        exit(0);
                    }
                }
                else if (p[idx] == 'n') {
                    if (str_equal1(p + idx, "nil", 3)) {
                        arr->add_item(new NilVal);
                        idx += 3;
                    }
                    else {
                        cout << "unregnize nil!" << endl;
                        exit(0);
                    }
                }
                if (p[idx] == ',') idx++;
            }
        }
        else {
            if (!check_entry((string(argv[i]) + ".b").c_str())) {
                cout << "invalid entry file path!" << endl;
                exit(0);
            }
            entry_file = (string(argv[i]) + ".b");
        }
    }
    return arr;
}


// --entry hello.y --args "1, 2, 3, \"hello\", false, nil" 
int main(int argc, char **argv)
{
    char* cwd = getcwd(NULL, 0);
    if (!cwd) {
        cout << "unexpected!" << endl;
        return 1;
    }
    string _cwd = cwd;
    setcwd(_cwd);
    State *st = new State(100);
    VM *vm = st->get_vm();
    st->run(entry_file.c_str(), parse_args(argc, argv));
    return 0;
}

