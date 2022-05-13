#include <iostream>

#include "lex.h"
#include "parser.h"
#include "codegen.h"
#include "state.h"
#include "utils.h"
#include "yuan.h"

using namespace std;

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
    st->run("D:/code/test/cpp/yuan-lang/hello.y", NULL);
    end = clock();
    cout << "spent time: " << (end - start) / CLOCKS_PER_SEC << endl;
    return 0;
}