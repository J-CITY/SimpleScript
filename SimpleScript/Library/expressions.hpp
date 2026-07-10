#pragma once
#include "arena.hpp"
#include "enums.h"
#include "value.hpp"

namespace IkigaiScript {
	struct If;
	struct Loop;
	struct Return;
	struct MemberVariable;
	struct MemberFunctionCall;
	struct FunctionExpression;
	struct BlockExpression;
	struct SafeBlockExpression;
	struct DefineVar;
	struct ResolveVar;
	struct Foreach;
	class IkigaiScriptInterpreter;
	struct AstVisitor;

	struct LineInfo {
		LineInfo();
		int line = 0;
	};

	struct Expression: public LineInfo {
		ExpressionType type;
		ExpressionPtr parent = nullptr;
		// Blueprint node ID that generated this expression (0 = not from a blueprint).
		// Set by the parser when a @bp(node=N) decorator precedes the statement.
		int bpNodeId = 0;
		Expression(ExpressionType ty, ExpressionPtr par = nullptr): type(ty), parent(par) {
		}

		virtual ~Expression() = default;

		virtual void accept(AstVisitor& v);

		virtual ExpressionPtr back() {
			return nullptr;
		}

		virtual std::vector<ExpressionPtr>::iterator begin() {
			return std::vector<ExpressionPtr>::iterator();
		}

		virtual std::vector<ExpressionPtr>::iterator end() {
			return std::vector<ExpressionPtr>::iterator();
		}

		virtual void push_back(ExpressionPtr ref) {
		}

		virtual void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) {
		}

	};

	struct FunctionExpression: public Expression {
		ValuePtr function;
		std::vector<ExpressionPtr> subexpressions;

		FunctionExpression(FunctionRef fnc, ExpressionPtr par): Expression(ExpressionType::FunctionDef, par), function(new Value(fnc)) {
		}
		FunctionExpression(ValuePtr fncvalue):Expression(ExpressionType::FunctionCall), function(fncvalue) {
		}

		void accept(AstVisitor& v) override;

		void clear() {
			subexpressions.clear();
		}

		~FunctionExpression() override {
			clear();
		}

		ExpressionPtr back() override {
			switch (type) {
				case ExpressionType::FunctionDef:
				return std::get<std::vector<ExpressionPtr>>(function->getFunction()->body).back();
				break;
				case ExpressionType::FunctionCall:
				return subexpressions.back();
				break;
				default:
				break;
			}
			return nullptr;
		}

		std::vector<ExpressionPtr>::iterator begin()override {
			switch (type) {
				case ExpressionType::FunctionCall:
				return subexpressions.begin();
				break;
				case ExpressionType::FunctionDef:
				return std::get<std::vector<ExpressionPtr>>(function->getFunction()->body).begin();
				break;
				default:
				return std::vector<ExpressionPtr>::iterator();
				break;
			}
		}

		std::vector<ExpressionPtr>::iterator end()override {
			switch (type) {
				case ExpressionType::FunctionCall:
				return subexpressions.end();
				break;
				case ExpressionType::FunctionDef:
				return get<std::vector<ExpressionPtr>>(function->getFunction()->body).end();
				break;
				default:
				return std::vector<ExpressionPtr>::iterator();
				break;
			}
		}

		void push_back(ExpressionPtr ref)override {
			switch (type) {
				case ExpressionType::FunctionCall:
				subexpressions.push_back(ref);
				break;
				case ExpressionType::FunctionDef:
				std::get<std::vector<ExpressionPtr>>(function->getFunction()->body).push_back(ref);
				break;
				default:
				break;
			}
		}

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			switch (type) {
				case ExpressionType::FunctionCall:
				for (auto& s : subexpressions) {
					if (s == oldNode) s = newNode;
				}
				break;
				case ExpressionType::FunctionDef:
				{
					auto& body = get<std::vector<ExpressionPtr>>(function->getFunction()->body);
					for (auto& s : body) {
						if (s == oldNode) s = newNode;
					}
					break;
				}
				default:
				break;
			}
		}
	};

	struct MemberVariable: public Expression {
		ExpressionPtr object = nullptr;
		std::string name;

		MemberVariable(ExpressionPtr ob, const std::string& name_): Expression(ExpressionType::MemberVariable), object(ob), name(name_) {
		}
		MemberVariable(): Expression(ExpressionType::MemberVariable) {
		}

		void accept(AstVisitor& v) override;
	};

	struct MemberFunctionCall: public Expression {
		ExpressionPtr object = nullptr;
		std::string functionName;
		std::vector<ExpressionPtr> subexpressions;

		MemberFunctionCall(ExpressionPtr ob, const std::string& fncvalue, const std::vector<ExpressionPtr>& sub): Expression(ExpressionType::MemberFunctionCall)
			, object(ob), functionName(fncvalue), subexpressions(sub) {
		}
		MemberFunctionCall(): Expression(ExpressionType::MemberFunctionCall) {
		}

		void clear() {
			subexpressions.clear();
		}

		void accept(AstVisitor& v) override;

		~MemberFunctionCall() override {
			clear();
		}
	};

	struct Return: public Expression {
		ExpressionPtr expression = nullptr;

		Return(ExpressionPtr e, ExpressionPtr par = nullptr): Expression(ExpressionType::Return, par), expression(e) {
		}
		Return(): Expression(ExpressionType::Return) {
		}

		void accept(AstVisitor& v) override;
	};

	struct Yield: public Expression {
		ExpressionPtr expression = nullptr;

		Yield(ExpressionPtr e, ExpressionPtr par = nullptr): Expression(ExpressionType::Yield, par), expression(e) {
		}
		Yield(): Expression(ExpressionType::Yield) {
		}

		void accept(AstVisitor& v) override;
	};

	struct Continue: public Expression {

		Continue(ExpressionPtr par): Expression(ExpressionType::Continue, par) {
		}
		Continue(): Expression(ExpressionType::Continue) {
		}

		void accept(AstVisitor& v) override;
	};

	struct Break: public Expression {

		Break(ExpressionPtr par): Expression(ExpressionType::Break, par) {
		}
		Break(): Expression(ExpressionType::Break) {
		}

		void accept(AstVisitor& v) override;
	};

	struct If {
		ExpressionPtr testExpression = nullptr;
		std::vector<ExpressionPtr> subexpressions;

		If() {
		}
	};

	struct IfElse: public Expression {
		IfElse(): Expression(ExpressionType::IfElse) {
		}
		IfElse(std::vector<If>& val, ExpressionPtr par = nullptr):
			Expression(ExpressionType::IfElse, par), states(val) {
		}

		std::vector<If> states;

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return states.back().subexpressions.back();
		}

		std::vector<ExpressionPtr>::iterator begin()override {
			return states.back().subexpressions.begin();
		}

		std::vector<ExpressionPtr>::iterator end()override {
			return states.back().subexpressions.end();
		}

		void push_back(ExpressionPtr ref)override {
			states.back().subexpressions.push_back(ref);
		}

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& st : states) {
				for (auto& s : st.subexpressions) {
					if (s == oldNode) s = newNode;
				}
			}
		}
	};

	struct Loop: public Expression {
		ExpressionPtr initExpression = nullptr;
		ExpressionPtr testExpression = nullptr;
		ExpressionPtr iterateExpression = nullptr;
		std::vector<ExpressionPtr> subexpressions;


		Loop(ExpressionPtr par): Expression(ExpressionType::Loop, par) {
		}
		Loop(): Expression(ExpressionType::Loop) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.back();
		}

		std::vector<ExpressionPtr>::iterator begin()override {
			return subexpressions.begin();
		}

		std::vector<ExpressionPtr>::iterator end()override {
			return subexpressions.end();
		}

		void push_back(ExpressionPtr ref)override {
			subexpressions.push_back(ref);
		}

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) {
				if (s == oldNode) s = newNode;
			}
		}
	};

	struct Foreach: public Expression {
		ExpressionPtr listExpression = nullptr;
		std::vector<std::string> iterateNames;
		std::vector<ExpressionPtr> subexpressions;

		Foreach(ExpressionPtr par = nullptr): Expression(ExpressionType::ForEach, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.back();
		}

		std::vector<ExpressionPtr>::iterator begin()override {
			return subexpressions.begin();
		}

		std::vector<ExpressionPtr>::iterator end()override {
			return subexpressions.end();
		}

		void push_back(ExpressionPtr ref)override {
			subexpressions.push_back(ref);
		}
	};

	struct DeferExpression: public Expression {
		std::vector<ExpressionPtr> subexpressions;

		DeferExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::Defer, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.back();
		}

		std::vector<ExpressionPtr>::iterator begin() override {
			return subexpressions.begin();
		}

		std::vector<ExpressionPtr>::iterator end() override {
			return subexpressions.end();
		}

		void push_back(ExpressionPtr ref) override {
			subexpressions.push_back(ref);
		}

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) {
				if (s == oldNode) s = newNode;
			}
		}
	};

	struct ResolveVar: public Expression {
		std::string name;

		ResolveVar(ExpressionPtr par = nullptr): Expression(ExpressionType::ResolveVar, par) {
		}
		ResolveVar(const std::string& n, ExpressionPtr par = nullptr): Expression(ExpressionType::ResolveVar, par), name(n) {
		}

		void accept(AstVisitor& v) override;
	};

	struct DefineVar: public Expression {
		std::string name;
		std::vector<std::string> patternNames; // non-empty → tuple destructuring pattern
		ExpressionPtr defineExpression = nullptr;
		TypeDescriptor typeDescriptor;
		bool isLive = false;

		DefineVar(ExpressionPtr par = nullptr): Expression(ExpressionType::DefineVar, par) {
		}
		DefineVar(const std::string& n, ExpressionPtr defExpr = nullptr, ExpressionPtr par = nullptr):
			Expression(ExpressionType::DefineVar, par), name(n), defineExpression(defExpr) {
		}
		DefineVar(const std::string& n, ExpressionPtr defExpr, const TypeDescriptor& td, ExpressionPtr par = nullptr):
			Expression(ExpressionType::DefineVar, par), name(n), defineExpression(defExpr), typeDescriptor(td) {
		}
		DefineVar(std::vector<std::string> names, ExpressionPtr defExpr, const TypeDescriptor& td, ExpressionPtr par = nullptr):
			Expression(ExpressionType::DefineVar, par), patternNames(std::move(names)), defineExpression(defExpr), typeDescriptor(td) {
		}

		void accept(AstVisitor& v) override;
	};

	struct LiveRebind: public Expression {
		std::string targetName;
		ExpressionPtr guardExpr = nullptr;

		LiveRebind(const std::string& target, ExpressionPtr guard, ExpressionPtr par = nullptr)
			: Expression(ExpressionType::LiveRebind, par), targetName(target), guardExpr(guard) {
		}

		void accept(AstVisitor& v) override;
	};

	struct NamedArgumentExpression: public Expression {
		std::string name;
		ExpressionPtr expression;
		NamedArgumentExpression(const std::string& n, ExpressionPtr expr, ExpressionPtr par = nullptr)
			: Expression(ExpressionType::NamedArgument, par), name(n), expression(expr) {
		}

		void accept(AstVisitor& v) override;
	};

	struct ValueNode: public Expression {
		ValuePtr value;

		ValueNode(ValuePtr v, ExpressionType type = ExpressionType::Value): Expression(type, nullptr) {
			value = v;
		}
		ValueNode(ValueNode* v, ExpressionType type): Expression(type, nullptr) {
			value = v->value;
		}
		ValueNode(ValuePtr val, ExpressionPtr par): Expression(ExpressionType::Value, par), value(val) {
		}

		void accept(AstVisitor& v) override;
	};

	struct BlockExpression: public Expression {
		std::vector<ExpressionPtr> subexpressions;

		BlockExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::Block, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.back();
		}

		std::vector<ExpressionPtr>::iterator begin()override {
			return subexpressions.begin();
		}

		std::vector<ExpressionPtr>::iterator end()override {
			return subexpressions.end();
		}

		void push_back(ExpressionPtr ref)override {
			subexpressions.push_back(ref);
		}

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) {
				if (s == oldNode) s = newNode;
			}
		}
	};

	// --- Match (Rust-like) ---

	struct MatchArm {
		ExpressionPtr pattern = nullptr;  // null = default arm
		std::vector<ExpressionPtr> body;
	};

	struct MatchExpression: public Expression {
		ExpressionPtr target = nullptr;
		std::vector<MatchArm> arms;

		MatchExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::Match, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return arms.empty() ? nullptr : arms.back().body.back();
		}

		std::vector<ExpressionPtr>::iterator begin() override {
			return arms.empty() ? std::vector<ExpressionPtr>::iterator() : arms.back().body.begin();
		}

		std::vector<ExpressionPtr>::iterator end() override {
			return arms.empty() ? std::vector<ExpressionPtr>::iterator() : arms.back().body.end();
		}

		void push_back(ExpressionPtr ref) override {
			if (!arms.empty()) arms.back().body.push_back(ref);
		}

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& arm : arms) {
				for (auto& s : arm.body) {
					if (s == oldNode) s = newNode;
				}
			}
		}
	};

	// --- Tuple literal ---

	struct TupleLiteralExpression: public Expression {
		std::vector<ExpressionPtr> elements;

		TupleLiteralExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::TupleLiteral, par) {
		}

		void accept(AstVisitor& v) override;
	};

	// --- Tuple destructuring reassignment: (a, b) = expr ---

	struct DestructuringAssign: public Expression {
		std::vector<std::string> patternNames;
		ExpressionPtr valueExpression;

		DestructuringAssign(std::vector<std::string> names, ExpressionPtr valExpr, ExpressionPtr par = nullptr)
			: Expression(ExpressionType::DestructuringAssign, par), patternNames(std::move(names)), valueExpression(valExpr) {
		}

		void accept(AstVisitor& v) override;
	};

	// --- Phase 2: await / spawn ---

	// await <expr>  — suspend current suspending-func until expr (a Task) completes
	struct AwaitExpression: public Expression {
		ExpressionPtr taskExpr = nullptr;

		AwaitExpression(ExpressionPtr te, ExpressionPtr par = nullptr)
			: Expression(ExpressionType::Await, par), taskExpr(te) {
		}
		AwaitExpression(): Expression(ExpressionType::Await) {
		}

		void accept(AstVisitor& v) override;
	};

	// spawn { body }  — create and enqueue a new Task; evaluates to TaskRef
	struct SpawnExpression: public Expression {
		std::vector<ExpressionPtr> subexpressions;
		// Alternative: single expression form  spawn f(args)
		ExpressionPtr callExpr = nullptr;

		SpawnExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::Spawn, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.empty() ? nullptr : subexpressions.back();
		}
		std::vector<ExpressionPtr>::iterator begin() override {
			return subexpressions.begin();
		}
		std::vector<ExpressionPtr>::iterator end()   override {
			return subexpressions.end();
		}
		void push_back(ExpressionPtr ref) override {
			subexpressions.push_back(ref);
		}

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) {
				if (s == oldNode) s = newNode;
			}
		}
	};

	// --- Phase 3: sync / race / branch ---

	// sync { a; b; }  — await all children, return tuple of results
	struct SyncBlockExpression: public Expression {
		std::vector<ExpressionPtr> subexpressions;

		SyncBlockExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::SyncBlock, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.empty() ? nullptr : subexpressions.back();
		}
		std::vector<ExpressionPtr>::iterator begin() override {
			return subexpressions.begin();
		}
		std::vector<ExpressionPtr>::iterator end()   override {
			return subexpressions.end();
		}
		void push_back(ExpressionPtr ref) override {
			subexpressions.push_back(ref);
		}
		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) {
				if (s == oldNode) s = newNode;
			}
		}
	};

	// race { a; b; }  — first child to complete wins; others cancelled
	struct RaceBlockExpression: public Expression {
		std::vector<ExpressionPtr> subexpressions;

		RaceBlockExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::RaceBlock, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.empty() ? nullptr : subexpressions.back();
		}
		std::vector<ExpressionPtr>::iterator begin() override {
			return subexpressions.begin();
		}
		std::vector<ExpressionPtr>::iterator end()   override {
			return subexpressions.end();
		}
		void push_back(ExpressionPtr ref) override {
			subexpressions.push_back(ref);
		}
		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) {
				if (s == oldNode) s = newNode;
			}
		}
	};

	// branch { body }  — fire-and-forget: run in background; cancel on parent scope exit
	struct BranchBlockExpression: public Expression {
		std::vector<ExpressionPtr> subexpressions;

		BranchBlockExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::BranchBlock, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.empty() ? nullptr : subexpressions.back();
		}
		std::vector<ExpressionPtr>::iterator begin() override {
			return subexpressions.begin();
		}
		std::vector<ExpressionPtr>::iterator end()   override {
			return subexpressions.end();
		}
		void push_back(ExpressionPtr ref) override {
			subexpressions.push_back(ref);
		}
		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) {
				if (s == oldNode) s = newNode;
			}
		}
	};

	struct SafeBlockExpression: public Expression {
		std::vector<ExpressionPtr> subexpressions;
		bool producesValue = false;

		SafeBlockExpression(ExpressionPtr par = nullptr): Expression(ExpressionType::SafeBlock, par) {
		}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override {
			return subexpressions.back();
		}

		std::vector<ExpressionPtr>::iterator begin() override {
			return subexpressions.begin();
		}

		std::vector<ExpressionPtr>::iterator end() override {
			return subexpressions.end();
		}

		void push_back(ExpressionPtr ref) override {
			subexpressions.push_back(ref);
		}

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) {
				if (s == oldNode) s = newNode;
			}
		}
	};
}