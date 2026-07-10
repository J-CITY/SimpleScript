#pragma once
#include "../Core/GraphModel.hpp"

namespace Visual {

    struct Visitor {
        virtual ~Visitor() = default;

        virtual void run(ValueNode&    node) = 0;
        virtual void run(VariableNode& node) = 0;
        virtual void run(Node&         node) = 0; // generic handler for typed nodes (If, For, etc.)
        virtual void run(FunNode&      node) = 0;
        virtual void run(FunCall&      node) = 0;
        virtual void run(ImportNode&   node) = 0;
        virtual void run(CommentNode&  node) = 0;
        virtual void run(OperatorNode& node) = 0;
        virtual void run(ClassNode&    node) = 0;
        virtual void run(MethodNode&   node) = 0;
        virtual void run(MethodCall&   node) = 0;
        virtual void run(ThisNode&     node) = 0;
        virtual void run(EqualNode&    node) = 0;
        virtual void run(StartNode&    node) = 0;
    };

} // namespace Visual
