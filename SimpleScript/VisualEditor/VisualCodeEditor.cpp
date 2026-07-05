#include "VisualCodeEditor.h"

#define NOMINMAX

#define IMGUI_DEFINE_MATH_OPERATORS
#include <fstream>
#include <iomanip>
#include <iostream>

#include "../imgui/imgui_internal.h"

#include "../imgui/misc/cpp/imgui_stdlib.h"

#include <variant>

#include "Context.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <set>

#include "../stb_image.h"

#include "VisualNodes.h"
#include "VisualVisitors.h"
#include "../imgui/imgui.h"

#include "../json.hpp"
#include "../imgui/imgui_impl_opengl3_loader.h"
#include "../imgui/TextEditor.h"

using namespace Visual;


NodeDrawer nodeDrawer;
CodeGenerator codeGenerator;
CodeExecutor codeExecutor;

const int m_PinIconSize = 24;
ImColor GetIconColor(PinType type) {
    switch (type) {
    default:
    case PinType::Flow:     return ImColor(255, 255, 255);
    case PinType::Bool:     return ImColor(220, 48, 48);
    case PinType::Int:      return ImColor(68, 201, 156);
    case PinType::Float:    return ImColor(147, 226, 74);
    case PinType::String:   return ImColor(124, 21, 153);
    case PinType::Object:   return ImColor(51, 150, 215);
    case PinType::Function: return ImColor(218, 0, 183);
    case PinType::None: return ImColor(128, 128, 128);
    }
};

enum class IconType : ImU32 { Flow, Circle, Square, Grid, RoundSquare, Diamond };
void DrawIcon(ImDrawList* drawList, const ImVec2& a, const ImVec2& b, IconType type, bool filled, ImU32 color, ImU32 innerColor)
{
    auto rect = ImRect(a, b);
    auto rect_x = rect.Min.x;
    auto rect_y = rect.Min.y;
    auto rect_w = rect.Max.x - rect.Min.x;
    auto rect_h = rect.Max.y - rect.Min.y;
    auto rect_center_x = (rect.Min.x + rect.Max.x) * 0.5f;
    auto rect_center_y = (rect.Min.y + rect.Max.y) * 0.5f;
    auto rect_center = ImVec2(rect_center_x, rect_center_y);
    const auto outline_scale = rect_w / 24.0f;
    const auto extra_segments = static_cast<int>(2 * outline_scale); // for full circle

    if (type == IconType::Flow)
    {
        const auto origin_scale = rect_w / 24.0f;

        const auto offset_x = 1.0f * origin_scale;
        const auto offset_y = 0.0f * origin_scale;
        const auto margin = (filled ? 2.0f : 2.0f) * origin_scale;
        const auto rounding = 0.1f * origin_scale;
        const auto tip_round = 0.7f; // percentage of triangle edge (for tip)
        //const auto edge_round = 0.7f; // percentage of triangle edge (for corner)
        const auto canvas = ImRect(
            rect.Min.x + margin + offset_x,
            rect.Min.y + margin + offset_y,
            rect.Max.x - margin + offset_x,
            rect.Max.y - margin + offset_y);
        const auto canvas_x = canvas.Min.x;
        const auto canvas_y = canvas.Min.y;
        const auto canvas_w = canvas.Max.x - canvas.Min.x;
        const auto canvas_h = canvas.Max.y - canvas.Min.y;

        const auto left = canvas_x + canvas_w * 0.5f * 0.3f;
        const auto right = canvas_x + canvas_w - canvas_w * 0.5f * 0.3f;
        const auto top = canvas_y + canvas_h * 0.5f * 0.2f;
        const auto bottom = canvas_y + canvas_h - canvas_h * 0.5f * 0.2f;
        const auto center_y = (top + bottom) * 0.5f;
        //const auto angle = AX_PI * 0.5f * 0.5f * 0.5f;

        const auto tip_top = ImVec2(canvas_x + canvas_w * 0.5f, top);
        const auto tip_right = ImVec2(right, center_y);
        const auto tip_bottom = ImVec2(canvas_x + canvas_w * 0.5f, bottom);

        drawList->PathLineTo(ImVec2(left, top) + ImVec2(0, rounding));
        drawList->PathBezierCubicCurveTo(
            ImVec2(left, top),
            ImVec2(left, top),
            ImVec2(left, top) + ImVec2(rounding, 0));
        drawList->PathLineTo(tip_top);
        drawList->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
        drawList->PathBezierCubicCurveTo(
            tip_right,
            tip_right,
            tip_bottom + (tip_right - tip_bottom) * tip_round);
        drawList->PathLineTo(tip_bottom);
        drawList->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
        drawList->PathBezierCubicCurveTo(
            ImVec2(left, bottom),
            ImVec2(left, bottom),
            ImVec2(left, bottom) - ImVec2(0, rounding));

        if (!filled)
        {
            if (innerColor & 0xFF000000)
                drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

            drawList->PathStroke(color, true, 2.0f * outline_scale);
        }
        else
            drawList->PathFillConvex(color);
    }
    else
    {
        auto triangleStart = rect_center_x + 0.32f * rect_w;

        auto rect_offset = -static_cast<int>(rect_w * 0.25f * 0.25f);

        rect.Min.x += rect_offset;
        rect.Max.x += rect_offset;
        rect_x += rect_offset;
        rect_center_x += rect_offset * 0.5f;
        rect_center.x += rect_offset * 0.5f;

        if (type == IconType::Circle)
        {
            const auto c = rect_center;

            if (!filled)
            {
                const auto r = 0.5f * rect_w / 2.0f - 0.5f;

                if (innerColor & 0xFF000000)
                    drawList->AddCircleFilled(c, r, innerColor, 12 + extra_segments);
                drawList->AddCircle(c, r, color, 12 + extra_segments, 2.0f * outline_scale);
            }
            else
            {
                drawList->AddCircleFilled(c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments);
            }
        }

        if (type == IconType::Square)
        {
            if (filled)
            {
                const auto r = 0.5f * rect_w / 2.0f;
                const auto p0 = rect_center - ImVec2(r, r);
                const auto p1 = rect_center + ImVec2(r, r);

#if IMGUI_VERSION_NUM > 18101
                drawList->AddRectFilled(p0, p1, color, 0, ImDrawFlags_RoundCornersAll);
#else
                drawList->AddRectFilled(p0, p1, color, 0, 15);
#endif
            }
            else
            {
                const auto r = 0.5f * rect_w / 2.0f - 0.5f;
                const auto p0 = rect_center - ImVec2(r, r);
                const auto p1 = rect_center + ImVec2(r, r);

                if (innerColor & 0xFF000000)
                {
#if IMGUI_VERSION_NUM > 18101
                    drawList->AddRectFilled(p0, p1, innerColor, 0, ImDrawFlags_RoundCornersAll);
#else
                    drawList->AddRectFilled(p0, p1, innerColor, 0, 15);
#endif
                }

#if IMGUI_VERSION_NUM > 18101
                drawList->AddRect(p0, p1, color, 0, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale);
#else
                drawList->AddRect(p0, p1, color, 0, 15, 2.0f * outline_scale);
#endif
            }
        }

        if (type == IconType::Grid)
        {
            const auto r = 0.5f * rect_w / 2.0f;
            const auto w = ceilf(r / 3.0f);

            const auto baseTl = ImVec2(floorf(rect_center_x - w * 2.5f), floorf(rect_center_y - w * 2.5f));
            const auto baseBr = ImVec2(floorf(baseTl.x + w), floorf(baseTl.y + w));

            auto tl = baseTl;
            auto br = baseBr;
            for (int i = 0; i < 3; ++i)
            {
                tl.x = baseTl.x;
                br.x = baseBr.x;
                drawList->AddRectFilled(tl, br, color);
                tl.x += w * 2;
                br.x += w * 2;
                if (i != 1 || filled)
                    drawList->AddRectFilled(tl, br, color);
                tl.x += w * 2;
                br.x += w * 2;
                drawList->AddRectFilled(tl, br, color);

                tl.y += w * 2;
                br.y += w * 2;
            }

            triangleStart = br.x + w + 1.0f / 24.0f * rect_w;
        }

        if (type == IconType::RoundSquare)
        {
            if (filled)
            {
                const auto r = 0.5f * rect_w / 2.0f;
                const auto cr = r * 0.5f;
                const auto p0 = rect_center - ImVec2(r, r);
                const auto p1 = rect_center + ImVec2(r, r);

#if IMGUI_VERSION_NUM > 18101
                drawList->AddRectFilled(p0, p1, color, cr, ImDrawFlags_RoundCornersAll);
#else
                drawList->AddRectFilled(p0, p1, color, cr, 15);
#endif
            }
            else
            {
                const auto r = 0.5f * rect_w / 2.0f - 0.5f;
                const auto cr = r * 0.5f;
                const auto p0 = rect_center - ImVec2(r, r);
                const auto p1 = rect_center + ImVec2(r, r);

                if (innerColor & 0xFF000000)
                {
#if IMGUI_VERSION_NUM > 18101
                    drawList->AddRectFilled(p0, p1, innerColor, cr, ImDrawFlags_RoundCornersAll);
#else
                    drawList->AddRectFilled(p0, p1, innerColor, cr, 15);
#endif
                }

#if IMGUI_VERSION_NUM > 18101
                drawList->AddRect(p0, p1, color, cr, ImDrawFlags_RoundCornersAll, 2.0f * outline_scale);
#else
                drawList->AddRect(p0, p1, color, cr, 15, 2.0f * outline_scale);
#endif
            }
        }
        else if (type == IconType::Diamond)
        {
            if (filled)
            {
                const auto r = 0.607f * rect_w / 2.0f;
                const auto c = rect_center;

                drawList->PathLineTo(c + ImVec2(0, -r));
                drawList->PathLineTo(c + ImVec2(r, 0));
                drawList->PathLineTo(c + ImVec2(0, r));
                drawList->PathLineTo(c + ImVec2(-r, 0));
                drawList->PathFillConvex(color);
            }
            else
            {
                const auto r = 0.607f * rect_w / 2.0f - 0.5f;
                const auto c = rect_center;

                drawList->PathLineTo(c + ImVec2(0, -r));
                drawList->PathLineTo(c + ImVec2(r, 0));
                drawList->PathLineTo(c + ImVec2(0, r));
                drawList->PathLineTo(c + ImVec2(-r, 0));

                if (innerColor & 0xFF000000)
                    drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

                drawList->PathStroke(color, true, 2.0f * outline_scale);
            }
        }
        else
        {
            const auto triangleTip = triangleStart + rect_w * (0.45f - 0.32f);

            drawList->AddTriangleFilled(
                ImVec2(ceilf(triangleTip), rect_y + rect_h * 0.5f),
                ImVec2(triangleStart, rect_center_y + 0.15f * rect_h),
                ImVec2(triangleStart, rect_center_y - 0.15f * rect_h),
                color);
        }
    }
}

void DrawIcon(const ImVec2& size, IconType type, bool filled, const ImVec4& color/* = ImVec4(1, 1, 1, 1)*/, const ImVec4& innerColor/* = ImVec4(0, 0, 0, 0)*/) {
    if (ImGui::IsRectVisible(size)) {
        auto cursorPos = ImGui::GetCursorScreenPos();
        auto drawList = ImGui::GetWindowDrawList();
        DrawIcon(drawList, cursorPos, cursorPos + size, type, filled, ImColor(color), ImColor(innerColor));
    }

    ImGui::Dummy(size);
}

inline ImRect ImGui_GetItemRect()
{
    return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}
inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

void LoadScene(std::string_view scenePath) {
    auto& ce = CodeEditorContext::Instance();
    ce.clear();
    std::ifstream f(std::string(scenePath.begin(), scenePath.end()));
    nlohmann::json data = nlohmann::json::parse(f);

    int maxId = 1;

    auto readSubtype = [](nlohmann::json& js) -> PinSubtype {
        if (!js.contains("subtype")) {
            return std::nullopt;
        }
        auto type = js["subtype"]["type"]["type"].get<std::string>();
        if (type == "string") {
	        return js["subtype"]["type"]["val"].get<std::string>();
        }
        else {
            return std::make_shared<PinTypeDescriptor>((PinType)js["subtype"]["type"]["val"].get<int>());
        }
    };

    auto addPins = [&ce, &maxId, &readSubtype](NodePtr node, nlohmann::json& e) {
        for (auto pin : e["outputs"]) {
            auto id = pin["id"].get<int>();
            maxId = std::max(maxId, id);

            auto _pin = ce.createPin(pin["id"].get<int>(), 
                pin["name"].get<std::string>().c_str(), 
                PinTypeDescriptor{ (PinType)pin["type"]["type"].get<int>(), readSubtype(pin["type"])},
                node.get(), PinKind::Output);
        }
        for (auto pin : e["inputs"]) {
            auto id = pin["id"].get<int>();
            maxId = std::max(maxId, id);

            auto _pin = ce.createPin(pin["id"].get<int>(),
                pin["name"].get<std::string>().c_str(), 
                PinTypeDescriptor{ (PinType)pin["type"]["type"].get<int>(), readSubtype(pin["type"])},
                node.get(), PinKind::Input);
        }
    };

    for (auto e : data["nodes"]) {
        auto id = e["id"].get<int>();
        maxId = std::max(maxId, id);
        if (e["type"].get<int>() == (int)NodeType::Variable) {
            auto node = ce.createNode<VariableNode>(e["id"].get<int>(), std::string("Variable"), e["value"].get<std::string>());
            addPins(node, e);
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::ValueBool || e["type"].get<int>() == (int)NodeType::ValueInt ||
            e["type"].get<int>() == (int)NodeType::ValueFloat || e["type"].get<int>() == (int)NodeType::ValueString
            
            
            || e["type"].get<int>() == (int)NodeType::ValueArrayBool || e["type"].get<int>() == (int)NodeType::ValueArrayInt
            || e["type"].get<int>() == (int)NodeType::ValueArrayFloat || e["type"].get<int>() == (int)NodeType::ValueList
            || e["type"].get<int>() == (int)NodeType::ValueMap) {

            auto node = ce.createNode<ValueNode>(
                e["id"].get<int>(),
                e["name"].get<std::string>().c_str(),
                (NodeType)e["type"].get<int>(),
                [&e]()->Value {
                    if (e["value"].is_boolean())
                    {
                        return Value{ e["value"].get<bool>() };
                    }
                    if (e["value"].is_number_integer())
                    {
                        return Value{ e["value"].get<int>() };
                    }
                    if (e["value"].is_number_float())
                    {
                        return Value{ e["value"].get<float>() };
                    }
                    if (e["value"].is_string())
                    {
                        return Value{ e["value"].get<std::string>() };
                    }
                    if (e["type"].get<int>() == (int)NodeType::ValueArrayBool) {
                        return Value{ std::vector<bool>{} };
                    }
                    if (e["type"].get<int>() == (int)NodeType::ValueArrayInt) {
                        return Value{ std::vector<int>{} };
                    }
                    if (e["type"].get<int>() == (int)NodeType::ValueArrayFloat) {
                        return Value{ std::vector<float>{} };
                    }
                    if (e["type"].get<int>() == (int)NodeType::ValueList) {
                        return Value{ std::vector<std::shared_ptr<Value>>{} };
                    }
                    if (e["type"].get<int>() == (int)NodeType::ValueMap) {
                        return Value{ std::unordered_map<int, std::shared_ptr<Value>>{} };
                    }
                    return Value(0);
                }()
             );
            addPins(node, e);
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        //TODO:
        //else if (containers)
        //{
	    //    
        //}
        else if (e["type"].get<int>() == (int)NodeType::FunctionDeclaration) {
            auto node = ce.createNode<FunNode>(
                e["id"].get<int>(),
                e["name"].get<std::string>());
            addPins(node, e);

            for (auto& pin : e["inputVars"]) {
                node->inputVars[pin["id"].get<int>()] = std::make_pair(pin["name"].get<std::string>(), 
                    PinTypeDescriptor{ (PinType)pin["type"]["type"].get<int>(), readSubtype(pin["type"])});
            }
            node->outputType = PinTypeDescriptor{ (PinType)e["outputType"]["type"]["type"].get<int>(), readSubtype(e["outputType"]["type"])};

            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::FunctionCall) {
            auto node = ce.createNode<FunCall>(
                e["id"].get<int>(),
                e["funDeclarationId"].get<int>());
            addPins(node, e);
            
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::Operator) {
            auto node = ce.createNode<OperatorNode>(
                e["id"].get<int>(),
                e["name"].get<std::string>().c_str(),
                (OperatorType)e["opType"].get<int>());
            addPins(node, e);
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }

        else if (e["type"].get<int>() == (int)NodeType::Import) {
            auto node = ce.createNode<ImportNode>(
                e["id"].get<int>(),
                e["name"].get<std::string>(),
                e["value"].get<std::string>());
            addPins(node, e);
            
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::Comment) {
            auto node = ce.createNode<CommentNode>(
                e["id"].get<int>(),
                e["value"].get<std::string>());
            addPins(node, e);

            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
        }
        else if (e["type"].get<int>() == (int)NodeType::ClassMethodDeclaration) {
            auto node = ce.createNode<MethodNode>(
                e["id"].get<int>(),
                e["name"].get<std::string>(),
                e["classNodeId"].get<int>());
            addPins(node, e);

            for (auto& pin : e["inputVars"]) {
                node->inputVars[pin["id"].get<int>()] = std::make_pair(pin["name"].get<std::string>(),
                    PinTypeDescriptor{ (PinType)pin["type"].get<int>(), readSubtype(pin["type"]) });
            }
            node->outputType = PinTypeDescriptor{ (PinType)e["outputType"].get<int>(), readSubtype(e["outputType"]) };

            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::ClassMethodCall) {
            auto node = ce.createNode<MethodCall>(
                e["id"].get<int>(),
                e["methodeDeclarationId"].get<int>());
            addPins(node, e);

            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::ClassDeclaration) {
            auto node = ce.createNode<ClassNode>(
                e["id"].get<int>(),
                e["className"].get<std::string>());
            addPins(node, e);
            for (auto& pin : e["memberVars"]) {
                node->memberVars[pin["id"].get<int>()] = ClassNode::Member{ pin["name"].get<std::string>(),
                    PinTypeDescriptor{ (PinType)pin["type"].get<int>(), readSubtype(pin["type"]) } };
                node->membersToPin[pin["name"].get<std::string>()] = pin["id"].get<int>();
            }


            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::This) {
            auto node = ce.createNode<ThisNode>(
                e["id"].get<int>(), e["classId"].get<int>());
            addPins(node, e);
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::Equal) {
            auto node = ce.createNode<EqualNode>(
                e["id"].get<int>());
            addPins(node, e);
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else if (e["type"].get<int>() == (int)NodeType::Start) {
            auto node = ce.createNode<StartNode>(
                e["id"].get<int>());
            addPins(node, e);
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
        else {
            auto node = ce.createNode<Node>(
                e["id"].get<int>(),
                e["name"].get<std::string>().c_str(),
                (NodeType)e["type"].get<int>());
            addPins(node, e);
            if (e.contains("pos")) {
                ax::NodeEditor::SetNodePosition(node->id, ImVec2(e["pos"]["x"].get<float>(), e["pos"]["y"].get<float>()));
            }
            //if (e.contains("computedType")) {
            //    node->computedType = PinTypeDescriptor{ (PinType)e["computedType"]["type"].get<int>(), readSubtype(e["computedType"]) };
            //}
        }
    }

    for (auto e : data["links"]) {
        auto id = e["id"].get<int>();
        maxId = std::max(maxId, id);

        ce.createLink(e["id"].get<int>(), 
            e["start"].get<int>(), e["end"].get<int>());
    }

    IdGenerator::SetStart(maxId + 1);
}

void SaveScene(std::string_view scenePath) {
    SaveNode visitor(scenePath);
    auto& context = CodeEditorContext::Instance();
    for (auto& node : context.nodes) {
        node.second->run(visitor);
    }
    visitor.saveLinks();

    std::ofstream o(std::string(scenePath.begin(), scenePath.end()));
    o << std::setw(4) << visitor.data << std::endl;
}

ImTextureID HeaderBackground = nullptr;

struct ImTexture
{
    int Width = 0;
    int Height = 0;
    unsigned TextureID = 0;
};

ImTexture headerTexture;

ImTextureID CreateTexture(const void* data, int width, int height)
{
    GLint last_texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures(1, &headerTexture.TextureID);
    glBindTexture(GL_TEXTURE_2D, headerTexture.TextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, last_texture);

    headerTexture.Width = width;
    headerTexture.Height = height;

    return reinterpret_cast<ImTextureID>(static_cast<std::intptr_t>(headerTexture.TextureID));
}

ImTextureID LoadTexture(const char* path) {
    int width = 0, height = 0, component = 0;
    if (auto data = stbi_load(path, &width, &height, &component, 4))
    {
        auto texture = CreateTexture(data, width, height);
        stbi_image_free(data);
        return texture;
    }
    else
        return nullptr;
}

VisualCodeEditor::VisualCodeEditor() {
    CodeEditorContext::Instance();
    initSideBar();

    createStartNode();

    HeaderBackground = LoadTexture("BlueprintBackground.png");
};


void VisualCodeEditor::addButton(const std::string& group, const std::string& name, std::function<void()> cb) {
    if (!menuButtons.contains(group)) {
        menuButtons[group] = {};
    }
    menuButtons[group].push_back(std::make_pair([_name = name]()->const std::string& { return _name; }, cb));
}

void VisualCodeEditor::addButton(const std::string& group, std::function<const std::string&()> name, std::function<void()> cb) {
    if (!menuButtons.contains(group)) {
        menuButtons[group] = {};
    }
    menuButtons[group].push_back(std::make_pair(name, cb));
}

std::string ToString(PinType type)
{
	switch (type)
	{
	case PinType::None: return"None"; break;
	case PinType::Flow: return"Flow"; break;
	case PinType::Bool: return"Bool"; break;
	case PinType::Int: return"Int"; break;
	case PinType::Float: return"Float"; break;
	case PinType::String: return"String"; break;
	case PinType::Object: return"Object"; break;
	case PinType::Function: return"Function"; break;
	case PinType::List: return"List"; break;
	case PinType::ArrayBool: return"ArrayBool"; break;
	case PinType::ArrayInt: return"ArrayInt"; break;
	case PinType::ArrayFloat: return"ArrayFloat"; break;
	case PinType::Map: return"Map"; break;
	case PinType::Any: return"Any"; break;
	case PinType::Container: return"Container"; break;
	case PinType::Variable: return"Variable"; break;
    //case PinType::Expression: return"Expression"; break;
	default: ;
	}

    return "";
}

OperatorType StringToEnum(const std::string& str) {
	if (str == "+") return OperatorType::Plus;
    if (str == "-") return OperatorType::Minus;
    if (str == "*") return OperatorType::Multiply;
    if (str == "/") return OperatorType::Divided;
    if (str == "<") return OperatorType::Less;
    if (str == "<=") return OperatorType::EqualLess;
    if (str == ">") return  OperatorType::Great;
    if (str == ">=") return OperatorType::EqualGreat;
    if (str == "!=") return OperatorType::Inequality;
    if (str == "==") return OperatorType::Equality;
    if (str == "&&") return OperatorType::LogicalAnd;
    if (str == "||") return OperatorType::LogicalOr;

    throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));

    return OperatorType::Minus;
};


void VisualCodeEditor::initSideBar() {
    addButton("Types constant", "Int", [this]() { createValueNode(Value{ 0 }); });
    addButton("Types constant", "Float", [this]() { createValueNode(Value{ 0.0f }); });
    addButton("Types constant", "String", [this]() { createValueNode(Value{ "" }); });
    addButton("Types constant", "Bool", [this]() { createValueNode(Value{ false }); });
    //addButton("Types constant", "Array Bool", [this]() { createValueNode(Value{ std::vector<bool>() }); });
    //addButton("Types constant", "Array Int", [this]() { createValueNode(Value{ std::vector<int>() }); });
    //addButton("Types constant", "Array Float", [this]() { createValueNode(Value{ std::vector<float>() }); });
    addButton("Types constant", "List", [this]() { createValueNode(Value{ std::vector<std::shared_ptr<Value>>() }); });
    addButton("Types constant", "Map", [this]() { createValueNode(Value{ std::unordered_map<int, std::shared_ptr<Value>>()}); });

    //for (auto& type : { PinType::Bool, PinType::Int, PinType::Float, PinType::String, PinType::List, PinType::Map }) {
        addButton("Variables", "Variable", [this]() { createVariableNode(PinType::None); });
    //}
    //TODO: custom types

    //for (auto& type : { PinType::Bool, PinType::Int, PinType::Float,
    //    PinType::String, PinType::ArrayBool, PinType::ArrayInt, PinType::ArrayFloat, PinType::List, PinType::Map }) {
        addButton("Set", "Set value", [this]() { createEqualNode(PinType::None); });
    //}
    //TODO: custom types

    //for (auto& type : { PinType::Bool, PinType::Int, PinType::Float }) {
        addButton("Binary operations", "+ ", [this]() { createBinaryOperatorNode("+", PinType::None); });
        addButton("Binary operations", "- ", [this]() { createBinaryOperatorNode("-", PinType::None); });
        addButton("Binary operations", "* ", [this]() { createBinaryOperatorNode("*", PinType::None); });
        addButton("Binary operations", "/ ", [this]() { createBinaryOperatorNode("/", PinType::None); });
    //}

    //for (auto& type : { PinType::Bool, PinType::Int, PinType::Float,
    //   PinType::String, PinType::ArrayBool, PinType::ArrayInt, PinType::ArrayFloat, PinType::List, PinType::Map }) {
        addButton("Logic operations", "==", [this]() { createBinaryOperatorLogicNode("==", PinType::None); });
        addButton("Logic operations", "!=", [this]() { createBinaryOperatorLogicNode("!=", PinType::None); });
   // }
    //TODO: custom types
    //for (auto& type : { PinType::Bool, PinType::Int, PinType::Float, PinType::String }) {
        addButton("Logic operations", ">=", [this]() { createBinaryOperatorLogicNode(">=", PinType::None); });
        addButton("Logic operations", "<=", [this]() { createBinaryOperatorLogicNode("<=", PinType::None); });
        addButton("Logic operations", "<",  [this]() { createBinaryOperatorLogicNode("<", PinType::None); });
        addButton("Logic operations", ">",  [this]() { createBinaryOperatorLogicNode(">", PinType::None); });
    //}
    //for (auto& type : { PinType::Bool }) {
        addButton("Logic operations", "||", [this]() { createBinaryOperatorLogicNode("||", PinType::Bool); });
        addButton("Logic operations", "&&", [this]() { createBinaryOperatorLogicNode("&&", PinType::Bool); });
    //}
    addButton("Func", "Fun", [this]() { createFunctionNode(); });

	addButton("Operators", "InElse", [this]() { createIfElseNode(); });
    addButton("Operators", "For", [this]() { createForNode(); });

    //for (auto& type : { PinType::Bool, PinType::Int, PinType::Float,
    //    PinType::String, PinType::ArrayBool, PinType::ArrayInt, PinType::ArrayFloat, PinType::List, PinType::Map }) {
        addButton("Operators", "Return", [this]() { createReturn(PinType::None); });
    //}
    //TODO: custom types

	addButton("Operators", "Continue", [this]() { createContinue(); });
    addButton("Operators", "Break", [this]() { createBreak(); });
    addButton("Operators", "Import", [this]() { createImportNode(); });
    addButton("Operators", "This", [this]() { createThisNode(); });
    addButton("Comments", "Comment", [this]() { createComment(); });
    
    addButton("Class", "Create new Class", [this]() { createClassNode(); });
}
TextEditor editor;

bool isDebugText = false;
bool isDebugBp = false;

void VisualCodeEditor::draw() {
    ax::NodeEditor::SetCurrentEditor(CodeEditorContext::Instance().context.get());
	auto& io = ImGui::GetIO();

    static bool isEditorInit = false;
    if (!isEditorInit) {
        isEditorInit = true;
        editor.SetText(R"(
func fizzbuzz(n) {
    for (i = 1; i <= n; i++) {
        if (i % 15 == 0) {
            print(1);
        }
        else if (i % 3 == 0) {
            print(2);
        }
        else if (i % 5 == 0) {
            print(3);
        }
        else {
            print(i);
        }
    }
}
)");

        editor.SetBreakpoints({ 13 });
    }
    ImGui::Begin("TextEditor");

    //Control buttons
    if (ImGui::Button("Run"))
    {
	    
    }
    ImGui::SameLine();
    if (ImGui::Button("Debug"))
    {
        isDebugText = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
    {
        isDebugText = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Next"))
    {
        
    }
    ImGui::SameLine();
    if (ImGui::Button("Play"))
    {
        
    }

    if (isDebugText)
    {
	    
    }

    editor.Render("TextEditor");
    if (ImGui::IsWindowFocused()) {
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_B)) {
            editor.AddBreakpoint(editor.GetCursorPosition().mLine-1);
        }
    }
	ImGui::End();

	ImGui::Begin("BluePrints");
    for (auto& [group, btns] : menuButtons) {
        if (ImGui::CollapsingHeader(group.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
	        for (auto& btn : btns) {
                auto& nameBtn = btn.first();
	        	if (ImGui::Button(nameBtn.empty() ? "##_btn_name_" : nameBtn.c_str())) {
                    btn.second();
                }
            }

        }
    }

    ImGui::Separator();
    if (ImGui::Button("Save")) {
        SaveScene("test.json");
    }
    if (ImGui::Button("Load")) {
        LoadScene("test.json");
    }


    ImGui::Separator();
    if (ImGui::Button("Generate")) {
        try {
            generateCode();
            std::cout << codeGenerator.code << std::endl;
        }
        catch (...)
        {
            std::cout << "Problem\n";
        }
    }
	ImGui::End();

	{
		ImGui::Begin("BluePrints1");
		ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
        ImGui::SameLine();
        if (ImGui::Button("Run1"))
        {
            CodeEditorContext::Instance().resetScope();

            CodeEditorContext::Instance().startNode->run(codeExecutor);
        }
		ImGui::Separator();
        

        ax::NodeEditor::Begin("BluePrint", ImVec2(0.0, 0.0f));
		for (const auto& [id, node] : CodeEditorContext::Instance().nodes) {
			drawNode(*node);
		}


        drawLinks();

        ax::NodeEditor::End();
        ax::NodeEditor::SetCurrentEditor(nullptr);
		ImGui::End();
	}
}

void VisualCodeEditor::generateCode() {
    CodeEditorContext::Instance().resetScope();
    codeGenerator.code = "\n\n";
    for (const auto& [id, node] : CodeEditorContext::Instance().nodes) {
        if (node->type == NodeType::ClassDeclaration) {
            node->run(codeGenerator);
        }
    }

    for (const auto& [id, node] : CodeEditorContext::Instance().nodes) {
        if (node->type == NodeType::FunctionDeclaration) {
            node->run(codeGenerator);
        }
    }

    CodeEditorContext::Instance().startNode->run(codeGenerator);
}

bool tryConvert(PinTypeDescriptor from, PinTypeDescriptor to) {
	if (from.type == to.type) {
		if (from.type == PinType::None) {
            std::cout << "Error, type is PinType::None\n";
            return false;
		}
        if (from.type == PinType::Object) {
            if (from.subType && !to.subType) {
                to.subType = std::get<std::string>(from.subType.value());
            }
            return std::get<std::string>(from.subType.value()) == std::get<std::string>(to.subType.value());
        }
        //if (from == PinType::Function) {
        //    //TODO: need check that it is same funcs
        //}
        return true;
	}
    if (from.type == PinType::Bool) {
        if (to.type == PinType::Int || to.type == PinType::Float) {
            return true;
        }
    }
    if (from.type == PinType::Int) {
        if (to.type == PinType::Float) {
            return true;
        }
    }
    return false;
}


bool checkTypes(Pin& from, Pin& to) {
    auto res =  tryConvert(from.type, to.type);
    // Node type is var and it contains some type so need double check
    if (from.type.type == PinType::Variable && to.type.type == PinType::Variable) {
        return tryConvert(*from.type.getPinDescr(), *to.type.getPinDescr());
    }
    if (from.type.type == PinType::Variable) {
        return tryConvert(*from.type.getPinDescr(), to.type);
    }
    if (to.type.type == PinType::Any) {
        return true;
    }
    return res;
}

void clearPinsForThisNode(ThisNode& thisNode) {
    auto& context = CodeEditorContext::Instance();

    thisNode.classId = 0;

    //delete prev pins
    for (auto e : thisNode.outputs) {
        context.deletePin(e->id.Get());
    }
    thisNode.outputs.clear();
}

void updatePinsForThisNode(ThisNode& thisNode, VariableNode& varNode) {
    auto& context = CodeEditorContext::Instance();

    clearPinsForThisNode(thisNode);

    // find input class node
    std::shared_ptr<ClassNode> classNode;
    for (auto e : context.classNodes) {
	    if (e.second->className == varNode.outputs[0]->type.getPinDescr()->getStr()) {
            classNode = e.second;
            break;
	    }
    }

    thisNode.classId = classNode->id.Get();
    // add new pins
    for (auto e : classNode->inputs) {
        CodeEditorContext::Instance().createPin(IdGenerator::GenId(), e->name, e->type, &thisNode, PinKind::Output);
    }

}

void setInputPinsType(Node* node, PinTypeDescriptor type) {
    auto _t = type;
    if (type.type == PinType::Variable) {
        _t = *type.getPinDescr();
    }

	if (node->type == NodeType::Operator) {
        auto op = static_cast<OperatorNode*>(node);
        for (auto& e : node->inputs) {
        	e->type = _t;
        }

        static std::set types = { OperatorType::Plus, OperatorType::Minus, OperatorType::Multiply, OperatorType::Divided };
        if (types.contains(op->opType)) {
            op->outputs[0]->type = _t;
        }
	}
    else if (node->type == NodeType::Equal) {
        //node->computedType = type;
    	node->inputs[1]->type.subType = std::make_shared<PinTypeDescriptor>(_t); //value pin
    	node->inputs[2]->type = _t; //value pin
        //node->computedType = type;
    }
}

bool checkInputTypeForOperator(Node* node, PinTypeDescriptor type) {
    if (node->type != NodeType::Operator) {
        return true;
    }
    auto op = static_cast<OperatorNode*>(node);

    static std::map<OperatorType, std::set<PinType>> types = {
        { OperatorType::Plus, { PinType::Bool, PinType::Int, PinType::Float, PinType::String, PinType::List, PinType::Map } },
        { OperatorType::Minus, { PinType::Int, PinType::Float } },
    	{ OperatorType::Multiply, { PinType::Int, PinType::Float, PinType::String } },
    	{ OperatorType::Divided, { PinType::Int, PinType::Float } },
        { OperatorType::Less, { PinType::Bool, PinType::Int, PinType::Float, PinType::String } },
        { OperatorType::EqualLess, { PinType::Bool, PinType::Int, PinType::Float, PinType::String } },
        { OperatorType::Great, { PinType::Bool, PinType::Int, PinType::Float, PinType::String } },
        { OperatorType::EqualGreat, { PinType::Bool, PinType::Int, PinType::Float, PinType::String } },
        { OperatorType::Inequality, { PinType::Bool, PinType::Int, PinType::Float, PinType::String } },
        { OperatorType::Equality, { PinType::Bool, PinType::Int, PinType::Float, PinType::String } },
        { OperatorType::LogicalAnd, { PinType::Bool } },
        { OperatorType::LogicalOr, { PinType::Bool } },
    };

    if (type.type == PinType::Variable) {
        return types[op->opType].contains(type.getPinDescr()->type);
    }

    return types[op->opType].contains(type.type);
}

void updatePinsAfterDeleteLink(Link& link) {
    auto& ce = CodeEditorContext::Instance();
    auto nodeStart = ce.getPinById(link.startPinID.Get()).value()->node;
    auto nodeEnd = ce.getPinById(link.endPinID.Get()).value()->node;

    if (nodeStart->type == NodeType::Operator || nodeStart->type == NodeType::Equal) {
        bool hasConnect = false;
	    for (auto& e : nodeStart->inputs) {
            auto _id = ce.getLinkIdByPinId(e->id.Get());
            if (e->type.type != PinType::Flow && _id != -1 && _id != link.id.Get()) {
                hasConnect = true;
                break;
            }
	    }
        if (!hasConnect) {
            setInputPinsType(nodeStart, { PinType::None });
        }
    }

	if (nodeEnd->type == NodeType::Operator || nodeEnd->type == NodeType::Equal) {
        bool hasConnect = false;
        for (auto& e : nodeEnd->inputs) {
            auto _id = ce.getLinkIdByPinId(e->id.Get());
            if (e->type.type != PinType::Flow && _id != -1 && _id != link.id.Get()) {
                hasConnect = true;
                break;
            }
        }
        for (auto& e : nodeEnd->outputs) {
            auto _id = ce.getLinkIdByPinId(e->id.Get());
            if (e->type.type != PinType::Flow && _id != -1 && _id != link.id.Get()) {
                hasConnect = true;
                break;
            }
        }
        if (!hasConnect) {
            setInputPinsType(nodeEnd, { PinType::None });
        }
    }
}

void VisualCodeEditor::drawLinks() {
    auto& context = CodeEditorContext::Instance();
    // Handle creation action, returns true if editor want to create new object (node or link)
    if (ax::NodeEditor::BeginCreate())
    {
        ax::NodeEditor::PinId fromPinId, toPinId;
        if (ax::NodeEditor::QueryNewLink(&fromPinId, &toPinId)) {

            if (fromPinId && toPinId) // both are valid, let's accept link
            {
                // ed::AcceptNewItem() return true when user release mouse button.
                if (ax::NodeEditor::AcceptNewItem())
                {
                    //check links kinds, maybe nne swap
                    auto fromPin = context.pins[fromPinId.Get()];
                    auto toPin = context.pins[toPinId.Get()];
                    if (fromPin->kind == toPin->kind) {
                        std::cout << "Can not create link with same kind";
                        ax::NodeEditor::EndCreate(); // Wraps up object creation action handling.
                        return;
                    }

                    if (fromPin->kind == PinKind::Input) {
                        std::swap(fromPin, toPin);
                        std::swap(fromPinId, toPinId);
                    }

                    if (fromPin->type.type == PinType::None) {
                        std::cout << "Can not create link, input type is None";
                        ax::NodeEditor::EndCreate(); // Wraps up object creation action handling.
                        return;
                    }

                    if (!checkInputTypeForOperator(toPin->node, fromPin->type)) {
                        std::cout << "Can not create link, operator does not support this type";
                        ax::NodeEditor::EndCreate(); // Wraps up object creation action handling.
                        return;
                    }

                    if (toPin->type.type == PinType::None) {
                        setInputPinsType(toPin->node, fromPin->type);
                    }

                    //Set type for variable
                    if (fromPin->node->type == NodeType::Variable && toPin->node->type == NodeType::Equal) {
                        if (!fromPin->type.subType || fromPin->type.getPinDescr()->type == PinType::None) {
                            fromPin->type.subType = std::make_shared<PinTypeDescriptor>(toPin->node->inputs[2]->type);
                        }
                    }

                    //return
                    if (toPin->node->type == NodeType::Return && toPin->type.type != PinType::Flow) {
                        if (fromPin->type.type != PinType::None) {
                            if (fromPin->type.type == PinType::Variable) {//is variable
                                toPin->node->inputs[1]->type = *fromPin->type.getPinDescr();
                            }
                            else {//is value
                                toPin->node->inputs[1]->type = fromPin->type;
                            }
                        }
                    }


                    auto* toNode = toPin->node;
                    auto* fromNode = fromPin->node;
                    if (toPin->node->type == NodeType::This) {
                        if (fromPin->node->type == NodeType::Variable && fromPin->type.type == PinType::Object) {
                            toPin->type.subType = fromPin->type.getStr();
                            updatePinsForThisNode(*static_cast<ThisNode*>(toNode),
                                *static_cast<VariableNode*>(fromNode));
                        }
                        if (fromPin->node->type == NodeType::Variable && 
                            fromPin->type.type == PinType::Variable && fromPin->type.getPinDescr()->type == PinType::Object) {
                                toPin->type.subType = fromPin->type.getPinDescr()->getStr();
                                updatePinsForThisNode(*static_cast<ThisNode*>(toNode),
                                    *static_cast<VariableNode*>(fromNode));
                        }
                    }


                    if (!checkTypes(*fromPin, *toPin)) {
                        std::cout << "Can not create link: wrong types";
                        ax::NodeEditor::EndCreate();
                        return;
                    }

                    // Since we accepted new link, lets add one to our list of links
                    auto newLink = context.createLink(IdGenerator::GenId(), fromPinId, toPinId);

                    //fromPin->node->updateComputedType();
                    //toPin->node->updateComputedType();

                    // Draw new link.
                    ax::NodeEditor::Link(newLink->id, newLink->startPinID, newLink->endPinID);
                }
            }
        }
    }
    ax::NodeEditor::EndCreate(); // Wraps up object creation action handling.


    //Copy paste
    {
        auto countSelectedNode = ax::NodeEditor::GetSelectedNodes(nullptr, 0);
        std::vector<ax::NodeEditor::NodeId> selectedNodes;
        selectedNodes.resize(countSelectedNode);
        ax::NodeEditor::GetSelectedNodes(selectedNodes.data(), countSelectedNode);


        auto countSelectedLinks = ax::NodeEditor::GetSelectedLinks(nullptr, 0);
        std::vector<ax::NodeEditor::LinkId> selectedLinks;
        selectedLinks.resize(countSelectedLinks);
        ax::NodeEditor::GetSelectedLinks(selectedLinks.data(), countSelectedLinks);
    }

    // Handle deletion action
    if (ax::NodeEditor::BeginDelete()) {
        // There may be many links marked for deletion, let's loop over them.
        ax::NodeEditor::LinkId deletedLinkId;
        while (ax::NodeEditor::QueryDeletedLink(&deletedLinkId)) {
            // If you agree that link can be deleted, accept deletion.
            if (ax::NodeEditor::AcceptDeletedItem()) {
                // Then remove link from your data.
                for (auto link = context.links.begin(); link != context.links.end();) {
                    if (link->second->id == deletedLinkId) {
                        updatePinsAfterDeleteLink(*link->second);
                        link = context.links.erase(link);
                        break;
                    }
                    ++link;
                }
            }
        }

        ax::NodeEditor::NodeId deletedNodeId;
        while (ax::NodeEditor::QueryDeletedNode(&deletedNodeId)) {
            // If you agree that link can be deleted, accept deletion.
            if (ax::NodeEditor::AcceptDeletedItem()) {
                // Then remove link from your data.
                auto node = context.getNodeById(deletedNodeId.Get()).value();

                //delete links
                for (auto& e : node->inputs) {
                    context.deleteAllLinksWithPinId(e->id);
                }
                for (auto& e : node->outputs) {
                    context.deleteAllLinksWithPinId(e->id);
                }

                //delete pins
                for (auto& e : node->inputs) {
                    context.deletePin(e->id.Get());
                }
                for (auto& e : node->outputs) {
                    context.deletePin(e->id.Get());
                }

                context.deleteNode(deletedNodeId.Get());
            }
        }
    }
    ax::NodeEditor::EndDelete(); // Wrap up deletion action

    for (auto& [id, link] : context.links) {
        ax::NodeEditor::Link(link->id, link->startPinID, link->endPinID);
    }
}

void VisualCodeEditor::drawPinIcon(const Pin& pin, bool connected, int alpha) {
    IconType iconType;
    auto _type = pin.type.type;
	if (_type == PinType::Variable && pin.type.subType) {
        _type = std::get<std::shared_ptr<PinTypeDescriptor>>(pin.type.subType.value())->type;
    }

    ImColor  color = GetIconColor(_type);
    color.Value.w = static_cast<float>(alpha) / 255.0f;
    switch (_type) {
        case PinType::Flow:     iconType = IconType::Flow;   break;
        case PinType::Bool:     iconType = IconType::Circle; break;
        case PinType::Int:      iconType = IconType::Circle; break;
        case PinType::Float:    iconType = IconType::Circle; break;
        case PinType::String:   iconType = IconType::Circle; break;
        case PinType::Object:   iconType = IconType::Circle; break;
        case PinType::Function: iconType = IconType::Circle; break;
        case PinType::Variable: iconType = IconType::Circle; break;

        case PinType::ArrayBool:  iconType = IconType::Circle; break;
        case PinType::ArrayInt:   iconType = IconType::Circle; break;
        case PinType::ArrayFloat: iconType = IconType::Circle; break;
        case PinType::List:       iconType = IconType::Circle; break;
        case PinType::Map:        iconType = IconType::Circle; break;
        case PinType::Any:        iconType = IconType::Circle; break;
        //case PinType::Expression: iconType = IconType::Circle; break;
        case PinType::None: iconType = IconType::Circle; break;
        default:
            return;
    }
    DrawIcon(ImVec2(static_cast<float>(m_PinIconSize), static_cast<float>(m_PinIconSize)), iconType, connected, color, ImColor(32, 32, 32, alpha));
};

void VisualCodeEditor::createValueNode(Value val) {
    std::visit(overloaded{
        [this, val](int arg) {
            auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "Int", NodeType::ValueInt, val);
            CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Int } , n.get(), PinKind::Output);
        },
        [this, val](float arg) {
            auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "Float", NodeType::ValueFloat, val);
			CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Float }, n.get(), PinKind::Output);
        },
        [this, val](bool arg) {
            auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "Bool", NodeType::ValueBool, val);
			CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Bool }, n.get(), PinKind::Output);
        },
        [this, val](std::string& arg) {
            auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "String", NodeType::ValueString, val);
			CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::String }, n.get(), PinKind::Output);
        },
        [this, val](ArrayType& arg) {
            std::visit(overloaded{
                [this, val](std::vector<int>& arg) {
                    auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "Array int", NodeType::ValueArrayInt, val);
                    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::ArrayInt }, n.get(), PinKind::Output);
                },
                [this, val](std::vector<float>& arg) {
                    auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "Array float", NodeType::ValueArrayFloat, val);
					CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::ArrayFloat }, n.get(), PinKind::Output);
                },
                [this, val](std::vector<bool>& arg) {
					auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "Array bool", NodeType::ValueArrayBool, val);
					CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::ArrayBool }, n.get(), PinKind::Output); },
            }, arg);
        },
        [this, val](std::vector<std::shared_ptr<Value>>& arg) {
            auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "List", NodeType::ValueList, val);
			CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::List }, n.get(), PinKind::Output);
        },
        [this, val](std::unordered_map<int, std::shared_ptr<Value>>& arg) {
            auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "Map", NodeType::ValueMap, val);
			CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Map }, n.get(), PinKind::Output);
        },
        [this, val](std::shared_ptr<Class>& arg) {
            auto n = CodeEditorContext::Instance().createNode<ValueNode>(IdGenerator::GenId(), "Class" + arg->name, NodeType::ValueClass, val);
            CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Object }, n.get(), PinKind::Output);
            //TODO: add pins
        },
        [this, val](auto& arg) {
            int i = 0;
        },

    }, val.value.value());
}

void VisualCodeEditor::createVariableNode(PinType type) {
    auto n = CodeEditorContext::Instance().createNode<VariableNode>(IdGenerator::GenId(), "Variable", "");
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Variable, std::make_shared<PinTypeDescriptor>(type) }, n.get(), PinKind::Output);
}

void VisualCodeEditor::createStartNode() {
    auto n = CodeEditorContext::Instance().createNode<StartNode>(IdGenerator::GenId());
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().startNode = n;
   // CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createImportNode() {
    auto n = CodeEditorContext::Instance().createNode<ImportNode>(IdGenerator::GenId(), "Import", "");
}

void VisualCodeEditor::createThisNode() {
    auto n = CodeEditorContext::Instance().createNode<ThisNode>(IdGenerator::GenId(), 0);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "obj", { PinType::Object }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createEqualNode(PinType type) {
    auto n = CodeEditorContext::Instance().createNode<EqualNode>(IdGenerator::GenId());
    //n->computedType = { type };
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Var", { PinType::Variable }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Val", { type }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createBinaryOperatorNode(const std::string& name, PinType type) {
    auto n = CodeEditorContext::Instance().createNode<OperatorNode>(IdGenerator::GenId(), name, StringToEnum(name));
    //n->computedType = { type } ;
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Result", { type } , n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "1", { type }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "2", { type }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createBinaryOperatorLogicNode(const std::string& name, PinType type) {
    auto n = CodeEditorContext::Instance().createNode<OperatorNode>(IdGenerator::GenId(), name, StringToEnum(name));
    //n->computedType = { PinType::Bool };
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Result", { PinType::Bool }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "1", { type } , n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "2", { type}, n.get(), PinKind::Input);
}

void VisualCodeEditor::createFunctionNode() {
    auto n = CodeEditorContext::Instance().createNode<FunNode>(IdGenerator::GenId(), "Fun");
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Body", { PinType::Flow } , n.get(), PinKind::Output);
    CodeEditorContext::Instance().registerFunctionCall(n);

    std::weak_ptr<FunNode> weak = n;
    addButton("FunCall",
        [n]() -> const std::string& {
        return n->funName;
    }, 
    [this, weak]() {
        if (auto node = weak.lock()) {
            createFunCall(node);
            return;
        }
		throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));
    });
}

void VisualCodeEditor::createClassNode() {
    auto n = CodeEditorContext::Instance().createNode<ClassNode>(IdGenerator::GenId(), "Class");
    CodeEditorContext::Instance().registerClassCall(n);

    std::weak_ptr<ClassNode> weak = n;
    addButton("Types constant",
        [n]() -> const std::string& {
            return n->className;
        },
        [this, weak]() {
            if (auto node = weak.lock()) {
                auto classVal = std::make_shared<Class>();
                classVal->name = node->className;
                classVal->classDeclarationId = node->id.Get();
                createValueNode(Value{ classVal });
                return;
            }
        throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));
    });
    addButton("ClassVariable",
        [n]() -> const std::string& {
            static std::string s;
			s = "Var " + n->className;
			return s;
        },
        [this, weak]() {
            if (auto node = weak.lock()) {
                createClassInstanceNode(node);
                return;
            }
        throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));
    });

    addButton("ClassMethod",
        [n]() -> const std::string& {
            return "New method " + n->className;
        },
        [this, weak]() {
            if (auto node = weak.lock()) {
                createClassMethodNode(node);
                return;
            }
        throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));
    });
}

void VisualCodeEditor::createClassMethodNode(std::shared_ptr<ClassNode> classNode) {
    auto n = CodeEditorContext::Instance().createNode<MethodNode>(IdGenerator::GenId(), "Method " + classNode->className, classNode->id.Get());
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Body", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "this", { PinType::Object, classNode->className }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().registerMethodCall(n);

    std::weak_ptr<MethodNode> weak = n;
    addButton("MethodCall",
        [n, className=classNode->className]() -> const std::string& {
            return className + " " + n->funName;
        },
        [this, weak]() {
            if (auto node = weak.lock()) {
                createMethodCall(node);
                return;
            }
        throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));
    });
}

void VisualCodeEditor::createMethodCall(std::shared_ptr<MethodNode> fnode) {
    auto n = CodeEditorContext::Instance().createNode<MethodCall>(IdGenerator::GenId(), fnode->id.Get());

    auto classNode = std::static_pointer_cast<ClassNode>(CodeEditorContext::Instance().getNodeById(fnode->classNodeId).value());
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), classNode->className, { PinType::Object, classNode->className }, n.get(), PinKind::Input);
    for (auto& [id, data] : fnode->inputVars) {
        CodeEditorContext::Instance().createPin(IdGenerator::GenId(), data.first, { data.second }, n.get(), PinKind::Input);
    }
    //CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", PinType::Flow, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Res", { fnode->outputType }, n.get(), PinKind::Output);
}

void VisualCodeEditor::createClassInstanceNode(std::shared_ptr<ClassNode> classNode) {
    auto n = CodeEditorContext::Instance().createNode<VariableNode>(IdGenerator::GenId(), "Variable", "");
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Variable, std::make_shared<PinTypeDescriptor>(PinType::Object, classNode->className) }, n.get(), PinKind::Output);
}

void VisualCodeEditor::createVariableNode(PinTypeDescriptor type) {
    auto n = CodeEditorContext::Instance().createNode<VariableNode>(IdGenerator::GenId(), "Variable", "");
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Variable, std::make_shared<PinTypeDescriptor>(type)}, n.get(), PinKind::Output);
}

void VisualCodeEditor::createIfElseNode() {
    auto n = CodeEditorContext::Instance().createNode<Node>(IdGenerator::GenId(), "IfElse", NodeType::If);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "True", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "False", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Complete", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Cond", { PinType::Bool }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createForNode() {
    auto n = CodeEditorContext::Instance().createNode<Node>(IdGenerator::GenId(), "For", NodeType::For);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Body", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Complete", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Index", { PinType::Variable, std::make_shared<PinTypeDescriptor>(PinType::Int) }, n.get(), PinKind::Output); //var with type int
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Start", { PinType::Int }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "End", { PinType::Int }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Step", { PinType::Int }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createWhileNode() {
    auto n = CodeEditorContext::Instance().createNode<Node>(IdGenerator::GenId(), "While", NodeType::While);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Body", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Complete", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Expr", { PinType::Bool }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createForEachNode() {
    auto n = CodeEditorContext::Instance().createNode<Node>(IdGenerator::GenId(), "ForEach", NodeType::ForEach);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Body", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Complete", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Var", { PinType::Variable }, n.get(), PinKind::Output); //Elem of container
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Container", { PinType::Container }, n.get(), PinKind::Input); //Type Container
}

void VisualCodeEditor::createReturn(PinType type) {
    auto n = CodeEditorContext::Instance().createNode<Node>(IdGenerator::GenId(), "Return", NodeType::Return);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Out", { type }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createContinue() {
    auto n = CodeEditorContext::Instance().createNode<Node>(IdGenerator::GenId(), "Continue", NodeType::Continue);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createBreak() {
    auto n = CodeEditorContext::Instance().createNode<Node>(IdGenerator::GenId(), "Break", NodeType::Break);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
}

void VisualCodeEditor::createFunCall(std::shared_ptr<FunNode> fnode) {
    auto n = CodeEditorContext::Instance().createNode<FunCall>(IdGenerator::GenId(), fnode->id.Get());
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Output);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", { PinType::Flow }, n.get(), PinKind::Input);
    for (auto& [id, data] : fnode->inputVars) {
        CodeEditorContext::Instance().createPin(IdGenerator::GenId(), data.first, { data.second }, n.get(), PinKind::Input);
    }
    //CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "", PinType::Flow, n.get(), PinKind::Input);
    CodeEditorContext::Instance().createPin(IdGenerator::GenId(), "Res", { fnode->outputType }, n.get(), PinKind::Output);
}

void VisualCodeEditor::createComment() {
    auto n = CodeEditorContext::Instance().createNode<CommentNode>(IdGenerator::GenId(), "");
    n->size = ImVec2(300.0f, 300.0f);
    ax::NodeEditor::SetGroupSize(n->id, ImVec2(640, 400));
}

void VisualCodeEditor::drawNode(Node& node) {
    auto alpha = ImGui::GetStyle().Alpha;

    ax::NodeEditor::BeginNode(node.id);
    //ImGui::Text("Node A");
    ImGui::PushID(node.id.AsPointer());

    

    node.run(nodeDrawer);
    

    ImGui::BeginGroup();
    for (auto& pin : node.inputs) {
        ax::NodeEditor::BeginPin(pin->id, ax::NodeEditor::PinKind::Input);
        ax::NodeEditor::PinPivotAlignment(ImVec2(0.0f, 0.5f));
        ax::NodeEditor::PinPivotSize(ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

        drawPinIcon(*pin, CodeEditorContext::Instance().isPinLinked(pin->id), (int)(alpha * 255));

        ImGui::SameLine();
        
    	ImGui::Text(pin->name.c_str());
        ImGui::PopStyleVar();
        ax::NodeEditor::EndPin();
    }
    ImGui::EndGroup();


    const float nodeWidthHalf = 100.0f;
    ImGui::SameLine(nodeWidthHalf);

	ImGui::BeginGroup();
    for (auto& pin : node.outputs) {
        ax::NodeEditor::BeginPin(pin->id, ax::NodeEditor::PinKind::Output);
        ax::NodeEditor::PinPivotAlignment(ImVec2(1.0f, 0.5f));
        ax::NodeEditor::PinPivotSize(ImVec2(0, 0));
        //ImGui::BeginHorizontal(output.ID.AsPointer());
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

        ImGui::Text("");
        ImVec2 textSize = ImGui::CalcTextSize(pin->name.c_str());
        ImGui::SameLine(std::max(nodeWidthHalf - textSize.x - 30.0f, 0.0f));
        ImGui::Text(pin->name.c_str());
        ImGui::SameLine();
        drawPinIcon(*pin, CodeEditorContext::Instance().isPinLinked(pin->id), (int)(alpha * 255));
        //ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
        //ImGui::EndHorizontal();
        ImGui::PopStyleVar();
        ax::NodeEditor::EndPin();
    }
	ImGui::EndGroup();
        
    

    ImGui::PopID();
    

    //if (ImGui::Button("Add")) {
    //    node.Outputs.push_back(Pin(genId(), "-> Out", PinType::Flow));
    //}

    ax::NodeEditor::EndNode();

    //Header background
    auto HeaderMin = ImGui::GetItemRectMin();
    HeaderMin.x += 8;
    HeaderMin.y += 5;
    auto HeaderMax = ImGui::GetItemRectMax();
    HeaderMax.y = HeaderMin.y + 20;
    HeaderMax.x -= 8;


    ImColor HeaderColor = ImColor(ImVec4(0, 0, 255, 255));
    if (ImGui::IsItemVisible())
    {
        auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

        auto drawList = ax::NodeEditor::GetNodeBackgroundDrawList(node.id);

        const auto halfBorderWidth = ax::NodeEditor::GetStyle().NodeBorderWidth * 0.5f;

        auto headerColor = IM_COL32(0, 0, 0, alpha) | (HeaderColor & IM_COL32(255, 255, 255, 0));
        if ((HeaderMax.x > HeaderMin.x) && (HeaderMax.y > HeaderMin.y) && HeaderBackground)
        {
            const auto uv = ImVec2(
                (HeaderMax.x - HeaderMin.x) / (float)(4.0f * 64),
                (HeaderMax.y - HeaderMin.y) / (float)(4.0f * 64));

            drawList->AddImageRounded(HeaderBackground,
                HeaderMin - ImVec2(8 - halfBorderWidth, 4 - halfBorderWidth),
                HeaderMax + ImVec2(8 - halfBorderWidth, 0),
                ImVec2(0.0f, 0.0f), uv,
                headerColor, ax::NodeEditor::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);


            auto headerSeparatorMin = ImVec2(HeaderMin.x, HeaderMax.y);
            auto headerSeparatorMax = ImVec2(HeaderMax.x, HeaderMin.y);

            //if ((headerSeparatorMax.x > headerSeparatorMin.x) && (headerSeparatorMax.y > headerSeparatorMin.y))
            //{
            //    drawList->AddLine(
            //        headerSeparatorMin + ImVec2(-(8 - halfBorderWidth), -0.5f),
            //        headerSeparatorMax + ImVec2((8 - halfBorderWidth), -0.5f),
            //        ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
            //}
        }
    }

    // Hint for Node

    if (node.type == NodeType::Comment && ax::NodeEditor::BeginGroupHint(node.id)) {
        auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

        auto min = ax::NodeEditor::GetGroupMin();

        ImGui::SetCursorScreenPos(ImVec2(min.x + 8, min.y - ImGui::GetTextLineHeightWithSpacing() + 4));
        ImGui::BeginGroup();
        ImGui::TextUnformatted(static_cast<CommentNode&>(node).value.c_str());
        ImGui::EndGroup();

        auto drawList = ax::NodeEditor::GetHintBackgroundDrawList();

        auto hintBounds = ImGui_GetItemRect();
        auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

        drawList->AddRectFilled(
            hintFrameBounds.GetTL(),
            hintFrameBounds.GetBR(),
            IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

        drawList->AddRect(
            hintFrameBounds.GetTL(),
            hintFrameBounds.GetBR(),
            IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);
    }
    ax::NodeEditor::EndGroupHint();

}
