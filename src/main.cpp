#include <iostream>

#include "lex.h"
#include "parser.h"
#include "visitor.h"
#include "state.h"

using namespace std;

int main()
{
    clock_t start = clock();
    TokenReader reader;
    tokenize("D:/code/src/vs/yuan-lang/hello.y", reader);
    unordered_map<string, TokenReader *> files;
    files["D:/code/src/vs/yuan-lang/hello.y"] = &reader;
    unordered_map<string, Chunck *> *chunks = parse(files);
    CodeWriter writer;
    visit(chunks, writer);
    clock_t end = clock();
    cout << "spent time: " << (end - start) << endl;
    start = clock();
    State *st = new State(100);
    VM *vm = st->get_vm();
    st->load("D:/code/src/vs/yuan-lang/hello.b");
    st->run();
    end = clock();
    cout << "spent time: " << (end - start) / CLOCKS_PER_SEC << endl;
    return 0;
}