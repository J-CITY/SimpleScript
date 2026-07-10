// Include IkigaiScript BEFORE any platform/imgui headers so Windows.h macros
// (e.g. #define Yield) do not pollute the IkigaiScript namespace.
#include "../../Library/ikigaiScript.h"
#include "../Visitors/CodeGenerator.hpp"
#include "../Visitors/GraphLoader.hpp"
#include "../Visitors/GraphSaver.hpp"
#include "../Visitors/GraphValidator.hpp"

#define NOMINMAX
#define IMGUI_DEFINE_MATH_OPERATORS
#include "VisualCodeEditor.hpp"
#include "ImGuiTypes.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <variant>

#include "../../imgui/imgui.h"
#include "../../imgui/imgui_internal.h"
#include "../../imgui/misc/cpp/imgui_stdlib.h"
#include "../../imgui/TextEditor.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "../../stb_image.h"
#include "../../imgui/imgui_impl_opengl3_loader.h"

using namespace Visual;
using namespace Visual::UI;

// -----------------------------------------------------------------
// Icon drawing (ported from original VisualCodeEditor.cpp)
// -----------------------------------------------------------------

enum class IconType : ImU32 { Flow, Circle, Square, Grid, RoundSquare, Diamond };

static void DrawIconImpl(ImDrawList* drawList, const ImVec2& a, const ImVec2& b,
                         IconType type, bool filled, ImU32 color, ImU32 innerColor) {
    auto rect = ImRect(a, b);
    auto rect_center_x = (rect.Min.x + rect.Max.x) * 0.5f;
    auto rect_center_y = (rect.Min.y + rect.Max.y) * 0.5f;
    auto rect_center   = ImVec2(rect_center_x, rect_center_y);
    auto rect_w        = rect.Max.x - rect.Min.x;
    auto rect_h        = rect.Max.y - rect.Min.y;
    const auto outline_scale  = rect_w / 24.0f;
    const auto extra_segments = static_cast<int>(2 * outline_scale);

    if (type == IconType::Circle) {
        const auto r = 0.5f * rect_w / 2.0f - 0.5f;
        if (!filled) {
            if (innerColor & 0xFF000000) drawList->AddCircleFilled(rect_center, r, innerColor, 12 + extra_segments);
            drawList->AddCircle(rect_center, r, color, 12 + extra_segments, 2.f * outline_scale);
        } else {
            drawList->AddCircleFilled(rect_center, 0.5f * rect_w / 2.0f, color, 12 + extra_segments);
        }
    } else if (type == IconType::Flow) {
        const auto origin_scale = rect_w / 24.0f;
        const auto offset_x = 1.0f * origin_scale;
        const auto margin    = 2.0f * origin_scale;
        const auto rounding  = 0.1f * origin_scale;
        const auto tip_round = 0.7f;
        const auto canvas = ImRect(rect.Min.x + margin + offset_x, rect.Min.y + margin,
                                   rect.Max.x - margin + offset_x, rect.Max.y - margin);
        const auto left  = canvas.Min.x + (canvas.Max.x - canvas.Min.x) * 0.5f * 0.3f;
        const auto right = canvas.Max.x - (canvas.Max.x - canvas.Min.x) * 0.5f * 0.3f;
        const auto top   = canvas.Min.y + (canvas.Max.y - canvas.Min.y) * 0.5f * 0.2f;
        const auto bottom= canvas.Max.y - (canvas.Max.y - canvas.Min.y) * 0.5f * 0.2f;
        const auto center_y = (top + bottom) * 0.5f;
        const auto tip_top   = ImVec2(canvas.Min.x + (canvas.Max.x - canvas.Min.x) * 0.5f, top);
        const auto tip_right = ImVec2(right, center_y);
        const auto tip_bottom= ImVec2(canvas.Min.x + (canvas.Max.x - canvas.Min.x) * 0.5f, bottom);
        drawList->PathLineTo(ImVec2(left, top) + ImVec2(0, rounding));
        drawList->PathBezierCubicCurveTo(ImVec2(left, top), ImVec2(left, top), ImVec2(left, top) + ImVec2(rounding, 0));
        drawList->PathLineTo(tip_top);
        drawList->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
        drawList->PathBezierCubicCurveTo(tip_right, tip_right, tip_bottom + (tip_right - tip_bottom) * tip_round);
        drawList->PathLineTo(tip_bottom);
        drawList->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
        drawList->PathBezierCubicCurveTo(ImVec2(left, bottom), ImVec2(left, bottom), ImVec2(left, bottom) - ImVec2(0, rounding));
        if (!filled) {
            if (innerColor & 0xFF000000) drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);
            drawList->PathStroke(color, true, 2.0f * outline_scale);
        } else drawList->PathFillConvex(color);
    } else {
        // Square / default
        const auto r = 0.5f * rect_w / 2.0f;
        if (filled) {
            drawList->AddRectFilled(rect_center - ImVec2(r,r), rect_center + ImVec2(r,r), color);
        } else {
            if (innerColor & 0xFF000000)
                drawList->AddRectFilled(rect_center - ImVec2(r,r), rect_center + ImVec2(r,r), innerColor);
            drawList->AddRect(rect_center - ImVec2(r,r), rect_center + ImVec2(r,r), color, 0.f, 0, 2.f * outline_scale);
        }
    }
}

static void DrawIcon(const ImVec2& size, IconType type, bool filled,
                     const ImVec4& color, const ImVec4& innerColor) {
    if (!ImGui::IsRectVisible(size)) { ImGui::Dummy(size); return; }
    auto cursorPos = ImGui::GetCursorScreenPos();
    auto drawList  = ImGui::GetWindowDrawList();
    DrawIconImpl(drawList, cursorPos, cursorPos + size, type, filled,
                 ImColor(color), ImColor(innerColor));
    ImGui::Dummy(size);
}

// -----------------------------------------------------------------
// GetIconColor / drawPinIcon helpers
// -----------------------------------------------------------------

static ImColor GetIconColor(PinType type) {
    switch (type) {
    case PinType::Flow:   return ImColor(255,255,255);
    case PinType::Bool:   return ImColor(220,48,48);
    case PinType::Int:    return ImColor(68,201,156);
    case PinType::Float:  return ImColor(147,226,74);
    case PinType::String: return ImColor(124,21,153);
    case PinType::Object: return ImColor(51,150,215);
    case PinType::Function:return ImColor(218,0,183);
    default:              return ImColor(128,128,128);
    }
}

void VisualCodeEditor::drawPinIcon(const Pin& pin, bool connected, int alpha) {
    const int m_PinIconSize = 24;
    IconType iconType = IconType::Circle;
    ImColor  color    = GetIconColor(pin.type.type);
    color.Value.w     = alpha / 255.0f;
    if (pin.type.type == PinType::Flow) iconType = IconType::Flow;

    DrawIcon(ImVec2((float)m_PinIconSize, (float)m_PinIconSize), iconType, connected,
             color.Value, ImVec4(0,0,0,0));
}

// -----------------------------------------------------------------
// Type-compatibility helpers (moved from VisualCodeEditor.cpp)
// -----------------------------------------------------------------

static bool tryConvert(PinTypeDescriptor from, PinTypeDescriptor to) {
    if (from.type == to.type) {
        if (from.type == PinType::None)   return false;
        if (from.type == PinType::Object) {
            if (!from.subType || !to.subType) return from.subType == to.subType;
            return std::get<std::string>(from.subType.value()) ==
                   std::get<std::string>(to.subType.value());
        }
        return true;
    }
    if (to.type   == PinType::Any) return true;
    if (to.type   == PinType::Flow|| from.type == PinType::Flow) return false;
    if (to.type   == PinType::Variable) {
        if (!to.subType) return true;
        return tryConvert(from, {to.getPinDescr()->type});
    }
    if (from.type == PinType::Variable) {
        if (!from.subType) return true;
        return tryConvert({from.getPinDescr()->type}, to);
    }
    if (to.type == PinType::Container)
        return from.type==PinType::List || from.type==PinType::Map ||
               from.type==PinType::ArrayInt || from.type==PinType::ArrayFloat || from.type==PinType::ArrayBool;
    return false;
}

static bool checkTypes(const Pin& from, const Pin& to) {
    return tryConvert(from.type, to.type);
}

static void setInputPinsType(Node* node, PinTypeDescriptor t) {
    for (auto& p : node->inputs) p->type = t;
}

static bool checkInputTypeForOperator(Node* node, PinTypeDescriptor t) {
    if (node->type != NodeType::Operator) return true;
    return t.type == PinType::Bool || t.type == PinType::Int ||
           t.type == PinType::Float || t.type == PinType::String;
}

static void updatePinsForThisNode(ThisNode& thisNode, VariableNode& varNode) {
    // resolve class node to add member pins to ThisNode
    // (simplified: clear extra output pins and re-add from class definition)
    // Full implementation would iterate classNodes
}

static void updatePinsAfterDeleteLink(const Link& link) {}

// -----------------------------------------------------------------
// Texture loading (for background)
// -----------------------------------------------------------------

struct OpenGLTexture { GLuint TextureID = 0; };
static OpenGLTexture CreateTexture(const void* data, int width, int height) {
    OpenGLTexture tex;
    glGenTextures(1, &tex.TextureID);
    glBindTexture(GL_TEXTURE_2D, tex.TextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return tex;
}
static ImTextureID LoadTexture(const char* path) {
    int w=0, h=0, c=0;
    if (auto data = stbi_load(path, &w, &h, &c, 4)) {
        auto tex = CreateTexture(data, w, h);
        stbi_image_free(data);
        return reinterpret_cast<ImTextureID>(static_cast<std::intptr_t>(tex.TextureID));
    }
    return nullptr;
}
static ImTextureID HeaderBackground = nullptr;

// -----------------------------------------------------------------
// VisualCodeEditor implementation
// -----------------------------------------------------------------

VisualCodeEditor::VisualCodeEditor()
    : ctx_(GraphContext::Instance()), session_(), drawer_(ctx_) {
    initSideBar();
}

void VisualCodeEditor::addButton(const std::string& group, const std::string& name,
                                 std::function<void()> cb) {
    addButton(group, [name]() -> const std::string& {
        static std::string n; n = name; return n;
    }, std::move(cb));
}

void VisualCodeEditor::addButton(const std::string& group,
                                 std::function<const std::string&()> name,
                                 std::function<void()> cb) {
    menuButtons_[group].emplace_back(std::move(name), std::move(cb));
}

void VisualCodeEditor::initSideBar() {
    addButton("Types", "Int",    [this]{ createValueNode(Value{0}); });
    addButton("Types", "Float",  [this]{ createValueNode(Value{0.f}); });
    addButton("Types", "String", [this]{ createValueNode(Value{std::string("")}); });
    addButton("Types", "Bool",   [this]{ createValueNode(Value{false}); });
    addButton("Types", "List",   [this]{ createValueNode(Value{std::vector<std::shared_ptr<Value>>{}}); });
    addButton("Types", "Map",    [this]{ createValueNode(Value{std::unordered_map<int,std::shared_ptr<Value>>{}}); });

    addButton("Variables", "Variable", [this]{ createVariableNode({PinType::None}); });
    addButton("Set",       "Set value",[this]{ createEqualNode(); });

    addButton("Binary ops", "+", [this]{ createBinaryOperatorNode("+"); });
    addButton("Binary ops", "-", [this]{ createBinaryOperatorNode("-"); });
    addButton("Binary ops", "*", [this]{ createBinaryOperatorNode("*"); });
    addButton("Binary ops", "/", [this]{ createBinaryOperatorNode("/"); });

    addButton("Logic", "==", [this]{ createBinaryOperatorLogicNode("=="); });
    addButton("Logic", "!=", [this]{ createBinaryOperatorLogicNode("!="); });
    addButton("Logic", ">=", [this]{ createBinaryOperatorLogicNode(">="); });
    addButton("Logic", "<=", [this]{ createBinaryOperatorLogicNode("<="); });
    addButton("Logic", "<",  [this]{ createBinaryOperatorLogicNode("<"); });
    addButton("Logic", ">",  [this]{ createBinaryOperatorLogicNode(">"); });
    addButton("Logic", "||", [this]{ createBinaryOperatorLogicNode("||"); });
    addButton("Logic", "&&", [this]{ createBinaryOperatorLogicNode("&&"); });

    addButton("Functions", "Fun", [this]{ createFunctionNode(); });

    addButton("Flow", "If/Else",  [this]{ createIfElseNode(); });
    addButton("Flow", "For",      [this]{ createForNode(); });
    addButton("Flow", "While",    [this]{ createWhileNode(); });
    addButton("Flow", "Return",   [this]{ createReturnNode(); });
    addButton("Flow", "Continue", [this]{ createContinueNode(); });
    addButton("Flow", "Break",    [this]{ createBreakNode(); });

    addButton("Misc", "Import",  [this]{ createImportNode(); });
    addButton("Misc", "This",    [this]{ createThisNode(); });
    addButton("Misc", "Comment", [this]{ createCommentNode(); });

    addButton("Classes", "New Class", [this]{ createClassNode(); });
}

// -----------------------------------------------------------------
// Main draw function
// -----------------------------------------------------------------

TextEditor g_codeEditor;

void VisualCodeEditor::draw() {
    session_.setAsCurrent();
    auto& io = ImGui::GetIO();

    static bool init = false;
    if (!init) {
        init = true;
        HeaderBackground = LoadTexture("BlueprintBackground.png");
        createStartNode();
        g_codeEditor.SetText("// Generated code will appear here");
    }

    // ---- Generated code text panel ----
    ImGui::Begin("Generated Script");
    if (ImGui::Button("Generate")) generateCode();
    ImGui::SameLine();
    if (ImGui::Button("Run"))      runScript();
    ImGui::SameLine();
    if (ImGui::Button("Save"))     saveScene("graph.bp.json");
    ImGui::SameLine();
    if (ImGui::Button("Load"))     loadScene("graph.bp.json");
    ImGui::Separator();
    g_codeEditor.Render("ScriptEditor");
    ImGui::End();

    // ---- Side panel (palette) ----
    ImGui::Begin("Blueprint Nodes");
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Separator();
    for (auto& [group, btns] : menuButtons_) {
        if (ImGui::CollapsingHeader(group.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& btn : btns) {
                const auto& name = btn.first();
                if (ImGui::Button(name.empty() ? "##_btn_" : name.c_str()))
                    btn.second();
            }
        }
    }

    // Dynamic: function calls, method calls, class instances
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Function Calls", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (auto& [id, fn] : ctx_.funNodes) {
            std::weak_ptr<FunNode> weak = fn;
            addButton("Function Calls",
                [fn]()->const std::string& { static std::string s; s=fn->funName; return s; },
                [this, weak]{ if (auto n = weak.lock()) createFunCall(n); });
        }
    }
    if (ImGui::CollapsingHeader("Class Methods", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (auto& [id, mn] : ctx_.methodNodes) {
            std::weak_ptr<MethodNode> weak = mn;
            addButton("Class Methods",
                [mn]()->const std::string& { static std::string s; s=mn->funName; return s; },
                [this, weak]{ if (auto n = weak.lock()) createMethodCall(n); });
        }
    }

    ImGui::End();

    // ---- Blueprint node editor ----
    ImGui::Begin("Blueprint Editor");
    ax::NodeEditor::Begin("BlueprintEditor", ImVec2(0,0));

    for (auto& [id, node] : ctx_.nodes)
        drawNode(*node);

    drawLinks();

    // Sync layout positions back to model after user interaction
    session_.syncLayout(ctx_);

    ax::NodeEditor::End();
    session_.clearCurrent();
    ImGui::End();
}

// -----------------------------------------------------------------
// drawNode
// -----------------------------------------------------------------

void VisualCodeEditor::drawNode(Node& node) {
    ax::NodeEditor::BeginNode(toEditorNodeId(node.id));
    ImGui::PushID(node.id);

    // Highlight active node during debugging
    if (node.id == highlightedNodeId_) {
        auto drawList = ax::NodeEditor::GetNodeBackgroundDrawList(toEditorNodeId(node.id));
        if (drawList) drawList->AddRectFilled({-4,-4}, {node.size.x+4,node.size.y+4},
                                              IM_COL32(255,200,0,80));
    }

    // Input pins (left)
    for (auto& pin : node.inputs) {
        ax::NodeEditor::BeginPin(toEditorPinId(pin->id), ax::NodeEditor::PinKind::Input);
        drawPinIcon(*pin, ctx_.isPinLinked(pin->id), 255);
        ImGui::SameLine();
        if (!pin->name.empty()) ImGui::TextUnformatted(pin->name.c_str());
        ax::NodeEditor::EndPin();
    }

    // Node body
    node.accept(drawer_);

    // Output pins (right)
    for (auto& pin : node.outputs) {
        ax::NodeEditor::BeginPin(toEditorPinId(pin->id), ax::NodeEditor::PinKind::Output);
        if (!pin->name.empty()) {
            ImGui::TextUnformatted(pin->name.c_str());
            ImGui::SameLine();
        }
        drawPinIcon(*pin, ctx_.isPinLinked(pin->id), 255);
        ax::NodeEditor::EndPin();
    }

    // Comment group overlay
    if (node.type == NodeType::Comment && ax::NodeEditor::BeginGroupHint(toEditorNodeId(node.id))) {
        ax::NodeEditor::EndGroupHint();
    }

    ImGui::PopID();
    ax::NodeEditor::EndNode();
}

// -----------------------------------------------------------------
// drawLinks + interaction
// -----------------------------------------------------------------

void VisualCodeEditor::drawLinks() {
    // Draw existing links
    for (auto& [id, link] : ctx_.links)
        ax::NodeEditor::Link(toEditorLinkId(link->id),
                             toEditorPinId(link->startPinID),
                             toEditorPinId(link->endPinID));

    // New link creation
    if (ax::NodeEditor::BeginCreate()) {
        ax::NodeEditor::PinId fromPinIdE, toPinIdE;
        if (ax::NodeEditor::QueryNewLink(&fromPinIdE, &toPinIdE) && fromPinIdE && toPinIdE) {
            if (ax::NodeEditor::AcceptNewItem()) {
                Id fromId = fromEditorPinId(fromPinIdE);
                Id toId   = fromEditorPinId(toPinIdE);
                auto fromPin = ctx_.pins.count(fromId) ? ctx_.pins[fromId] : nullptr;
                auto toPin   = ctx_.pins.count(toId)   ? ctx_.pins[toId]   : nullptr;
                if (!fromPin || !toPin) { ax::NodeEditor::EndCreate(); return; }

                // Enforce output→input direction
                if (fromPin->kind == toPin->kind) { ax::NodeEditor::EndCreate(); return; }
                if (fromPin->kind == PinKind::Input) { std::swap(fromPin, toPin); std::swap(fromId, toId); }

                if (fromPin->type.type == PinType::None)    { ax::NodeEditor::EndCreate(); return; }
                if (!checkInputTypeForOperator(toPin->node, fromPin->type)) { ax::NodeEditor::EndCreate(); return; }
                if (toPin->type.type == PinType::None) setInputPinsType(toPin->node, fromPin->type);
                if (!checkTypes(*fromPin, *toPin))          { ax::NodeEditor::EndCreate(); return; }

                auto link = ctx_.createLink(IdGenerator::GenId(), fromId, toId);
                ax::NodeEditor::Link(toEditorLinkId(link->id),
                                     toEditorPinId(link->startPinID),
                                     toEditorPinId(link->endPinID));
            }
        }
    }
    ax::NodeEditor::EndCreate();

    // Deletion
    if (ax::NodeEditor::BeginDelete()) {
        ax::NodeEditor::LinkId deletedLinkIdE;
        while (ax::NodeEditor::QueryDeletedLink(&deletedLinkIdE)) {
            if (ax::NodeEditor::AcceptDeletedItem()) {
                Id lid = fromEditorLinkId(deletedLinkIdE);
                for (auto it = ctx_.links.begin(); it != ctx_.links.end();) {
                    if (it->second->id == lid) {
                        updatePinsAfterDeleteLink(*it->second);
                        it = ctx_.links.erase(it); break;
                    } else ++it;
                }
            }
        }
        ax::NodeEditor::NodeId deletedNodeIdE;
        while (ax::NodeEditor::QueryDeletedNode(&deletedNodeIdE)) {
            if (ax::NodeEditor::AcceptDeletedItem()) {
                Id nid  = fromEditorNodeId(deletedNodeIdE);
                auto opt = ctx_.getNodeById(nid);
                if (opt) {
                    auto& nd = opt.value();
                    for (auto& p : nd->inputs)  ctx_.deleteAllLinksWithPinId(p->id);
                    for (auto& p : nd->outputs) ctx_.deleteAllLinksWithPinId(p->id);
                    for (auto& p : nd->inputs)  ctx_.pins.erase(p->id);
                    for (auto& p : nd->outputs) ctx_.pins.erase(p->id);
                    ctx_.deleteNode(nid);
                }
            }
        }
    }
    ax::NodeEditor::EndDelete();
}

// -----------------------------------------------------------------
// Code generation / execution
// -----------------------------------------------------------------

void VisualCodeEditor::generateCode() {
    if (!ctx_.startNode) { std::cout << "No StartNode!\n"; return; }
    try {
        GraphValidator validator(ctx_);
        auto errors = validator.validate();
        if (!errors.empty()) {
            for (auto& e : errors) std::cout << "Validation error: " << e.message << "\n";
            return;
        }
        auto result     = compileGraph(ctx_, "graph");
        lastScript_     = result.script;
        lastSourceMap_  = result.sourceMap;
        g_codeEditor.SetText(lastScript_);
        std::cout << "[Blueprint] Generated:\n" << lastScript_ << "\n";
    } catch (const std::exception& e) {
        std::cout << "[Blueprint] Generate error: " << e.what() << "\n";
    }
}

void VisualCodeEditor::runScript() {
    if (lastScript_.empty()) generateCode();
    if (lastScript_.empty()) return;
    try {
        IkigaiScript::IkigaiScriptInterpreter interp;
        interp.evaluate(lastScript_);
    } catch (const std::exception& e) {
        std::cout << "[Blueprint] Run error: " << e.what() << "\n";
    }
}

// -----------------------------------------------------------------
// Save / Load
// -----------------------------------------------------------------

void VisualCodeEditor::saveScene(std::string_view path) {
    session_.syncLayout(ctx_); // ensure model has latest positions
    GraphSaver saver(ctx_);
    saver.save(path);
}

void VisualCodeEditor::loadScene(std::string_view path) {
    int maxId = GraphLoader::load(ctx_, path);
    IdGenerator::SetStart(maxId + 1);
    session_.applyLayout(ctx_); // push stored positions into NodeEditor

    // Rebuild startNode reference
    for (auto& [id, node] : ctx_.nodes)
        if (node->type == NodeType::Start)
            ctx_.startNode = std::static_pointer_cast<StartNode>(node);
}

// -----------------------------------------------------------------
// Node factory helpers
// -----------------------------------------------------------------

void VisualCodeEditor::createValueNode(Value val) {
    NodeType t = NodeType::ValueInt;
    if (val.value) {
        std::visit(overloaded{
            [&](int)   { t = NodeType::ValueInt;    },
            [&](float) { t = NodeType::ValueFloat;  },
            [&](bool)  { t = NodeType::ValueBool;   },
            [&](std::string&) { t = NodeType::ValueString; },
            [&](std::vector<std::shared_ptr<Value>>&) { t = NodeType::ValueList; },
            [&](std::unordered_map<int,std::shared_ptr<Value>>&) { t = NodeType::ValueMap; },
            [&](auto&) {}
        }, val.value.value());
    }
    auto node = ctx_.createNode<ValueNode>(IdGenerator::GenId(), "Constant", t, val);
    ctx_.createPin(IdGenerator::GenId(), "Value", {PinType::Any}, node.get(), PinKind::Output);
}

void VisualCodeEditor::createVariableNode(PinTypeDescriptor type) {
    auto node = ctx_.createNode<VariableNode>(IdGenerator::GenId(), "Variable", "");
    ctx_.createPin(IdGenerator::GenId(), "",
                   {PinType::Variable, std::make_shared<PinTypeDescriptor>(type)},
                   node.get(), PinKind::Output);
}

void VisualCodeEditor::createStartNode() {
    auto node = ctx_.createNode<StartNode>(IdGenerator::GenId());
    ctx_.createPin(IdGenerator::GenId(), "", {PinType::Flow}, node.get(), PinKind::Output);
    ctx_.startNode = node;
}

void VisualCodeEditor::createImportNode() {
    auto node = ctx_.createNode<ImportNode>(IdGenerator::GenId(), "Import", "");
    ctx_.createPin(IdGenerator::GenId(), "", {PinType::Flow}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "", {PinType::Flow}, node.get(), PinKind::Output);
}

void VisualCodeEditor::createThisNode() {
    auto node = ctx_.createNode<ThisNode>(IdGenerator::GenId(), 0);
    ctx_.createPin(IdGenerator::GenId(), "Obj",    {PinType::Variable}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Member", {PinType::Any},      node.get(), PinKind::Output);
}

void VisualCodeEditor::createEqualNode() {
    auto node = ctx_.createNode<EqualNode>(IdGenerator::GenId());
    ctx_.createPin(IdGenerator::GenId(), "",    {PinType::Flow}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Var", {PinType::None}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Val", {PinType::None}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "",    {PinType::Flow}, node.get(), PinKind::Output);
}

void VisualCodeEditor::createBinaryOperatorNode(const std::string& name) {
    OperatorType op = OperatorType::Plus;
    if (name=="-") op=OperatorType::Minus;
    else if (name=="*") op=OperatorType::Multiply;
    else if (name=="/") op=OperatorType::Divided;
    auto node = ctx_.createNode<OperatorNode>(IdGenerator::GenId(), name, op);
    ctx_.createPin(IdGenerator::GenId(), "A",      {PinType::None}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "B",      {PinType::None}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Result", {PinType::None}, node.get(), PinKind::Output);
}

void VisualCodeEditor::createBinaryOperatorLogicNode(const std::string& name) {
    OperatorType op = OperatorType::Equality;
    if (name=="!=")  op=OperatorType::Inequality;
    else if (name==">=") op=OperatorType::EqualGreat;
    else if (name=="<=") op=OperatorType::EqualLess;
    else if (name=="<")  op=OperatorType::Less;
    else if (name==">")  op=OperatorType::Great;
    else if (name=="||") op=OperatorType::LogicalOr;
    else if (name=="&&") op=OperatorType::LogicalAnd;
    auto node = ctx_.createNode<OperatorNode>(IdGenerator::GenId(), name, op);
    ctx_.createPin(IdGenerator::GenId(), "A",      {PinType::None}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "B",      {PinType::None}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Result", {PinType::Bool}, node.get(), PinKind::Output);
}

void VisualCodeEditor::createFunctionNode() {
    auto node = ctx_.createNode<FunNode>(IdGenerator::GenId(), "Function");
    ctx_.createPin(IdGenerator::GenId(), "Body",   {PinType::Flow}, node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "Result", {PinType::None}, node.get(), PinKind::Output);
    ctx_.registerFunctionCall(node);

    std::weak_ptr<FunNode> weak = node;
    addButton("Function Calls",
        [node]()->const std::string& { static std::string s; s=node->funName; return s; },
        [this, weak]{ if (auto n = weak.lock()) createFunCall(n); });
}

void VisualCodeEditor::createFunCall(std::shared_ptr<FunNode> fnode) {
    auto node = ctx_.createNode<FunCall>(IdGenerator::GenId(), fnode->id);
    ctx_.createPin(IdGenerator::GenId(), "", {PinType::Flow}, node.get(), PinKind::Input);
    for (auto& [id, pair] : fnode->inputVars)
        ctx_.createPin(IdGenerator::GenId(), pair.first, {pair.second}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "",       {PinType::Flow}, node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "Result", fnode->outputType, node.get(), PinKind::Output);
}

void VisualCodeEditor::createClassNode() {
    auto node = ctx_.createNode<ClassNode>(IdGenerator::GenId(), "Class");
    ctx_.registerClassCall(node);

    std::weak_ptr<ClassNode> weak = node;
    addButton("Class Methods",
        [node]()->const std::string&{ static std::string s; s="Method:"+node->className; return s; },
        [this, weak]{ if (auto n = weak.lock()) createClassMethodNode(n); });
    addButton("Instances",
        [node]()->const std::string&{ static std::string s; s="New "+node->className; return s; },
        [this, weak]{ if (auto n = weak.lock()) createClassInstanceNode(n); });
}

void VisualCodeEditor::createClassMethodNode(std::shared_ptr<ClassNode> classNode) {
    auto node = ctx_.createNode<MethodNode>(IdGenerator::GenId(),
        "Method:" + classNode->className, classNode->id);
    ctx_.createPin(IdGenerator::GenId(), "Body", {PinType::Flow}, node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "this", {PinType::Object, classNode->className}, node.get(), PinKind::Output);
    ctx_.registerMethodCall(node);

    std::weak_ptr<MethodNode> weak = node;
    addButton("MethodCalls",
        [node, cn=classNode]()->const std::string& {
            static std::string s; s=cn->className+"."+node->funName; return s; },
        [this, weak]{ if (auto n = weak.lock()) createMethodCall(n); });
}

void VisualCodeEditor::createMethodCall(std::shared_ptr<MethodNode> fnode) {
    auto classOpt = ctx_.getNodeById(fnode->classNodeId);
    if (!classOpt) return;
    auto classNode = std::static_pointer_cast<ClassNode>(classOpt.value());
    auto node = ctx_.createNode<MethodCall>(IdGenerator::GenId(), fnode->id);
    ctx_.createPin(IdGenerator::GenId(), classNode->className,
                   {PinType::Object, classNode->className}, node.get(), PinKind::Input);
    for (auto& [id, pair] : fnode->inputVars)
        ctx_.createPin(IdGenerator::GenId(), pair.first, {pair.second}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Result", fnode->outputType, node.get(), PinKind::Output);
}

void VisualCodeEditor::createClassInstanceNode(std::shared_ptr<ClassNode> classNode) {
    auto node = ctx_.createNode<VariableNode>(IdGenerator::GenId(), "Variable", "");
    ctx_.createPin(IdGenerator::GenId(), "",
                   {PinType::Variable, std::make_shared<PinTypeDescriptor>(PinType::Object, classNode->className)},
                   node.get(), PinKind::Output);
}

void VisualCodeEditor::createIfElseNode() {
    auto node = ctx_.createNode<Node>(IdGenerator::GenId(), "IfElse", NodeType::If);
    ctx_.createPin(IdGenerator::GenId(), "",        {PinType::Flow}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Cond",    {PinType::Bool}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "True",    {PinType::Flow}, node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "False",   {PinType::Flow}, node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "Complete",{PinType::Flow}, node.get(), PinKind::Output);
}

void VisualCodeEditor::createForNode() {
    auto node = ctx_.createNode<Node>(IdGenerator::GenId(), "For", NodeType::For);
    ctx_.createPin(IdGenerator::GenId(), "",       {PinType::Flow}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Start",  {PinType::Int},  node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "End",    {PinType::Int},  node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Step",   {PinType::Int},  node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Body",   {PinType::Flow}, node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "Complete",{PinType::Flow},node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "Index",
                   {PinType::Variable, std::make_shared<PinTypeDescriptor>(PinType::Int)},
                   node.get(), PinKind::Output);
}

void VisualCodeEditor::createWhileNode() {
    auto node = ctx_.createNode<Node>(IdGenerator::GenId(), "While", NodeType::While);
    ctx_.createPin(IdGenerator::GenId(), "",        {PinType::Flow}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Cond",    {PinType::Bool}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Body",    {PinType::Flow}, node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "Complete",{PinType::Flow}, node.get(), PinKind::Output);
}

void VisualCodeEditor::createForEachNode() {
    auto node = ctx_.createNode<Node>(IdGenerator::GenId(), "ForEach", NodeType::ForEach);
    ctx_.createPin(IdGenerator::GenId(), "",        {PinType::Flow},      node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "List",    {PinType::Container}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Body",    {PinType::Flow},      node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "Complete",{PinType::Flow},      node.get(), PinKind::Output);
    ctx_.createPin(IdGenerator::GenId(), "Elem",    {PinType::Variable},  node.get(), PinKind::Output);
}

void VisualCodeEditor::createReturnNode() {
    auto node = ctx_.createNode<Node>(IdGenerator::GenId(), "Return", NodeType::Return);
    ctx_.createPin(IdGenerator::GenId(), "",     {PinType::Flow}, node.get(), PinKind::Input);
    ctx_.createPin(IdGenerator::GenId(), "Value",{PinType::None}, node.get(), PinKind::Input);
}

void VisualCodeEditor::createContinueNode() {
    auto node = ctx_.createNode<Node>(IdGenerator::GenId(), "Continue", NodeType::Continue);
    ctx_.createPin(IdGenerator::GenId(), "", {PinType::Flow}, node.get(), PinKind::Input);
}

void VisualCodeEditor::createBreakNode() {
    auto node = ctx_.createNode<Node>(IdGenerator::GenId(), "Break", NodeType::Break);
    ctx_.createPin(IdGenerator::GenId(), "", {PinType::Flow}, node.get(), PinKind::Input);
}

void VisualCodeEditor::createCommentNode() {
    auto node = ctx_.createNode<CommentNode>(IdGenerator::GenId(), "Comment...");
}
