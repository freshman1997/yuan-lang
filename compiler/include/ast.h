#ifndef __AST_H__
#define __AST_H__
#include <string>
using std::string;

class Position;
class AstVisitor;
class AstNode
{
public:
    Position beginPos; //在源代码中的第一个Token的位置
    Position endPos;   //在源代码中的最后一个Token的位置
    bool isErrorNode;// = false;

    AstNode *parentNode = NULL; //父节点。父节点为null代表这是根节点。
    
    AstNode(Position _beginPos, Position _endPos, bool _isErrorNode) : beginPos(_beginPos), endPos(_endPos), isErrorNode(_isErrorNode) {}

    //visitor模式中，用于接受vistor的访问。
    virtual void accept(const AstVisitor &visitor, void *data) = 0;      

    // 简单的string格式
    string toString();

};

class Decl
{
public:
    string name;
};

class Statement : public AstNode
{
public:
    virtual void accept(const AstVisitor &visitor, void *data) = 0;
    // 简单的string格式
    string toString();

};

class Scope
{
public:
    
};

#endif