#pragma once

// Conversion helpers between plain Visual::Id / Visual::Vec2 / Visual::ColorRGBA
// and the imgui-node-editor / ImGui native types.
// Only included in UI-layer translation units.

#include "../../imgui/imgui.h"
#include "../../imgui/imgui-node-editor/imgui_node_editor.h"
#include "../Core/GraphTypes.hpp"

namespace Visual::UI {

    // ID conversions
    inline ax::NodeEditor::NodeId toEditorNodeId(Id id) { return ax::NodeEditor::NodeId(id); }
    inline ax::NodeEditor::PinId  toEditorPinId(Id id)  { return ax::NodeEditor::PinId(id);  }
    inline ax::NodeEditor::LinkId toEditorLinkId(Id id) { return ax::NodeEditor::LinkId(id); }

    inline Id fromEditorNodeId(ax::NodeEditor::NodeId id) { return static_cast<Id>(id.Get()); }
    inline Id fromEditorPinId(ax::NodeEditor::PinId   id) { return static_cast<Id>(id.Get()); }
    inline Id fromEditorLinkId(ax::NodeEditor::LinkId  id) { return static_cast<Id>(id.Get()); }

    // Color / vector conversions
    inline ImColor  toImColor(const ColorRGBA& c) { return ImColor(c.r, c.g, c.b, c.a); }
    inline ImVec2   toImVec2(const Vec2& v)       { return ImVec2(v.x, v.y); }
    inline Vec2     fromImVec2(const ImVec2& v)   { return {v.x, v.y}; }
    inline ColorRGBA fromImColor(const ImColor& c) {
        return ColorRGBA(c.Value.x, c.Value.y, c.Value.z, c.Value.w);
    }

} // namespace Visual::UI
