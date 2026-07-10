#include "CodeGenerator.hpp"
#include "../Core/GraphContext.hpp"
#include <stdexcept>
#include <string>

using namespace Visual;

CodeGenerator::CodeGenerator(GraphContext& ctx) : ctx_(ctx) {}

// -----------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------

void CodeGenerator::genCodeForFuncInputVar(FunNode& node, Pin& pin) {
    for (auto& e : node.outputs) {
        if (e->id == pin.id) {
            emitter.code += node.inputVars[e->id].first;
        }
    }
}

void CodeGenerator::genCodeForLoopIndexVar(Node& node) {
    emitter.code += "i_" + std::to_string(node.id);
}

void CodeGenerator::genCodeForExpression(NodePtr node, Pin& pin) {
    if (!node) return;
    if (node->type == NodeType::FunctionDeclaration) {
        auto nextPin = ctx_.getNextPin(pin);
        if (nextPin)
            genCodeForFuncInputVar(*static_cast<FunNode*>(node.get()), *nextPin);
    } else if (node->type == NodeType::For) {
        genCodeForLoopIndexVar(*node);
    } else {
        node->accept(*this);
    }
}

// -----------------------------------------------------------------
// Visitor implementations
// -----------------------------------------------------------------

void CodeGenerator::run(ValueNode& node) {
    int startLine = emitter.beginNode(node.id, node.type);
    std::visit(overloaded{
        [this](int arg) {
            emitter.appendToken(std::to_string(arg));
        },
        [this](float arg) {
            emitter.appendToken(std::to_string(arg));
        },
        [this](bool arg) {
            emitter.appendToken(arg ? "true" : "false");
        },
        [this](std::string& arg) {
            emitter.appendToken("\"" + arg + "\"");
        },
        [this, &node](std::vector<std::shared_ptr<Value>>& /*arg*/) {
            emitter.appendToken("list(");
            int i = 0;
            for (auto& e : node.inputs) {
                auto n = ctx_.getNextNodeByPin(*e);
                if (n) n->accept(*this);
                if (i != (int)node.inputs.size() - 1) emitter.appendToken(",");
                i++;
            }
            emitter.appendToken(")");
        },
        [this, &node](std::unordered_map<int, std::shared_ptr<Value>>& /*arg*/) {
            emitter.appendToken("map(");
            int i = 0;
            for (auto& e : node.inputs) {
                auto n = ctx_.getNextNodeByPin(*e);
                if (n) n->accept(*this);
                if (i != (int)node.inputs.size() - 1) emitter.appendToken(",");
                i++;
            }
            emitter.appendToken(")");
        },
        [](auto& /*arg*/) {}
    }, node.value->value.value());
    emitter.endNode(node.id, node.type, startLine);
}

void CodeGenerator::run(VariableNode& node) {
    emitter.appendToken(node.value);
}

void CodeGenerator::setVar(VariableNode& node) {
    if (!ctx_.initVariables.count(node.id)) {
        emitter.appendToken("var");
        ctx_.addInScope(node.value, std::make_shared<Value>());
    } else {
        ctx_.updateInScope(node.value, std::make_shared<Value>());
    }
    emitter.appendToken(node.value);
    ctx_.initVariables.insert(node.id);
}

void CodeGenerator::setVar(ThisNode& node, PinPtr pin) {
    if (node.inputs.empty() || !node.inputs[0]->node) return;
    auto varNode = std::static_pointer_cast<VariableNode>(
        ctx_.getNextNodeByPin(*node.inputs[0]));
    if (!varNode) return;

    if (!ctx_.initVariables.count(varNode->id)) {
        emitter.appendToken("var");
        emitter.appendToken(varNode->value);
        emitter.appendToken("=");
        emitter.appendToken(varNode->outputs[0]->type.getPinDescr()->getStr());
        emitter.appendToken("()");
        emitter.endToken();
        ctx_.addInScope(varNode->value, std::make_shared<Value>());
    }
    emitter.appendToken(varNode->value + ".");
    auto nextPin = ctx_.getNextPin(*pin);
    if (nextPin) emitter.appendToken(nextPin->name);
    ctx_.initVariables.insert(varNode->id);
}

void CodeGenerator::run(CommentNode& /*node*/) {
    // Comment nodes produce no code.
}

void CodeGenerator::run(OperatorNode& node) {
    auto leftNode  = ctx_.getNextNodeByPin(*node.inputs[0]);
    auto rightNode = ctx_.getNextNodeByPin(*node.inputs[1]);
    genCodeForExpression(leftNode,  *node.inputs[0]);
    emitter.appendToken(node.name);
    genCodeForExpression(rightNode, *node.inputs[1]);
}

void CodeGenerator::run(ClassNode& node) {
    int startLine = emitter.beginNode(node.id, node.type);
    emitter.emitDecorator(node.id);
    emitter.appendToken("class");
    emitter.appendToken(node.className);
    emitter.addStartScopeToken();
    emitter.endLineToken();
    for (auto& e : node.inputs) {
        emitter.addTabsToken();
        emitter.appendToken("var");
        emitter.appendToken(e->name);
        emitter.appendToken("=");
        auto pinNode = ctx_.getNextNodeByPin(*e);
        if (pinNode) pinNode->accept(*this);
        emitter.endToken();
    }
    emitter.addEndScopeToken();
    emitter.endToken();
    emitter.endNode(node.id, node.type, startLine);
}

void CodeGenerator::run(MethodNode& node) {
    // Resolve parent class name
    std::string className;
    auto classOpt = ctx_.getNodeById(node.classNodeId);
    if (classOpt) {
        auto classNode = std::static_pointer_cast<ClassNode>(classOpt.value());
        className      = classNode->className;
    }

    int startLine = emitter.beginNode(node.id, node.type);
    emitter.emitDecorator(node.id);
    emitter.code += "fun " + className + "." + node.funName + "(";
    bool first = true;
    for (auto& [pid, pair] : node.inputVars) {
        if (!first) emitter.code += ", ";
        emitter.code += pair.first;
        first = false;
    }
    emitter.code += ") {\n";
    ++emitter.currentLine;

    // Body: walk the flow starting from outputs[0] (after Body pin)
    if (!node.outputs.empty()) {
        auto bodyLink = ctx_.findLinkByPinId(node.outputs[0]->id);
        if (bodyLink) {
            Id otherId      = bodyLink->endPinID;
            auto otherPin   = ctx_.findPin(otherId);
            if (otherPin && otherPin->node) {
                auto bodyOpt = ctx_.getNodeById(otherPin->node->id);
                if (bodyOpt) bodyOpt.value()->accept(*this);
            }
        }
    }

    emitter.code += "}\n";
    ++emitter.currentLine;
    emitter.endNode(node.id, node.type, startLine);
}

void CodeGenerator::run(MethodCall& node) {
    auto declOpt = ctx_.getNodeById(node.methodeDeclarationId);
    if (!declOpt) return;
    auto decl = std::static_pointer_cast<MethodNode>(declOpt.value());

    // inputs[0] = object instance, inputs[1..] = args
    if (node.inputs.empty()) return;
    auto objNode = ctx_.getNextNodeByPin(*node.inputs[0]);
    if (objNode) objNode->accept(*this);
    emitter.code.back() = '.'; // replace trailing space with '.'
    emitter.code += decl->funName + "(";
    for (int i = 1; i < (int)node.inputs.size(); i++) {
        auto argNode = ctx_.getNextNodeByPin(*node.inputs[i]);
        if (argNode) argNode->accept(*this);
        if (i != (int)node.inputs.size() - 1) emitter.appendToken(",");
    }
    emitter.appendToken(")");
}

void CodeGenerator::run(ThisNode& node) {
    auto classNode = ctx_.getNextNodeByPin(*node.inputs[0]);
    if (!classNode) return;
    auto varNode = std::static_pointer_cast<VariableNode>(classNode);
    emitter.appendToken(varNode->value + ".");
}

void CodeGenerator::run(EqualNode& node) {
    int startLine = emitter.beginNode(node.id, node.type);
    emitter.emitDecorator(node.id);

    auto varNode = ctx_.getNextNodeByPin(*node.inputs[1]);
    if (varNode) {
        if (varNode->type == NodeType::Variable)
            setVar(*std::static_pointer_cast<VariableNode>(varNode));
        else if (varNode->type == NodeType::This)
            setVar(*std::static_pointer_cast<ThisNode>(varNode), node.inputs[1]);
    }
    emitter.appendToken("=");
    auto exprNode = ctx_.getNextNodeByPin(*node.inputs[2]);
    genCodeForExpression(exprNode, *node.inputs[2]);
    emitter.endToken();

    auto nextNode = ctx_.getNextNodeByPin(*node.outputs[0]);
    if (nextNode) nextNode->accept(*this);
    emitter.endNode(node.id, node.type, startLine);
}

void CodeGenerator::run(StartNode& node) {
    auto n = ctx_.getNextNodeByPin(*node.outputs[0]);
    if (n) n->accept(*this);
}

void CodeGenerator::run(FunNode& node) {
    int startLine = emitter.beginNode(node.id, node.type);
    emitter.emitDecorator(node.id);

    ctx_.pushScope();
    emitter.code += "fun " + node.funName + "(";
    bool first = true;
    for (auto& [pid, pair] : node.inputVars) {
        if (!first) emitter.code += ", ";
        emitter.code += pair.first;
        first = false;
    }
    emitter.code += ")";

    // Emit return type annotation if set
    if (node.outputType.type != PinType::None) {
        // Map PinType to type string (basic mapping)
        static auto pinTypeStr = [](PinType t) -> std::string {
            switch (t) {
            case PinType::Bool:   return "Bool";
            case PinType::Int:    return "Int";
            case PinType::Float:  return "Float";
            case PinType::String: return "String";
            default: return "";
            }
        };
        auto ts = pinTypeStr(node.outputType.type);
        if (!ts.empty()) emitter.code += ": " + ts;
    }

    emitter.code += " {\n";
    ++emitter.currentLine;

    // Body
    if (!node.outputs.empty()) {
        auto bodyLink = ctx_.findLinkByPinId(node.outputs[0]->id);
        if (bodyLink) {
            auto otherPin = ctx_.findPin(bodyLink->endPinID);
            if (otherPin && otherPin->node) {
                auto bodyOpt = ctx_.getNodeById(otherPin->node->id);
                if (bodyOpt) bodyOpt.value()->accept(*this);
            }
        }
    }

    emitter.code += "}\n";
    ++emitter.currentLine;
    ctx_.popScope();
    emitter.endNode(node.id, node.type, startLine);
}

void CodeGenerator::run(FunCall& node) {
    auto funcOpt = ctx_.getNodeById(node.funDeclarationId);
    if (!funcOpt) return;
    auto funcNode = std::static_pointer_cast<FunNode>(funcOpt.value());

    // Check if this is a statement (flow pin connected) or expression
    bool isStatement = !node.inputs.empty() &&
                       ctx_.isPinLinked(node.inputs[0]->id);

    if (isStatement) {
        int startLine = emitter.beginNode(node.id, node.type);
        emitter.emitDecorator(node.id);
    }

    emitter.appendToken(funcNode->funName);
    emitter.appendToken("(");
    for (int i = 1; i < (int)node.inputs.size(); i++) {
        auto n = ctx_.getNextNodeByPin(*node.inputs[i]);
        if (n) n->accept(*this);
        if (i != (int)node.inputs.size() - 1) emitter.appendToken(",");
    }
    emitter.appendToken(")");

    if (isStatement) {
        emitter.endToken();
        // Continue flow
        if (node.outputs.size() > 0) {
            auto nextNode = ctx_.getNextNodeByPin(*node.outputs[0]);
            if (nextNode) nextNode->accept(*this);
        }
        int startLine = emitter.currentLine; // approximate
        emitter.endNode(node.id, node.type, startLine);
    }
}

void CodeGenerator::run(ImportNode& node) {
    int startLine = emitter.beginNode(node.id, node.type);
    emitter.code += "import \"" + node.value + "\"\n";
    ++emitter.currentLine;
    emitter.endNode(node.id, node.type, startLine);
}

void CodeGenerator::run(Node& node) {
    int startLine = emitter.beginNode(node.id, node.type);

    if (node.type == NodeType::If) {
        emitter.emitDecorator(node.id);
        emitter.code += "if (";
        auto condPin  = node.inputs[1];
        auto condNode = ctx_.getNextNodeByPin(*condPin);
        genCodeForExpression(condNode, *condPin);
        emitter.code += ") {\n";
        ++emitter.currentLine;
        ctx_.pushScope();
        auto trueNode = ctx_.getNextNodeByPin(*node.outputs[0]);
        if (trueNode) trueNode->accept(*this);
        ctx_.popScope();
        emitter.code += "} else {\n";
        ++emitter.currentLine;
        ctx_.pushScope();
        auto falseNode = ctx_.getNextNodeByPin(*node.outputs[1]);
        if (falseNode) falseNode->accept(*this);
        ctx_.popScope();
        emitter.code += "}\n";
        ++emitter.currentLine;
        auto nextNode = ctx_.getNextNodeByPin(*node.outputs[2]);
        if (nextNode) nextNode->accept(*this);
    }
    else if (node.type == NodeType::For) {
        emitter.emitDecorator(node.id);
        ctx_.pushScope();
        auto indexStr = "i_" + std::to_string(node.id);
        emitter.code += "for (" + indexStr + " = ";
        auto startNode = ctx_.getNextNodeByPin(*node.inputs[1]);
        genCodeForExpression(startNode, *node.inputs[1]);
        emitter.code += "; " + indexStr + " < ";
        auto endNode = ctx_.getNextNodeByPin(*node.inputs[2]);
        genCodeForExpression(endNode, *node.inputs[2]);
        emitter.code += "; " + indexStr + " += ";
        auto stepNode = ctx_.getNextNodeByPin(*node.inputs[3]);
        genCodeForExpression(stepNode, *node.inputs[3]);
        emitter.code += ") {\n";
        ++emitter.currentLine;
        auto bodyNode = ctx_.getNextNodeByPin(*node.outputs[0]);
        if (bodyNode) bodyNode->accept(*this);
        emitter.code += "}\n";
        ++emitter.currentLine;
        ctx_.popScope();
        auto nextNode = ctx_.getNextNodeByPin(*node.outputs[1]);
        if (nextNode) nextNode->accept(*this);
    }
    else if (node.type == NodeType::While) {
        emitter.emitDecorator(node.id);
        emitter.code += "while (";
        auto condPin  = node.inputs[1];
        auto condNode = ctx_.getNextNodeByPin(*condPin);
        genCodeForExpression(condNode, *condPin);
        emitter.code += ") {\n";
        ++emitter.currentLine;
        ctx_.pushScope();
        auto bodyNode = ctx_.getNextNodeByPin(*node.outputs[0]);
        if (bodyNode) bodyNode->accept(*this);
        ctx_.popScope();
        emitter.code += "}\n";
        ++emitter.currentLine;
        auto nextNode = ctx_.getNextNodeByPin(*node.outputs[1]);
        if (nextNode) nextNode->accept(*this);
    }
    else if (node.type == NodeType::Return) {
        emitter.emitDecorator(node.id);
        emitter.appendToken("return");
        if (node.inputs.size() > 1) {
            auto exprNode = ctx_.getNextNodeByPin(*node.inputs[1]);
            genCodeForExpression(exprNode, *node.inputs[1]);
        }
        emitter.endToken();
    }
    else if (node.type == NodeType::Continue) {
        emitter.appendToken("continue");
        emitter.endToken();
    }
    else if (node.type == NodeType::Break) {
        emitter.appendToken("break");
        emitter.endToken();
    }

    emitter.endNode(node.id, node.type, startLine);
}

// -----------------------------------------------------------------
// Top-level compile
// -----------------------------------------------------------------

CompileResult Visual::compileGraph(GraphContext& ctx, const std::string& graphName) {
    ctx.resetScope();
    CodeGenerator gen(ctx);
    gen.emitter.map.graphName = graphName;

    // Emit source-map header placeholder (will be replaced after generation)
    gen.emitter.code = "";
    gen.emitter.currentLine = 2; // first real line after the header comment

    // 1. Class declarations
    for (auto& [id, node] : ctx.nodes)
        if (node->type == NodeType::ClassDeclaration) node->accept(gen);

    // 2. Function declarations
    for (auto& [id, node] : ctx.nodes)
        if (node->type == NodeType::FunctionDeclaration) node->accept(gen);

    // 3. Main flow from StartNode
    if (ctx.startNode) ctx.startNode->accept(gen);

    // Prepend header comment
    auto header = gen.emitter.map.toHeaderComment() + "\n";
    gen.emitter.code = header + gen.emitter.code;

    return {gen.emitter.code, gen.emitter.map};
}
