#pragma once
#include "../json.hpp"
#include <stack>

#include "VisualNodes.h"


namespace Visual {

    struct Node;
    struct ValueNode;
    struct VariableNode;

    struct Visitor {
        virtual ~Visitor() = default;
        virtual void run(ValueNode& node) = 0;
        virtual void run(VariableNode& node) = 0;
        virtual void run(Node& node) = 0;
        virtual void run(FunNode& node) = 0;
        virtual void run(FunCall& node) = 0;
        virtual void run(ImportNode& node) = 0;
        virtual void run(CommentNode& node) = 0;
        virtual void run(OperatorNode& node) = 0;
        virtual void run(ClassNode& node) = 0;
        virtual void run(MethodNode& node) = 0;
        virtual void run(MethodCall& node) = 0;
        virtual void run(ThisNode& node) = 0;
        virtual void run(EqualNode& node) = 0;
        virtual void run(StartNode& node) = 0;
    };


    struct NodeDrawer final : public Visitor {
        void run(ValueNode& node) override;
        void run(VariableNode& node) override;
        void run(Node& node) override;
        void run(FunNode& node) override;
        void run(FunCall& node) override;
        void run(ImportNode& node) override;
        void run(CommentNode& node) override;
        void run(OperatorNode& node) override;
        void run(ClassNode& node) override;
        void run(MethodNode& node) override;
        void run(MethodCall& node) override;
        void run(ThisNode& node) override;
        void run(EqualNode& node) override;
        void run(StartNode& node) override;
    };


    struct CodeGenerator final : public Visitor {
        const std::string ENDL = "\n";
        void run(ValueNode& node) override;
        void run(VariableNode& node) override;
        void setVar(VariableNode& node);
        void setVar(ThisNode& node, PinPtr pin);
        void run(Node& node) override;
        void run(FunNode& node) override;
        void run(FunCall& node) override;
        void run(ImportNode& node) override;
        void run(CommentNode& node) override;
        void run(OperatorNode& node) override;
        void appendToken(std::string token);
        void endToken();
        void endLineToken();
        void addStartScopeToken();
        void addEndScopeToken();
        void addTabsToken();
        void run(ClassNode& node) override;
        void run(MethodNode& node) override;
        void run(MethodCall& node) override;
        void run(ThisNode& node) override;
        void run(EqualNode& node) override;
        void run(StartNode& node) override;

        int tabsCount = 0;

        void genCodeForFuncInputVar(FunNode& node, Pin& pin);
        void genCodeForLoopIndexVar(Node& node);
        void genCodeForExpression(NodePtr node, Pin& pin);
        
        std::string code;
    };

    struct CodeExecutor final : public Visitor {
        std::stack<ValuePtr> expressionValues;

        void run(ValueNode& node) override;
        void run(VariableNode& node) override;
        void run(Node& node) override;
        void run(FunNode& node) override;
        void run(FunCall& node) override;
        void run(ImportNode& node) override;
        void run(CommentNode& node) override;
        void run(OperatorNode& node) override;
        void run(ClassNode& node) override;
        void run(MethodNode& node) override;
        void run(MethodCall& node) override;
        void run(ThisNode& node) override;
        void run(EqualNode& node) override;
        void run(StartNode& node) override;


        ValuePtr executeExpression() {
	        
        }
        //void genCodeForFuncInputVar(FunNode& node, Pin& pin);
        //void genCodeForLoopIndexVar(Node& node);
        //void getnCodeForExpression(NodePtr node, Pin& pin);
    };


    struct SaveNode final : public Visitor {
        nlohmann::json data;
        SaveNode(std::string_view path);
        void saveBase(nlohmann::json& nd, Node& node);
        void run(ValueNode& node) override;
        void run(VariableNode& node) override;
        void run(Node& node) override;
        void run(FunNode& node) override;
        void run(FunCall& node) override;
        void run(ImportNode& node) override;
        void run(CommentNode& node) override;
        void run(OperatorNode& node) override;
        void run(ClassNode& node) override;
        void run(MethodNode& node) override;
        void run(MethodCall& node) override;
        void run(ThisNode& node) override;
        void run(EqualNode& node) override;
        void run(StartNode& node) override;

        void saveLinks();
    };
}

