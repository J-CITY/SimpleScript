#pragma once
#include "../Visitors/Visitor.hpp"

namespace Visual {

    struct GraphContext;

    // Visitor that draws each node's inner content using ImGui widgets.
    // Runs inside ax::NodeEditor::BeginNode() / EndNode() calls in VisualCodeEditor.
    struct NodeDrawer final : Visitor {
        explicit NodeDrawer(GraphContext& ctx);

        void run(ValueNode&    node) override;
        void run(VariableNode& node) override;
        void run(Node&         node) override;
        void run(FunNode&      node) override;
        void run(FunCall&      node) override;
        void run(ImportNode&   node) override;
        void run(CommentNode&  node) override;
        void run(OperatorNode& node) override;
        void run(ClassNode&    node) override;
        void run(MethodNode&   node) override;
        void run(MethodCall&   node) override;
        void run(ThisNode&     node) override;
        void run(EqualNode&    node) override;
        void run(StartNode&    node) override;

    private:
        GraphContext& ctx_;
    };

} // namespace Visual
