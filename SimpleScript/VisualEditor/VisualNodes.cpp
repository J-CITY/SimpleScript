#include "VisualNodes.h"

#include "VisualVisitors.h"

using namespace Visual;

void Node::run(Visitor& drawer) {
	drawer.run(*this);
}

void ValueNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void OperatorNode::run(Visitor& drawer) {
	drawer.run(*this);
}

//void OperatorNode::updateComputedType() {
//	computedType = inputs[0]->type;
//}

void VariableNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void FunNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void FunCall::run(Visitor& drawer) {
	drawer.run(*this);
}

void ClassNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void CommentNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void ImportNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void ThisNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void MethodNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void MethodCall::run(Visitor& drawer) {
	drawer.run(*this);
}

void EqualNode::run(Visitor& drawer) {
	drawer.run(*this);
}

void StartNode::run(Visitor& drawer) {
	drawer.run(*this);
}
