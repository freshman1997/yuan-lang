#include <iostream>

#include "lex.h"
#include "parser.h"
#include "codegen.h"
#include "state.h"
#include "utils.h"
#include "yuan.h"
#include "types.h"

using namespace std;

static const char *entry = NULL;

static ArrayVal * parse_args(int argc, char** argv)
{
    
    return new ArrayVal;
}

static bool check_entry(const char *entry)
{

    return false;
}

// --entry hello.y --args "1, 2, 3, [12, 3, 4], {1212}, \"hello\", false, nil" 
int main(int argc, char **argv)
{
    to_cwd(argv[0]);
    clock_t start = clock();
    compile("D:/code/test/cpp/yuan-lang/hello.y");
    clock_t end = clock();
    cout << "spent time: " << (end - start) << endl;
    start = clock();
    State *st = new State(100);
    VM *vm = st->get_vm();
    st->run("D:/code/test/cpp/yuan-lang/hello.y", parse_args(argc, argv));
    end = clock();
    cout << "spent time: " << (end - start) << endl;
    return 0;
}