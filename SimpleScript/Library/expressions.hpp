#include "arena.hpp"
#pragma once
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



		//Expression(ValuePtr val)
		//	: type(ExpressionType::FunctionCall), expression(FunctionExpression(val)), parent(nullptr) {}
		//
		//Expression(ExpressionPtr obj, const string& name)
		//	: type(ExpressionType::MemberVariable), expression(MemberVariable(obj, name)), parent(nullptr) {}
		//Expression(ExpressionPtr obj, const string& name, const vector<ExpressionPtr> subs)
		//	: type(ExpressionType::MemberFunctionCall), expression(MemberFunctionCall(obj, name, subs)), parent(nullptr) {}
		//Expression(FunctionRef val, ExpressionPtr par)
		//	: type(ExpressionType::FunctionDef), expression(FunctionExpression(val)), parent(par) {}
		//Expression(ValuePtr val, ExpressionPtr par)
		//	: type(ExpressionType::Value), expression(val), parent(par) {}
		//Expression(Foreach val, ExpressionPtr par = nullptr)
		//	: type(ExpressionType::ForEach), expression(val), parent(par) {}
		//Expression(Loop val, ExpressionPtr par = nullptr)
		//	: type(ExpressionType::Loop), expression(val), parent(par) {}
		//Expression(IfElse val, ExpressionPtr par = nullptr)
		//	: type(ExpressionType::IfElse), expression(val), parent(par) {}
		//Expression(Return val, ExpressionPtr par = nullptr)
		//	: type(ExpressionType::Return), expression(val), parent(par) {}
		//Expression(ResolveVar val, ExpressionPtr par = nullptr)
		//	: type(ExpressionType::ResolveVar), expression(val), parent(par) {}
		//Expression(DefineVar val, ExpressionPtr par = nullptr)
		//	: type(ExpressionType::DefineVar), expression(val), parent(par) {}

		Expression(ExpressionType ty, ExpressionPtr par = nullptr) : type(ty), parent(par) {}

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

		virtual void push_back(ExpressionPtr ref) {}

		virtual void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) {}
		

		//void push_back(const If& ref) {
		//	switch (type) {
		//	case ExpressionType::IfElse:
		//		get<IfElse>(expression).push_back(ref);
		//		break;
		//	default:
		//		break;
		//	}
		//}
	};


	struct FunctionExpression: public Expression {
		ValuePtr function;
		std::vector<ExpressionPtr> subexpressions;

		FunctionExpression(FunctionRef fnc, ExpressionPtr par) : Expression(ExpressionType::FunctionDef, par), function(new Value(fnc)) {}
		FunctionExpression(ValuePtr fncvalue) :Expression(ExpressionType::FunctionCall), function(fncvalue) {}

		void accept(AstVisitor& v) override;


		//FunctionExpression() : Expression() {}

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
				for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
				break;
			case ExpressionType::FunctionDef: {
				auto& body = get<std::vector<ExpressionPtr>>(function->getFunction()->body);
				for (auto& s : body) { if (s == oldNode) s = newNode; }
				break;
			}
			default:
				break;
			}
		}
	};

	struct MemberVariable : public Expression {
	    ExpressionPtr object = nullptr;
		std::string name;
	
		MemberVariable(ExpressionPtr ob, const std::string& name_) : Expression(ExpressionType::MemberVariable), object(ob), name(name_) {}
		MemberVariable() : Expression(ExpressionType::MemberVariable) {}

		void accept(AstVisitor& v) override;
	};

	struct MemberFunctionCall : public Expression {
		ExpressionPtr object = nullptr;
		std::string functionName;
		std::vector<ExpressionPtr> subexpressions;

				MemberFunctionCall(ExpressionPtr ob, const std::string& fncvalue, const std::vector<ExpressionPtr>& sub) : Expression(ExpressionType::MemberFunctionCall)
			, object(ob), functionName(fncvalue), subexpressions(sub) {}
		MemberFunctionCall() : Expression(ExpressionType::MemberFunctionCall) {}

		void clear() {
			subexpressions.clear();
		}

		void accept(AstVisitor& v) override;

		~MemberFunctionCall() override {
			clear();
		}
	};

	struct Return : public Expression {
		ExpressionPtr expression = nullptr;

		Return(ExpressionPtr e, ExpressionPtr par = nullptr) : Expression(ExpressionType::Return, par), expression(e) {}
		Return() : Expression(ExpressionType::Return) {}

		void accept(AstVisitor& v) override;
	};

	struct Yield : public Expression {
		ExpressionPtr expression = nullptr;

		Yield(ExpressionPtr e, ExpressionPtr par = nullptr) : Expression(ExpressionType::Yield, par), expression(e) {}
		Yield() : Expression(ExpressionType::Yield) {}

		void accept(AstVisitor& v) override;
	};

	struct Continue : public Expression {

		Continue(ExpressionPtr par) : Expression(ExpressionType::Continue, par) {}
		Continue() : Expression(ExpressionType::Continue) {}

		void accept(AstVisitor& v) override;
	};

	struct Break : public Expression {

		Break(ExpressionPtr par) : Expression(ExpressionType::Break, par) {}
		Break() : Expression(ExpressionType::Break) {}

		void accept(AstVisitor& v) override;
	};

	struct If {
		ExpressionPtr testExpression = nullptr;
		std::vector<ExpressionPtr> subexpressions;

		If() {}
	};

	struct IfElse : public Expression {
		IfElse(): Expression(ExpressionType::IfElse) {}
		IfElse(std::vector<If>& val, ExpressionPtr par = nullptr) :
			Expression(ExpressionType::IfElse, par), states(val) {}

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
				for (auto& s : st.subexpressions) { if (s == oldNode) s = newNode; }
			}
		}
	};

	struct Loop : public Expression {
		ExpressionPtr initExpression = nullptr;
		ExpressionPtr testExpression = nullptr;
		ExpressionPtr iterateExpression = nullptr;
		std::vector<ExpressionPtr> subexpressions;


		Loop(ExpressionPtr par) : Expression(ExpressionType::Loop, par) {}
		Loop(): Expression(ExpressionType::Loop) {}

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
			for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
		}
	};

	struct Foreach : public Expression {
		ExpressionPtr listExpression = nullptr;
		std::vector<std::string> iterateNames;
		std::vector<ExpressionPtr> subexpressions;

				Foreach(ExpressionPtr par = nullptr): Expression(ExpressionType::ForEach, par) {}

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

	struct DeferExpression : public Expression {
		std::vector<ExpressionPtr> subexpressions;

		DeferExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::Defer, par) {}

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
			for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
		}
	};

	struct ResolveVar : public Expression {
		std::string name;

		ResolveVar(ExpressionPtr par = nullptr) : Expression(ExpressionType::ResolveVar, par) {}
		ResolveVar(const std::string& n, ExpressionPtr par = nullptr) : Expression(ExpressionType::ResolveVar, par), name(n) {}

		void accept(AstVisitor& v) override;
	};

	struct DefineVar : public Expression {
		std::string name;
		std::vector<std::string> patternNames; // non-empty → tuple destructuring pattern
		ExpressionPtr defineExpression = nullptr;
		TypeDescriptor typeDescriptor;
		bool isLive = false;

		DefineVar(ExpressionPtr par = nullptr) : Expression(ExpressionType::DefineVar, par) {}
		DefineVar(const std::string& n, ExpressionPtr defExpr = nullptr, ExpressionPtr par = nullptr) :
			Expression(ExpressionType::DefineVar, par) , name(n), defineExpression(defExpr) {}
		DefineVar(const std::string& n, ExpressionPtr defExpr, const TypeDescriptor& td, ExpressionPtr par = nullptr) :
			Expression(ExpressionType::DefineVar, par) , name(n), defineExpression(defExpr), typeDescriptor(td) {}
		DefineVar(std::vector<std::string> names, ExpressionPtr defExpr, const TypeDescriptor& td, ExpressionPtr par = nullptr) :
			Expression(ExpressionType::DefineVar, par), patternNames(std::move(names)), defineExpression(defExpr), typeDescriptor(td) {}

		void accept(AstVisitor& v) override;
	};

	struct LiveRebind : public Expression {
		std::string targetName;
		ExpressionPtr guardExpr = nullptr;

		LiveRebind(const std::string& target, ExpressionPtr guard, ExpressionPtr par = nullptr)
			: Expression(ExpressionType::LiveRebind, par), targetName(target), guardExpr(guard) {}

		void accept(AstVisitor& v) override;
	};

	struct NamedArgumentExpression : public Expression {
		std::string name;
		ExpressionPtr expression;
		NamedArgumentExpression(const std::string& n, ExpressionPtr expr, ExpressionPtr par = nullptr) 
			: Expression(ExpressionType::NamedArgument, par), name(n), expression(expr) {}

		void accept(AstVisitor& v) override;
	};

	struct ValueNode : public Expression {
		ValuePtr value;

		ValueNode(ValuePtr v, ExpressionType type= ExpressionType::Value) : Expression(type, nullptr) {
			value = v;
		}
				ValueNode(ValueNode* v, ExpressionType type) : Expression(type, nullptr) {
			value = v->value;
		}
				ValueNode(ValuePtr val, ExpressionPtr par): Expression(ExpressionType::Value, par), value(val) {}

		void accept(AstVisitor& v) override;
	};

	//OLD


	//using IfElse = vector<If>;
	//using ExpressionVariant =
	//	std::variant<
	//	ValuePtr,
	//	ResolveVar,
	//	DefineVar,
	//	FunctionExpression,
	//	MemberFunctionCall,
	//	MemberVariable,
	//	Return,
	//	Loop,
	//	Foreach,
	//	IfElse
	//>;


	

	//struct FunctionExpression {
	//	ValuePtr function;
	//	vector<ExpressionPtr> subexpressions;
	//
	//	//	}
	//	FunctionExpression(FunctionRef fnc) : function(new Value(fnc)) {}
	//	FunctionExpression(ValuePtr fncvalue) : function(fncvalue) {}
	//	FunctionExpression() {}
	//
	//	void clear() {
	//		subexpressions.clear();
	//	}
	//
	//	~FunctionExpression() {
	//		clear();
	//	}
	//};

    //struct MemberVariable {
    //    ExpressionPtr object = nullptr;
	//	string name;
	//
	//	//	MemberVariable(ExpressionPtr ob, const string& name_) : object(ob), name(name_) {}
	//	MemberVariable() {}
	//};

    //struct MemberFunctionCall {
    //    ExpressionPtr object = nullptr;
	//	string functionName;
	//	vector<ExpressionPtr> subexpressions;
	//
	//	//	}
	//	MemberFunctionCall(ExpressionPtr ob, const string& fncvalue, const vector<ExpressionPtr>& sub) 
    //        : object(ob), functionName(fncvalue), subexpressions(sub) {}
	//	MemberFunctionCall() {}
	//
	//	void clear() {
	//		subexpressions.clear();
	//	}
	//
	//	~MemberFunctionCall() {
	//		clear();
	//	}
	//};

	//struct Return {
	//	ExpressionPtr expression = nullptr;
	//
	//	//	Return(ExpressionPtr e) : expression(e) {}
	//	Return() {}
	//};
	//
	//struct If {
	//	ExpressionPtr testExpression = nullptr;
	//	vector<ExpressionPtr> subexpressions;
	//
	//	//	}
	//	If() {}
	//};

	//struct Loop {
	//	ExpressionPtr initExpression = nullptr;
	//	ExpressionPtr testExpression = nullptr;
	//	ExpressionPtr iterateExpression = nullptr;
	//	vector<ExpressionPtr> subexpressions;
	//
	//	//	}
	//	Loop() {}
	//};

	//struct Foreach {
    //    ExpressionPtr listExpression = nullptr;
	//	string iterateName;
	//	vector<ExpressionPtr> subexpressions;
	//
	//	//	}
	//	Foreach() {}
	//};

    //struct ResolveVar {
    //    string name;
	//
    //        //    ResolveVar() {}
    //    ResolveVar(const string& n) : name(n) {}
    //};
	//
    //struct DefineVar {
    //    string name;
    //    ExpressionPtr defineExpression = nullptr;
	//
    //        //    DefineVar() {}
    //    DefineVar(const string& n) : name(n) {}
    //    DefineVar(const string& n, ExpressionPtr defExpr) : name(n), defineExpression(defExpr) {}
    //};

	
	//class IkigaiScriptInterpreter;
	//struct Expression {
	//	ExpressionVariant expression;
	//	ExpressionType type;
	//	ExpressionPtr parent = nullptr;
	//
	//	Expression(ValuePtr val)
	//		: type(ExpressionType::FunctionCall), expression(FunctionExpression(val)), parent(nullptr) {}
	//
	//	Expression(ExpressionPtr obj, const string& name)
	//		: type(ExpressionType::MemberVariable), expression(MemberVariable(obj, name)), parent(nullptr) {}
	//	Expression(ExpressionPtr obj, const string& name, const vector<ExpressionPtr> subs)
	//		: type(ExpressionType::MemberFunctionCall), expression(MemberFunctionCall(obj, name, subs)), parent(nullptr) {}
	//	Expression(FunctionRef val, ExpressionPtr par)
	//		: type(ExpressionType::FunctionDef), expression(FunctionExpression(val)), parent(par) {}
	//	Expression(ValuePtr val, ExpressionPtr par)
	//		: type(ExpressionType::Value), expression(val), parent(par) {}
	//	Expression(Foreach val, ExpressionPtr par = nullptr)
	//		: type(ExpressionType::ForEach), expression(val), parent(par) {}
	//	Expression(Loop val, ExpressionPtr par = nullptr)
	//		: type(ExpressionType::Loop), expression(val), parent(par) {}
	//	Expression(IfElse val, ExpressionPtr par = nullptr)
	//		: type(ExpressionType::IfElse), expression(val), parent(par) {}
	//	Expression(Return val, ExpressionPtr par = nullptr)
	//		: type(ExpressionType::Return), expression(val), parent(par) {}
	//	Expression(ResolveVar val, ExpressionPtr par = nullptr)
	//		: type(ExpressionType::ResolveVar), expression(val), parent(par) {}
	//	Expression(DefineVar val, ExpressionPtr par = nullptr)
	//		: type(ExpressionType::DefineVar), expression(val), parent(par) {}
	//
	//	Expression(ExpressionVariant val, ExpressionType ty)
	//		: type(ty), expression(val) {}
	//
	//	ExpressionPtr back() {
	//		switch (type) {
	//		case ExpressionType::FunctionDef:
	//			return std::get<vector<ExpressionPtr>>(std::get<FunctionExpression>(expression).function->getFunction()->body).back();
	//			break;
	//		case ExpressionType::FunctionCall:
	//			return std::get<FunctionExpression>(expression).subexpressions.back();
	//			break;
	//		case ExpressionType::Loop:
	//			return std::get<Loop>(expression).subexpressions.back();
	//			break;
	//		case ExpressionType::ForEach:
	//			return std::get<Foreach>(expression).subexpressions.back();
	//			break;
	//		case ExpressionType::IfElse:
	//			return std::get<IfElse>(expression).back().subexpressions.back();
	//			break;
	//		default:
	//			break;
	//		}
	//		return nullptr;
	//	}
	//
	//	auto begin() {
	//		switch (type) {
	//		case ExpressionType::FunctionCall:
	//			return get<FunctionExpression>(expression).subexpressions.begin();
	//			break;
	//		case ExpressionType::FunctionDef:
	//			return std::get<vector<ExpressionPtr>>(get<FunctionExpression>(expression).function->getFunction()->body).begin();
	//			break;
	//		case ExpressionType::Loop:
	//			return get<Loop>(expression).subexpressions.begin();
	//			break;
	//		case ExpressionType::ForEach:
	//			return get<Foreach>(expression).subexpressions.begin();
	//			break;
	//		case ExpressionType::IfElse:
	//			return get<IfElse>(expression).back().subexpressions.begin();
	//			break;
	//		default:
	//			return vector<ExpressionPtr>::iterator();
	//			break;
	//		}
	//	}
	//
	//	auto end() {
	//		switch (type) {
	//		case ExpressionType::FunctionCall:
	//			return get<FunctionExpression>(expression).subexpressions.end();
	//			break;
	//		case ExpressionType::FunctionDef:
	//			return get<vector<ExpressionPtr>>(get<FunctionExpression>(expression).function->getFunction()->body).end();
	//			break;
	//		case ExpressionType::Loop:
	//			return get<Loop>(expression).subexpressions.end();
	//			break;
	//		case ExpressionType::ForEach:
	//			return get<Foreach>(expression).subexpressions.end();
	//			break;
	//		case ExpressionType::IfElse:
	//			return get<IfElse>(expression).back().subexpressions.end();
	//			break;
	//		default:
	//			return vector<ExpressionPtr>::iterator();
	//			break;
	//		}
	//	}
	//
	//	void push_back(ExpressionPtr ref) {
	//		switch (type) {
	//		case ExpressionType::FunctionCall:
	//			get<FunctionExpression>(expression).subexpressions.push_back(ref);
	//			break;
	//		case ExpressionType::FunctionDef:
	//			get<vector<ExpressionPtr>>(get<FunctionExpression>(expression).function->getFunction()->body).push_back(ref);
	//			break;
	//		case ExpressionType::Loop:
	//			get<Loop>(expression).subexpressions.push_back(ref);
	//			break;
	//		case ExpressionType::ForEach:
	//			get<Foreach>(expression).subexpressions.push_back(ref);
	//			break;
	//		case ExpressionType::IfElse:
	//			get<IfElse>(expression).back().subexpressions.push_back(ref);
	//			break;
	//		default:
	//			break;
	//		}
	//	}
	//
	//	void push_back(const If& ref) {
	//		switch (type) {
	//		case ExpressionType::IfElse:
	//			get<IfElse>(expression).push_back(ref);
	//			break;
	//		default:
	//			break;
	//		}
	//	}
	//	}
	//};

	struct BlockExpression : public Expression {
		std::vector<ExpressionPtr> subexpressions;

		BlockExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::Block, par) {}

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
			for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
		}
	};

	// --- Match (Rust-like) ---

	struct MatchArm {
		ExpressionPtr pattern = nullptr;  // null = default arm
		std::vector<ExpressionPtr> body;
	};

	struct MatchExpression : public Expression {
		ExpressionPtr target = nullptr;
		std::vector<MatchArm> arms;

		MatchExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::Match, par) {}

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
				for (auto& s : arm.body) { if (s == oldNode) s = newNode; }
			}
		}
	};

	// --- Tuple literal ---

	struct TupleLiteralExpression : public Expression {
		std::vector<ExpressionPtr> elements;

		TupleLiteralExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::TupleLiteral, par) {}

		void accept(AstVisitor& v) override;
	};

	// --- Tuple destructuring reassignment: (a, b) = expr ---

	struct DestructuringAssign : public Expression {
		std::vector<std::string> patternNames;
		ExpressionPtr valueExpression;

		DestructuringAssign(std::vector<std::string> names, ExpressionPtr valExpr, ExpressionPtr par = nullptr)
			: Expression(ExpressionType::DestructuringAssign, par), patternNames(std::move(names)), valueExpression(valExpr) {}

		void accept(AstVisitor& v) override;
	};

	// --- Phase 2: await / spawn ---

	// await <expr>  — suspend current suspending-func until expr (a Task) completes
	struct AwaitExpression : public Expression {
		ExpressionPtr taskExpr = nullptr;

		AwaitExpression(ExpressionPtr te, ExpressionPtr par = nullptr)
			: Expression(ExpressionType::Await, par), taskExpr(te) {}
		AwaitExpression() : Expression(ExpressionType::Await) {}

		void accept(AstVisitor& v) override;
	};

	// spawn { body }  — create and enqueue a new Task; evaluates to TaskRef
	struct SpawnExpression : public Expression {
		std::vector<ExpressionPtr> subexpressions;
		// Alternative: single expression form  spawn f(args)
		ExpressionPtr callExpr = nullptr;

		SpawnExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::Spawn, par) {}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override { return subexpressions.empty() ? nullptr : subexpressions.back(); }
		std::vector<ExpressionPtr>::iterator begin() override { return subexpressions.begin(); }
		std::vector<ExpressionPtr>::iterator end()   override { return subexpressions.end(); }
		void push_back(ExpressionPtr ref) override { subexpressions.push_back(ref); }

		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
		}
	};

	// --- Phase 3: sync / race / branch ---

	// sync { a; b; }  — await all children, return tuple of results
	struct SyncBlockExpression : public Expression {
		std::vector<ExpressionPtr> subexpressions;

		SyncBlockExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::SyncBlock, par) {}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override { return subexpressions.empty() ? nullptr : subexpressions.back(); }
		std::vector<ExpressionPtr>::iterator begin() override { return subexpressions.begin(); }
		std::vector<ExpressionPtr>::iterator end()   override { return subexpressions.end(); }
		void push_back(ExpressionPtr ref) override { subexpressions.push_back(ref); }
		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
		}
	};

	// race { a; b; }  — first child to complete wins; others cancelled
	struct RaceBlockExpression : public Expression {
		std::vector<ExpressionPtr> subexpressions;

		RaceBlockExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::RaceBlock, par) {}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override { return subexpressions.empty() ? nullptr : subexpressions.back(); }
		std::vector<ExpressionPtr>::iterator begin() override { return subexpressions.begin(); }
		std::vector<ExpressionPtr>::iterator end()   override { return subexpressions.end(); }
		void push_back(ExpressionPtr ref) override { subexpressions.push_back(ref); }
		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
		}
	};

	// branch { body }  — fire-and-forget: run in background; cancel on parent scope exit
	struct BranchBlockExpression : public Expression {
		std::vector<ExpressionPtr> subexpressions;

		BranchBlockExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::BranchBlock, par) {}

		void accept(AstVisitor& v) override;

		ExpressionPtr back() override { return subexpressions.empty() ? nullptr : subexpressions.back(); }
		std::vector<ExpressionPtr>::iterator begin() override { return subexpressions.begin(); }
		std::vector<ExpressionPtr>::iterator end()   override { return subexpressions.end(); }
		void push_back(ExpressionPtr ref) override { subexpressions.push_back(ref); }
		void replaceChild(ExpressionPtr oldNode, ExpressionPtr newNode) override {
			for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
		}
	};

	struct SafeBlockExpression : public Expression {
		std::vector<ExpressionPtr> subexpressions;
		bool producesValue = false;

		SafeBlockExpression(ExpressionPtr par = nullptr) : Expression(ExpressionType::SafeBlock, par) {}

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
			for (auto& s : subexpressions) { if (s == oldNode) s = newNode; }
		}
	};
}