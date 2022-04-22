#include <iostream>

#include "lex.h"
#include "parser.h"

using namespace std;

int main()
{

    clock_t start = clock();
    TokenReader reader;
    tokenize("D:/code/test/cpp/yuan-lang/hello.y", reader);
    unordered_map<string, TokenReader *> files;
    files["D:/code/test/cpp/yuan-lang/hello.y"] = &reader;
    parse(files);
    clock_t end = clock();
    cout << "spent time: " << (end - start) << endl;

    return 0;
}