#pragma once
#include "../visitor/ast_visitor.hpp"
#include "chunk.hpp"
#include <stack>
#include <vector>
#include <string>
#include <unordered_map>

namespace IkigaiScript {

	class IkigaiScriptInterpreter;

	// Compiles a vector of AST expressions (a function body or top-level block)
	// into a BytecodeFunction.
	//
	// Usage:
	//   BytecodeCompiler compiler(interp);
	//   auto fn = compiler.compileFunction(funcRef);
	//   auto chunk = compiler.compileTopLevel(statements, scope);
	//
	// The compiler does NOT modify the interpreter state — it is a pure read-only
	// pass over the AST.  Runtime semantics (type checking, scope resolution) are
	// preserved by the VM at execution time using the same helpers as the AST path.
	class BytecodeCompiler: public AstVisitor {
	public:
		explicit BytecodeCompiler(IkigaiScriptInterpreter* interp);

		// Compile a script-defined function's body into a BytecodeFunctionRef.
		BytecodeFunctionRef compileFunction(FunctionRef fnc);

		// Compile a sequence of top-level statements into a BytecodeFunctionRef.
		BytecodeFunctionRef compileStatements(const std::vector<ExpressionPtr>& stmts,
			const std::string& name = "__main__");

	private:
		IkigaiScriptInterpreter* interpreter_;
		BytecodeFunctionRef current_;  // function being compiled

		// Local variable table for the current function scope.
		struct LocalVar {
			std::string name;
			uint16_t slot = 0;
		};
		std::vector<LocalVar> locals_;
		uint16_t localCount_ = 0;

		// Pending break/continue jump offsets to patch.
		struct LoopContext {
			std::vector<size_t> breakJumps;
			std::vector<size_t> continueJumps;
			size_t loopStart = 0;
		};
		std::vector<LoopContext> loopStack_;

		// Helpers
		void compile(ExpressionPtr node);
		void compileBlock(const std::vector<ExpressionPtr>& stmts);

		int resolveLine(ExpressionPtr node) const;
		uint16_t addConst(ValuePtr v);
		uint16_t addName(const std::string& name);

		int resolveLocal(const std::string& name) const;

		void pushLoopContext(size_t loopStart);
		void popLoopContext();
		void patchBreaks();
		void patchContinues();

		// AstVisitor overrides — one per node type
		void visit(ValueNode& n) override;
		void visit(ResolveVar& n) override;
		void visit(DefineVar& n) override;
		void visit(FunctionExpression& n) override;
		void visit(Return& n) override;
		void visit(Yield& n) override;
		void visit(Continue& n) override;
		void visit(Break& n) override;
		void visit(IfElse& n) override;
		void visit(Loop& n) override;
		void visit(Foreach& n) override;
		void visit(MemberVariable& n) override;
		void visit(MemberFunctionCall& n) override;
		void visit(MatchExpression& n) override;
		void visit(TupleLiteralExpression& n) override;
		void visit(DestructuringAssign& n) override;
		void visit(BlockExpression& n) override;
		void visit(SafeBlockExpression& n) override;
		void visit(DeferExpression& n) override;
		void visit(LiveRebind& n) override;
		void visit(AwaitExpression& n) override;
		void visit(SpawnExpression& n) override;
		void visit(SyncBlockExpression& n) override;
		void visit(RaceBlockExpression& n) override;
		void visit(BranchBlockExpression& n) override;
		void visit(NamedArgumentExpression& n) override;

		// Compile a FunctionCall expression (most complex case).
		void compileFunctionCall(FunctionExpression& n);
	};

} // namespace IkigaiScript
