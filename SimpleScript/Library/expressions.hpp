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
	struct DefineVar;
	struct ResolveVar;
	struct Foreach;
	class IkigaiScriptInterpreter;

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

		

		~MemberFunctionCall() override {
			clear();
		}
	};

	struct Return : public Expression {
		ExpressionPtr expression = nullptr;

		Return(ExpressionPtr e, ExpressionPtr par = nullptr) : Expression(ExpressionType::Return, par), expression(e) {}
		Return() : Expression(ExpressionType::Return) {}

		
	};

	struct Yeld : public Expression {
		ExpressionPtr expression = nullptr;

		Yeld(ExpressionPtr e, ExpressionPtr par = nullptr) : Expression(ExpressionType::Yeld, par), expression(e) {}
		Yeld() : Expression(ExpressionType::Yeld) {}

		
	};

	struct Continue : public Expression {

		Continue(ExpressionPtr par) : Expression(ExpressionType::Continue, par) {}
		Continue() : Expression(ExpressionType::Continue) {}

		
	};

	struct Break : public Expression {

		Break(ExpressionPtr par) : Expression(ExpressionType::Break, par) {}
		Break() : Expression(ExpressionType::Break) {}

		
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

	struct ResolveVar : public Expression {
		std::string name;

		ResolveVar(ExpressionPtr par = nullptr) : Expression(ExpressionType::ResolveVar, par) {}
		ResolveVar(const std::string& n, ExpressionPtr par = nullptr) : Expression(ExpressionType::ResolveVar, par), name(n) {}

		
	};

	struct DefineVar : public Expression {
		std::string name;
		ExpressionPtr defineExpression = nullptr;
		TypeDescriptor typeDescriptor;

		DefineVar(ExpressionPtr par = nullptr) : Expression(ExpressionType::DefineVar, par) {}
		DefineVar(const std::string& n, ExpressionPtr defExpr = nullptr, ExpressionPtr par = nullptr) :
			Expression(ExpressionType::DefineVar, par) , name(n), defineExpression(defExpr) {}
		DefineVar(const std::string& n, ExpressionPtr defExpr, const TypeDescriptor& td, ExpressionPtr par = nullptr) :
			Expression(ExpressionType::DefineVar, par) , name(n), defineExpression(defExpr), typeDescriptor(td) {}
	};

	struct NamedArgumentExpression : public Expression {
		std::string name;
		ExpressionPtr expression;
		NamedArgumentExpression(const std::string& n, ExpressionPtr expr, ExpressionPtr par = nullptr) 
			: Expression(ExpressionType::NamedArgument, par), name(n), expression(expr) {}
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
}