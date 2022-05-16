#include "lex.h"
#include "parser.h"
#include "code_writer.h"
#include "codegen.h"
#include "yuan.h"
#include "utils.h"

void compile(const char *file)
{
    TokenReader reader;
    tokenize(file, reader);
    unordered_map<string, TokenReader *> files;
    files[file] = &reader;
    unordered_map<string, Chunck *> *chunks = parse(files);
    CodeWriter writer;
    visit(chunks, writer);
}

int compile_dir(const char *dir)
{
    vector<string> *files = get_dir_files(dir, ".y", true, true);
    if (files->empty()) {
        cout << "no such directory or no yuanscript file in directory!" << endl;
        return -1;
    }

    unordered_map<string, TokenReader *> tokenFiles;
    for (auto &it : *files) {
        TokenReader *reader = new TokenReader;
        tokenize(it.c_str(), *reader);
        tokenFiles[it] = reader;
    }
    unordered_map<string, Chunck *> *chunks = parse(tokenFiles);
    CodeWriter writer;
    visit(chunks, writer);
    int sz = chunks->size();
    delete chunks;
    return sz;
}
