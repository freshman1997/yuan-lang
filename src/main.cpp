#include <iostream>

#include "lex.h"
#include "parser.h"

using namespace std;

int main()
{

    TokenReader reader;
    tokenize("D:/code/src/vs/yuan-lang/hello.y", reader);
    unordered_map<string, TokenReader *> files;
    files["hello"] = &reader;
    parse(files);
    
    return 0;
}