#include "NodeEditorSession.hpp"
#include "ImGuiTypes.hpp"

using namespace Visual;

NodeEditorSession::NodeEditorSession()
    : ctx_(ax::NodeEditor::CreateEditor(), ax::NodeEditor::DestroyEditor) {}

NodeEditorSession::~NodeEditorSession() = default;

void NodeEditorSession::applyLayout(GraphContext& graphCtx) {
    for (auto& [id, node] : graphCtx.nodes) {
        ax::NodeEditor::SetNodePosition(
            UI::toEditorNodeId(node->id),
            UI::toImVec2(node->layoutPosition));
    }
}

void NodeEditorSession::syncLayout(GraphContext& graphCtx) {
    for (auto& [id, node] : graphCtx.nodes) {
        auto pos = ax::NodeEditor::GetNodePosition(UI::toEditorNodeId(node->id));
        node->layoutPosition = UI::fromImVec2(pos);
    }
}
