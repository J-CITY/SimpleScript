#include "GraphLoader.hpp"
#include "../Core/GraphContext.hpp"
#include <algorithm>
#include <fstream>
#include "../../json.hpp"

using namespace Visual;
using json = nlohmann::json;

static PinSubtype readSubtype(json& js) {
    if (!js.contains("subtype")) return std::nullopt;
    auto type = js["subtype"]["type"]["type"].get<std::string>();
    if (type == "string")
        return js["subtype"]["type"]["val"].get<std::string>();
    return std::make_shared<PinTypeDescriptor>(
        PinTypeDescriptor{(PinType)js["subtype"]["type"]["val"].get<int>()});
}

static void addPins(GraphContext& ctx, NodePtr node, json& e, int& maxId) {
    for (auto& pin : e["outputs"]) {
        auto id = pin["id"].get<int>();
        maxId   = std::max(maxId, id);
        ctx.createPin(id, pin["name"].get<std::string>(),
                      PinTypeDescriptor{(PinType)pin["type"]["type"].get<int>(),
                                        readSubtype(pin["type"])},
                      node.get(), PinKind::Output);
    }
    for (auto& pin : e["inputs"]) {
        auto id = pin["id"].get<int>();
        maxId   = std::max(maxId, id);
        ctx.createPin(id, pin["name"].get<std::string>(),
                      PinTypeDescriptor{(PinType)pin["type"]["type"].get<int>(),
                                        readSubtype(pin["type"])},
                      node.get(), PinKind::Input);
    }
}

static void applyPos(NodePtr node, json& e) {
    if (e.contains("pos"))
        node->layoutPosition = {e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()};
}

int GraphLoader::load(GraphContext& ctx, std::string_view path) {
    ctx.clear();

    auto pathStr = std::string(path);
    std::ifstream f;
    f.open(pathStr);
    if (!f.good()) return 1;
    json data = json::parse(f);

    int maxId = 1;

    for (auto& e : data["nodes"]) {
        auto id = e["id"].get<int>();
        maxId   = std::max(maxId, id);
        int typeInt = e["type"].get<int>();

        // Value nodes
        if (typeInt == (int)NodeType::ValueBool  || typeInt == (int)NodeType::ValueInt   ||
            typeInt == (int)NodeType::ValueFloat  || typeInt == (int)NodeType::ValueString ||
            typeInt == (int)NodeType::ValueArrayBool || typeInt == (int)NodeType::ValueArrayInt ||
            typeInt == (int)NodeType::ValueArrayFloat || typeInt == (int)NodeType::ValueList ||
            typeInt == (int)NodeType::ValueMap) {

            Value val;
            if (e.contains("value")) {
                if      (e["value"].is_boolean())       val.value = e["value"].get<bool>();
                else if (e["value"].is_number_integer()) val.value = e["value"].get<int>();
                else if (e["value"].is_number_float())  val.value = e["value"].get<float>();
                else if (e["value"].is_string())        val.value = e["value"].get<std::string>();
            }
            if (typeInt == (int)NodeType::ValueList) val.value = std::vector<std::shared_ptr<Value>>{};
            if (typeInt == (int)NodeType::ValueMap)  val.value = std::unordered_map<int, std::shared_ptr<Value>>{};

            auto node = ctx.createNode<ValueNode>(id,
                e["name"].get<std::string>(), (NodeType)typeInt, val);
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::Variable) {
            auto node = ctx.createNode<VariableNode>(id, "Variable",
                e["value"].get<std::string>());
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::FunctionDeclaration) {
            auto node = ctx.createNode<FunNode>(id, e["name"].get<std::string>());
            addPins(ctx, node, e, maxId);
            for (auto& pinJ : e["inputVars"]) {
                node->inputVars[pinJ["id"].get<int>()] = {
                    pinJ["name"].get<std::string>(),
                    PinTypeDescriptor{(PinType)pinJ["type"]["type"].get<int>(), readSubtype(pinJ["type"])}
                };
            }
            if (e.contains("outputType"))
                node->outputType = PinTypeDescriptor{(PinType)e["outputType"]["type"]["type"].get<int>(),
                                                      readSubtype(e["outputType"]["type"])};
            applyPos(node, e);
            ctx.registerFunctionCall(node);
        }
        else if (typeInt == (int)NodeType::FunctionCall) {
            auto node = ctx.createNode<FunCall>(id, e["funDeclarationId"].get<int>());
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::Operator) {
            auto node = ctx.createNode<OperatorNode>(id,
                e["name"].get<std::string>(), (OperatorType)e["opType"].get<int>());
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::Import) {
            auto node = ctx.createNode<ImportNode>(id,
                e["name"].get<std::string>(), e["value"].get<std::string>());
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::Comment) {
            auto node = ctx.createNode<CommentNode>(id, e["value"].get<std::string>());
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::ClassDeclaration) {
            auto node = ctx.createNode<ClassNode>(id, e["name"].get<std::string>());
            node->className = e["className"].get<std::string>();
            for (auto& mv : e.value("memberVars", json::array())) {
                ClassNode::Member m{mv["name"].get<std::string>(),
                    PinTypeDescriptor{(PinType)mv["type"]["type"].get<int>(), readSubtype(mv["type"])}};
                node->memberVars[mv["id"].get<int>()] = m;
            }
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
            ctx.registerClassCall(node);
        }
        else if (typeInt == (int)NodeType::ClassMethodDeclaration) {
            auto node = ctx.createNode<MethodNode>(id, e["name"].get<std::string>(),
                e["classNodeId"].get<int>());
            node->funName = e["funName"].get<std::string>();
            for (auto& pinJ : e.value("inputVars", json::array())) {
                node->inputVars[pinJ["id"].get<int>()] = {
                    pinJ["name"].get<std::string>(),
                    PinTypeDescriptor{(PinType)pinJ["type"]["type"].get<int>(), readSubtype(pinJ["type"])}
                };
            }
            if (e.contains("outputType"))
                node->outputType = PinTypeDescriptor{(PinType)e["outputType"]["type"]["type"].get<int>()};
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
            ctx.registerMethodCall(node);
        }
        else if (typeInt == (int)NodeType::ClassMethodCall) {
            auto node = ctx.createNode<MethodCall>(id, e["methodeDeclarationId"].get<int>());
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::This) {
            auto node = ctx.createNode<ThisNode>(id, e["classId"].get<int>());
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::Equal) {
            auto node = ctx.createNode<EqualNode>(id);
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
        else if (typeInt == (int)NodeType::Start) {
            auto node = ctx.createNode<StartNode>(id);
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
            ctx.startNode = node;
        }
        else {
            // Generic node (If, For, While, Return, Continue, Break, ForEach)
            auto node = ctx.createNode<Node>(id, e["name"].get<std::string>(),
                (NodeType)typeInt);
            addPins(ctx, node, e, maxId);
            applyPos(node, e);
        }
    }

    for (auto& lk : data["links"]) {
        auto id    = lk["id"].get<int>();
        auto start = lk["start"].get<int>();
        auto end   = lk["end"].get<int>();
        maxId      = std::max(maxId, id);
        ctx.createLink(id, start, end);
    }

    return maxId;
}
