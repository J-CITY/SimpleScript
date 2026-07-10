#pragma once
#include "Visitor.hpp"
#include "../Mapping/BpSourceMap.hpp"

namespace Visual {

    struct GraphContext;

    struct CodeGenerator final : Visitor {
        explicit CodeGenerator(GraphContext& ctx);

        // --- Results (also accessible via emitter directly for compileGraph) ---
        const std::string& getCode()    const { return emitter.code; }
        const BpSourceMap& getSourceMap() const { return emitter.map; }

        // Made public so compileGraph() can configure and read results.
        CodeEmitter emitter;

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

        // Variable initialisation helpers
        void setVar(VariableNode& node);
        void setVar(ThisNode& node, PinPtr pin);

        // Expression / sub-expression generation
        void genCodeForExpression(NodePtr node, Pin& pin);
        void genCodeForFuncInputVar(FunNode& node, Pin& pin);
        void genCodeForLoopIndexVar(Node& node);
    };

    // Compile the full graph and return generated script + source map.
    struct CompileResult {
        std::string  script;
        BpSourceMap  sourceMap;
    };

    CompileResult compileGraph(GraphContext& ctx, const std::string& graphName = "");

} // namespace Visual
