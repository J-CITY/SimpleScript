#pragma once
#include <memory>
#include "../Core/GraphContext.hpp"
#include <imgui_node_editor.h>

namespace Visual {

    // Owns the ax::NodeEditor::EditorContext and synchronises node layout positions
    // between the model (node.layoutPosition) and the NodeEditor API.
    struct NodeEditorSession {
        NodeEditorSession();
        ~NodeEditorSession();

        ax::NodeEditor::EditorContext* editorContext() const { return ctx_.get(); }

        // After loading a graph, push stored positions into NodeEditor.
        void applyLayout(GraphContext& graphCtx);

        // After a frame where nodes may have been dragged, pull positions back into model.
        void syncLayout(GraphContext& graphCtx);

        void setAsCurrent() { ax::NodeEditor::SetCurrentEditor(ctx_.get()); }
        void clearCurrent() { ax::NodeEditor::SetCurrentEditor(nullptr); }

    private:
        std::unique_ptr<ax::NodeEditor::EditorContext,
                        void(*)(ax::NodeEditor::EditorContext*)> ctx_;
    };

} // namespace Visual
