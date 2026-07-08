#pragma once
#include "../expressions.hpp"

namespace IkigaiScript {

	struct AstVisitor {
		virtual ~AstVisitor() = default;

		virtual void visit(Expression&) {
		}
		virtual void visit(FunctionExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(MemberVariable& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(MemberFunctionCall& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(Return& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(Yield& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(Continue& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(Break& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(IfElse& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(Loop& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(Foreach& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(DeferExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(ResolveVar& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(DefineVar& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(LiveRebind& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(NamedArgumentExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(ValueNode& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(BlockExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(MatchExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(TupleLiteralExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(DestructuringAssign& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(AwaitExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(SpawnExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(SyncBlockExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(RaceBlockExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(BranchBlockExpression& n) {
			visit(static_cast<Expression&>(n));
		}
		virtual void visit(SafeBlockExpression& n) {
			visit(static_cast<Expression&>(n));
		}

		void visitChildren(Expression& node) {
			for (auto it = node.begin(); it != node.end(); ++it) {
				if (*it) (*it)->accept(*this);
			}
		}
	};

} // namespace IkigaiScript
