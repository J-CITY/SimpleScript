// Include core headers before imgui to avoid Windows.h macro pollution.
#include "NodeDrawer.hpp"
#include "../Core/GraphContext.hpp"
#define NOMINMAX
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_node_editor.h>

using namespace Visual;

NodeDrawer::NodeDrawer(GraphContext& ctx) : ctx_(ctx) {}

// -----------------------------------------------------------------
// Helpers (local to this translation unit)
// -----------------------------------------------------------------

static bool drawCombo(const char* id, int& current, const std::vector<std::string>& items) {
    static bool doPopup = false;
    if (ImGui::Button(items[current].c_str())) doPopup = true;
    ax::NodeEditor::Suspend();
    if (doPopup) { ImGui::OpenPopup(id); doPopup = false; }
    bool changed = false;
    if (ImGui::BeginPopup(id)) {
        ImGui::TextDisabled("Pick One:");
        ImGui::BeginChild("popup_scroller", ImVec2(100, 100), true,
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);
        for (int i = 0; i < (int)items.size(); i++) {
            if (ImGui::Button(items[i].c_str())) {
                current = i; changed = true;
                ImGui::CloseCurrentPopup();
                ImGui::EndChild(); ImGui::EndPopup();
                ax::NodeEditor::Resume();
                return changed;
            }
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
    ax::NodeEditor::Resume();
    return changed;
}

static std::vector<std::string> getBaseTypes()       { return {"Bool","Int","Float","String"}; }
static std::vector<std::string> getContainerTypes()  { return {"ArrayBool","ArrayInt","ArrayFloat","List","Map"}; }

static std::vector<std::string> getClassTypes(GraphContext& ctx) {
    std::vector<std::string> res;
    for (auto& [id, cn] : ctx.classNodes) res.push_back(cn->className);
    return res;
}

static std::vector<std::string> getAllTypesStrs(GraphContext& ctx, bool withNone) {
    std::vector<std::string> res;
    if (withNone) res.push_back("none");
    for (auto& t : getBaseTypes())      res.push_back(t);
    for (auto& t : getContainerTypes()) res.push_back(t);
    for (auto& t : getClassTypes(ctx))  res.push_back(t);
    return res;
}

static PinTypeDescriptor stringToPinType(const std::string& s) {
    if (s == "Bool")      return {PinType::Bool};
    if (s == "Int")       return {PinType::Int};
    if (s == "Float")     return {PinType::Float};
    if (s == "String")    return {PinType::String};
    if (s == "ArrayBool") return {PinType::ArrayBool};
    if (s == "ArrayInt")  return {PinType::ArrayInt};
    if (s == "ArrayFloat")return {PinType::ArrayFloat};
    if (s == "List")      return {PinType::List};
    if (s == "Map")       return {PinType::Map};
    if (s == "Any")       return {PinType::Any};
    if (s.rfind("Class", 0) == 0) return {PinType::Object, s.substr(5)};
    return {PinType::None};
}

static int pinTypeToInt(const PinTypeDescriptor& type, GraphContext& ctx) {
    if ((int)type.type >= (int)PinType::Object) {
        int i = 0;
        for (auto& [id, cn] : ctx.classNodes) {
            if (type.subType && std::holds_alternative<std::string>(type.subType.value()))
                if (cn->className == std::get<std::string>(type.subType.value())) break;
            i++;
        }
        return (int)type.type + i;
    }
    return (int)type.type;
}

// -----------------------------------------------------------------
// Visitor implementations
// -----------------------------------------------------------------

void NodeDrawer::run(ValueNode& node) {
    ImGui::PushItemWidth(150.f);
    if (!node.value || !node.value->value) { ImGui::PopItemWidth(); return; }
    std::visit(overloaded{
        [](int& v)         { ImGui::Text("Int");   ImGui::InputInt("##iv", &v); },
        [](float& v)       { ImGui::Text("Float"); ImGui::InputFloat("##fv", &v); },
        [](bool& v)        { ImGui::Text("Bool");  ImGui::Checkbox("##bv", &v); },
        [](std::string& v) { ImGui::Text("String");ImGui::InputText("##sv", &v); },
        [&, this](std::vector<std::shared_ptr<Value>>& /*v*/) {
            ImGui::Text("List");
            if (ImGui::Button("Add"))
                ctx_.createPin(IdGenerator::GenId(), "Value", {PinType::Any}, &node, PinKind::Input);
            ImGui::SameLine();
            if (ImGui::Button("Del") && !node.inputs.empty()) {
                ctx_.deletePin(node.inputs.back()->id);
                node.inputs.pop_back();
            }
        },
        [&, this](std::unordered_map<int, std::shared_ptr<Value>>& /*v*/) {
            ImGui::Text("Map");
            if (ImGui::Button("Add")) {
                ctx_.createPin(IdGenerator::GenId(), "Key",   {PinType::Any}, &node, PinKind::Input);
                ctx_.createPin(IdGenerator::GenId(), "Value", {PinType::Any}, &node, PinKind::Input);
            }
            ImGui::SameLine();
            if (ImGui::Button("Del") && node.inputs.size() >= 2) {
                ctx_.deletePin(node.inputs.back()->id); node.inputs.pop_back();
                ctx_.deletePin(node.inputs.back()->id); node.inputs.pop_back();
            }
        },
        [](auto& /*v*/) {}
    }, node.value->value.value());
    ImGui::PopItemWidth();
}

void NodeDrawer::run(VariableNode& node) {
    ImGui::PushItemWidth(150.f);
    ImGui::Text("Variable");
    ImGui::InputText("##var", &node.value);
    ImGui::PopItemWidth();
}

void NodeDrawer::run(Node& node) {
    ImGui::Text("%s", node.name.c_str());
}

void NodeDrawer::run(FunNode& node) {
    ImGui::Text("%s", node.name.c_str());
    ImGui::PushItemWidth(150.f);
    ImGui::InputText("##fn", &node.funName);

    auto items = getAllTypesStrs(ctx_, true);
    int item   = pinTypeToInt(node.outputType, ctx_);
    if (drawCombo("##CRT", item, items)) node.outputType = stringToPinType(items[item]);

    if (ImGui::Button("Add")) {
        auto id = IdGenerator::GenId();
        node.inputVars[id] = {"", {PinType::Bool}};
        ctx_.createPin(id, "", {PinType::Bool}, &node, PinKind::Output);
    }
    ImGui::PopItemWidth();

    int i = 0;
    for (auto e = node.inputVars.begin(); e != node.inputVars.end();) {
        ImGui::PushID(("iv" + std::to_string(i)).c_str());
        bool del = false;
        if (ImGui::Button("X")) del = true;
        ImGui::SameLine();
        auto vname = e->second.first;
        ImGui::PushItemWidth(100.f);
        if (ImGui::InputText("##vn", &vname)) {
            for (auto& p : node.outputs) if (p->id == e->first) p->name = vname;
            e->second.first = vname;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        auto allT  = getAllTypesStrs(ctx_, false);
        int  tItem = pinTypeToInt(e->second.second, ctx_) - 1;
        if (drawCombo("##CB", tItem, allT)) {
            auto newT = stringToPinType(allT[tItem]);
            for (auto& p : node.outputs) if (p->id == e->first) p->type = newT;
            e->second.second = newT;
        }
        if (del) {
            std::erase_if(node.outputs, [_id = e->first](auto& p){ return p->id == _id; });
            ctx_.deletePin(e->first);
            e = node.inputVars.erase(e);
        } else { e++; }
        ImGui::PopID();
        i++;
    }
}

void NodeDrawer::run(FunCall& node) {
    auto opt = ctx_.getNodeById(node.funDeclarationId);
    if (opt) ImGui::Text("%s", std::static_pointer_cast<FunNode>(opt.value())->funName.c_str());
}

void NodeDrawer::run(ImportNode& node) {
    ImGui::PushItemWidth(150.f);
    ImGui::Text("Import");
    ImGui::InputText("##imp", &node.value);
    ImGui::PopItemWidth();
}

void NodeDrawer::run(CommentNode& node) {
    const float width = node.size.x > 0.f ? node.size.x : 200.f;
    ImGui::PushItemWidth(width);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImVec4(0,0,0,0)));
    ImGui::InputText("##cmt", &node.value);
    ImGui::PopStyleColor();
    ImGui::PopItemWidth();
    ax::NodeEditor::Group(node.size.x > 0 ? ImVec2(node.size.x, node.size.y) : ImVec2(200, 100));
}

void NodeDrawer::run(OperatorNode& node) { ImGui::Text("%s", node.name.c_str()); }
void NodeDrawer::run(EqualNode&    node) { ImGui::Text("%s", node.name.c_str()); }
void NodeDrawer::run(StartNode&    node) { ImGui::Text("%s", node.name.c_str()); }
void NodeDrawer::run(MethodCall&   node) { ImGui::Text("%s", node.name.c_str()); }
void NodeDrawer::run(ThisNode&     node) { ImGui::Text("%s", node.name.c_str()); }

void NodeDrawer::run(ClassNode& node) {
    ImGui::Text("%s", node.name.c_str());
    ImGui::PushItemWidth(150.f);
    ImGui::InputText("##cn", &node.className);
    if (ImGui::Button("Add Member")) {
        auto id = IdGenerator::GenId();
        node.memberVars[id] = {"", {PinType::Bool}};
        ctx_.createPin(id, "", {PinType::Variable, std::make_shared<PinTypeDescriptor>(PinType::Bool)},
                       &node, PinKind::Input);
    }
    ImGui::PopItemWidth();
    int i = 0;
    for (auto e = node.memberVars.begin(); e != node.memberVars.end();) {
        ImGui::PushID(("mv" + std::to_string(i)).c_str());
        bool del = false;
        if (ImGui::Button("X")) del = true;
        ImGui::SameLine();
        auto vname = e->second.name;
        ImGui::PushItemWidth(100.f);
        if (ImGui::InputText("##mvn", &vname)) {
            for (auto& p : node.inputs) if (p->id == e->first) p->name = vname;
            e->second.name = vname;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        auto allT  = getAllTypesStrs(ctx_, false);
        int  tItem = pinTypeToInt(e->second.type, ctx_) - 1;
        if (drawCombo("##CB", tItem, allT)) {
            auto newT = stringToPinType(allT[tItem]);
            for (auto& p : node.inputs) if (p->id == e->first)
                p->type.subType = std::make_shared<PinTypeDescriptor>(newT);
            e->second.type.subType = std::make_shared<PinTypeDescriptor>(newT);
        }
        if (del) {
            std::erase_if(node.outputs, [_id = e->first](auto& p){ return p->id == _id; });
            ctx_.deletePin(e->first);
            e = node.memberVars.erase(e);
        } else { e++; }
        ImGui::PopID();
        i++;
    }
}

void NodeDrawer::run(MethodNode& node) {
    ImGui::Text("%s", node.name.c_str());
    ImGui::PushItemWidth(150.f);
    ImGui::InputText("##mfn", &node.funName);
    auto items = getAllTypesStrs(ctx_, true);
    int item   = pinTypeToInt(node.outputType, ctx_);
    if (drawCombo("##CRT", item, items)) node.outputType = stringToPinType(items[item]);
    if (ImGui::Button("Add")) {
        auto id = IdGenerator::GenId();
        node.inputVars[id] = {"", {PinType::Bool}};
        ctx_.createPin(id, "", {PinType::Bool}, &node, PinKind::Output);
    }
    ImGui::PopItemWidth();
    int i = 0;
    for (auto e = node.inputVars.begin(); e != node.inputVars.end();) {
        ImGui::PushID(("miv" + std::to_string(i)).c_str());
        bool del = false;
        if (ImGui::Button("X")) del = true;
        ImGui::SameLine();
        auto vname = e->second.first;
        ImGui::PushItemWidth(100.f);
        if (ImGui::InputText("##vn", &vname)) {
            for (auto& p : node.outputs) if (p->id == e->first) p->name = vname;
            e->second.first = vname;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        auto allT  = getAllTypesStrs(ctx_, false);
        int  tItem = pinTypeToInt(e->second.second, ctx_) - 1;
        if (drawCombo("##CB", tItem, allT)) {
            auto newT = stringToPinType(allT[tItem]);
            for (auto& p : node.outputs) if (p->id == e->first) p->type = newT;
            e->second.second = newT;
        }
        if (del) {
            std::erase_if(node.outputs, [_id = e->first](auto& p){ return p->id == _id; });
            ctx_.deletePin(e->first);
            e = node.inputVars.erase(e);
        } else { e++; }
        ImGui::PopID();
        i++;
    }
}
