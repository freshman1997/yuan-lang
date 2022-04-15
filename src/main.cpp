#include <iostream>

#include "lex.h"

using namespace std;

int main()
{

    TokenReader reader;
    tokenize("D:/code/test/cpp/yuan-lang/hello.y", reader);
    

    return 0;
}