#include "GraphValidator.hpp"
#include "../Core/GraphContext.hpp"

using namespace Visual;

GraphValidator::GraphValidator(GraphContext& ctx) : ctx_(ctx) {}

bool GraphValidator::flowPinConnected(Node& node, int outputIdx) {
    if (outputIdx >= (int)node.outputs.size()) return false;
    return ctx_.isPinLinked(node.outputs[outputIdx]->id);
}

std::vector<ValidationError> GraphValidator::validate() {
    errors_.clear();

    // 1. Exactly one StartNode
    int startCount = 0;
    for (auto& [id, node] : ctx_.nodes)
        if (node->type == NodeType::Start) ++startCount;
    if (startCount == 0)
        errors_.push_back({0, "Graph has no StartNode."});
    else if (startCount > 1)
        errors_.push_back({0, "Graph has more than one StartNode."});

    // 2. Visit each node for per-node checks
    for (auto& [id, node] : ctx_.nodes)
        node->accept(*this);

    return errors_;
}

void GraphValidator::run(ValueNode& /*n*/)    {}
void GraphValidator::run(VariableNode& /*n*/) {}
void GraphValidator::run(ImportNode& /*n*/)   {}
void GraphValidator::run(CommentNode& /*n*/)  {}
void GraphValidator::run(StartNode& /*n*/)    {}
void GraphValidator::run(EqualNode& /*n*/)    {}
void GraphValidator::run(ThisNode& /*n*/)     {}

void GraphValidator::run(Node& node) {
    // Reject unsupported node types
    if (!NodeRegistry::isSupported(node.type))
        addError(node.id, "Node type is not supported in visual blueprints: " + node.name);
}

void GraphValidator::run(FunNode& node) {
    if (node.funName.empty())
        addError(node.id, "Function node has no name.");
}

void GraphValidator::run(FunCall& node) {
    if (!ctx_.getNodeById(node.funDeclarationId))
        addError(node.id, "FunCall references missing FunctionDeclaration.");
}

void GraphValidator::run(OperatorNode& node) {
    if (node.inputs.size() < 2)
        addError(node.id, "Operator node needs 2 inputs.");
}

void GraphValidator::run(ClassNode& node) {
    if (node.className.empty())
        addError(node.id, "Class node has no name.");
}

void GraphValidator::run(MethodNode& node) {
    if (node.funName.empty())
        addError(node.id, "Method node has no name.");
    if (!ctx_.getNodeById(node.classNodeId))
        addError(node.id, "MethodNode references missing ClassDeclaration.");
}

void GraphValidator::run(MethodCall& node) {
    if (!ctx_.getNodeById(node.methodeDeclarationId))
        addError(node.id, "MethodCall references missing MethodDeclaration.");
}
