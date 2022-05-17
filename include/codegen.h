#ifndef __VISITOR_H__
#define __VISITOR_H__

#include "parser.h"
#include "code_writer.h"

void visit(unordered_map<string, Chunck *> *chunks, CodeWriter &writer, bool isDir);

#endif