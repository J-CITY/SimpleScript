#include "GraphModel.hpp"
#include "../Visitors/Visitor.hpp"

using namespace Visual;

void Node::accept(Visitor& v)        { v.run(*this); }
void StartNode::accept(Visitor& v)   { v.run(*this); }
void ValueNode::accept(Visitor& v)   { v.run(*this); }
void OperatorNode::accept(Visitor& v){ v.run(*this); }
void VariableNode::accept(Visitor& v){ v.run(*this); }
void ImportNode::accept(Visitor& v)  { v.run(*this); }
void CommentNode::accept(Visitor& v) { v.run(*this); }
void FunNode::accept(Visitor& v)     { v.run(*this); }
void FunCall::accept(Visitor& v)     { v.run(*this); }
void EqualNode::accept(Visitor& v)   { v.run(*this); }
void ClassNode::accept(Visitor& v)   { v.run(*this); }
void MethodNode::accept(Visitor& v)  { v.run(*this); }
void MethodCall::accept(Visitor& v)  { v.run(*this); }
void ThisNode::accept(Visitor& v)    { v.run(*this); }
