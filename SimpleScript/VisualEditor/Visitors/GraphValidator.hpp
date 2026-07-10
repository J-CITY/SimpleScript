#pragma once
#include "Visitor.hpp"
#include "../Core/NodeRegistry.hpp"
#include <string>
#include <vector>

namespace Visual {

    struct GraphContext;

    struct ValidationError {
        Id          nodeId;
        std::string message;
    };

    // Validates the graph before code generation.
    // Returns an empty error list on success.
    struct GraphValidator final : Visitor {
        explicit GraphValidator(GraphContext& ctx);

        std::vector<ValidationError> validate();

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
        std::vector<ValidationError> errors_;

        void addError(Id nodeId, std::string msg) {
            errors_.push_back({nodeId, std::move(msg)});
        }
        bool flowPinConnected(Node& node, int outputIdx);
    };

} // namespace Visual
