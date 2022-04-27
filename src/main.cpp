#include <iostream>

#include "lex.h"
#include "parser.h"
#include "visitor.h"

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

    return 0;
}