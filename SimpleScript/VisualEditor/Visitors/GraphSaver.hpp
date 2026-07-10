#pragma once
#include "Visitor.hpp"
#include <memory>
#include <string>
#include <string_view>

namespace Visual {

    struct GraphContext;

    // Serialises the full graph (nodes + links + layout positions) to JSON.
    // Layout positions are read from node.layoutPosition — no ImGui calls required.
    struct GraphSaver final : Visitor {
        explicit GraphSaver(GraphContext& ctx);
        ~GraphSaver();

        // Serialise all nodes and write to file. Returns true on success.
        bool save(std::string_view path);

        // --- Visitor interface ---
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
        // Opaque pointer to implementation (holds nlohmann::json internally).
        struct Impl;
        std::unique_ptr<Impl> impl_;

        // Helper called by each run() overload.
        void saveBaseImpl(Impl& impl, Node& node);
        void saveLinks(Impl& impl);
    };

} // namespace Visual
