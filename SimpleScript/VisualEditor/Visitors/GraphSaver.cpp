#include "GraphSaver.hpp"
#include "../Core/GraphContext.hpp"
#include <fstream>
#include "../../json.hpp"

using namespace Visual;
using json = nlohmann::json;

struct GraphSaver::Impl {
    json data;
    Impl() { data["nodes"] = json::array(); data["links"] = json::array(); }
};

GraphSaver::GraphSaver(GraphContext& ctx)
    : ctx_(ctx), impl_(std::make_unique<Impl>()) {}

GraphSaver::~GraphSaver() = default;

// -----------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------

static void saveType(json& pn, const PinTypeDescriptor& type) {
    pn["type"]["type"] = (int)type.type;
    if (type.subType) {
        if (std::holds_alternative<std::string>(type.subType.value())) {
            pn["type"]["subtype"]["type"] = "string";
            pn["type"]["subtype"]["val"]  = std::get<std::string>(type.subType.value());
        } else {
            auto descr = std::get<std::shared_ptr<PinTypeDescriptor>>(type.subType.value());
            pn["type"]["subtype"]["type"]["type"] = "int";
            pn["type"]["subtype"]["type"]["val"]  = (int)descr->type;
        }
    }
}

void GraphSaver::saveBaseImpl(Impl& impl, Node& node) {
    json nd;
    nd["pos"]["x"] = node.layoutPosition.x;
    nd["pos"]["y"] = node.layoutPosition.y;
    nd["type"]     = (int)node.type;
    nd["id"]       = node.id;
    nd["name"]     = node.name;
    nd["color"]    = {node.color.r, node.color.g, node.color.b, node.color.a};
    nd["inputs"]   = json::array();
    nd["outputs"]  = json::array();
    for (auto& e : node.inputs) {
        json pn;
        pn["id"]   = e->id;
        pn["name"] = e->name;
        saveType(pn, e->type);
        nd["inputs"].push_back(pn);
    }
    for (auto& e : node.outputs) {
        json pn;
        pn["id"]   = e->id;
        pn["name"] = e->name;
        saveType(pn, e->type);
        nd["outputs"].push_back(pn);
    }
    impl.data["nodes"].push_back(nd);
}

void GraphSaver::saveLinks(Impl& impl) {
    for (auto& [id, link] : ctx_.links) {
        json lk;
        lk["id"]    = link->id;
        lk["start"] = link->startPinID;
        lk["end"]   = link->endPinID;
        impl.data["links"].push_back(lk);
    }
}

bool GraphSaver::save(std::string_view path) {
    for (auto& [id, node] : ctx_.nodes)
        node->accept(*this);
    saveLinks(*impl_);
    try {
        std::ofstream f;
        f.open(std::string(path));
        if (!f.good()) return false;
        auto str = impl_->data.dump(4);
        f.write(str.data(), (std::streamsize)str.size());
        return f.good();
    } catch (...) { return false; }
}

// -----------------------------------------------------------------
// Visitor implementations
// -----------------------------------------------------------------

void GraphSaver::run(ValueNode& node) {
    if (node.value && node.value->value) {
        // We need to build a json entry manually; saveBaseImpl adds to nodes array
        // but we need to add the value field first.
        // Work around: call saveBaseImpl then patch the last entry.
        saveBaseImpl(*impl_, node);
        auto& nd = impl_->data["nodes"].back();
        std::visit(overloaded{
            [&nd](int arg)          { nd["value"] = arg; },
            [&nd](float arg)        { nd["value"] = arg; },
            [&nd](bool arg)         { nd["value"] = arg; },
            [&nd](std::string& arg) { nd["value"] = arg; },
            [](auto& /*arg*/)       {}
        }, node.value->value.value());
    } else {
        saveBaseImpl(*impl_, node);
    }
}

void GraphSaver::run(VariableNode& node) {
    saveBaseImpl(*impl_, node);
    impl_->data["nodes"].back()["value"] = node.value;
}

void GraphSaver::run(Node& node) {
    saveBaseImpl(*impl_, node);
}

void GraphSaver::run(FunNode& node) {
    saveBaseImpl(*impl_, node);
    auto& nd = impl_->data["nodes"].back();
    nd["funName"]   = node.funName;
    nd["inputVars"] = json::array();
    for (auto& [pid, pair] : node.inputVars) {
        json pn;
        pn["id"]   = pid;
        pn["name"] = pair.first;
        saveType(pn, pair.second);
        nd["inputVars"].push_back(pn);
    }
    saveType(nd["outputType"], node.outputType);
}

void GraphSaver::run(FunCall& node) {
    saveBaseImpl(*impl_, node);
    impl_->data["nodes"].back()["funDeclarationId"] = node.funDeclarationId;
}

void GraphSaver::run(ImportNode& node) {
    saveBaseImpl(*impl_, node);
    impl_->data["nodes"].back()["value"] = node.value;
}

void GraphSaver::run(CommentNode& node) {
    saveBaseImpl(*impl_, node);
    impl_->data["nodes"].back()["value"] = node.value;
}

void GraphSaver::run(OperatorNode& node) {
    saveBaseImpl(*impl_, node);
    impl_->data["nodes"].back()["opType"] = (int)node.opType;
}

void GraphSaver::run(ClassNode& node) {
    saveBaseImpl(*impl_, node);
    auto& nd      = impl_->data["nodes"].back();
    nd["className"]  = node.className;
    nd["memberVars"] = json::array();
    for (auto& [pid, member] : node.memberVars) {
        json pn;
        pn["id"]   = pid;
        pn["name"] = member.name;
        saveType(pn, member.type);
        nd["memberVars"].push_back(pn);
    }
}

void GraphSaver::run(MethodNode& node) {
    saveBaseImpl(*impl_, node);
    auto& nd      = impl_->data["nodes"].back();
    nd["funName"]    = node.funName;
    nd["classNodeId"]= node.classNodeId;
    nd["inputVars"]  = json::array();
    for (auto& [pid, pair] : node.inputVars) {
        json pn;
        pn["id"]   = pid;
        pn["name"] = pair.first;
        saveType(pn, pair.second);
        nd["inputVars"].push_back(pn);
    }
    saveType(nd["outputType"], node.outputType);
}

void GraphSaver::run(MethodCall& node) {
    saveBaseImpl(*impl_, node);
    impl_->data["nodes"].back()["methodeDeclarationId"] = node.methodeDeclarationId;
}

void GraphSaver::run(ThisNode& node) {
    saveBaseImpl(*impl_, node);
    impl_->data["nodes"].back()["classId"] = node.classId;
}

void GraphSaver::run(EqualNode& node) {
    saveBaseImpl(*impl_, node);
}

void GraphSaver::run(StartNode& node) {
    saveBaseImpl(*impl_, node);
}
