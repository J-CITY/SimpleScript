#pragma once

#include "ikigaiScript.h"
#include "exception.h"
#include "expressions.hpp"

namespace IkigaiScript {
	ValuePtr IkigaiScriptInterpreter::needsToReturn(ExpressionPtr exp, ScopePtr scope, Class* classs) {
		if (exp->type == ExpressionType::Return) {
			return getValue(exp, scope, classs);
		} else {
			auto result = consolidated(exp, scope, classs);
			if (result->type == ExpressionType::Return) {
				return std::static_pointer_cast<ValueNode>(result)->value;
			}
		}
		throw ExceptionExpectExpression("Expect return expression", ExpressionType::Return);
		return nullptr;
	}

	ValuePtr IkigaiScriptInterpreter::needsToReturn(const std::vector<ExpressionPtr>& subexpressions, ScopePtr scope, Class* classs) {
		for (auto&& sub : subexpressions) {
			if (auto returnVal = needsToReturn(sub, scope, classs)) {
				return returnVal;
			}
		}
		throw ExceptionExpectExpression("Expect return expression", ExpressionType::Return);
		return nullptr;
	}

	// walk the tree depth first and replace any function expressions 
	// with a value expression of their results
	ExpressionPtr IkigaiScriptInterpreter::consolidated(ExpressionPtr exp, ScopePtr scope, Class* classs) {
		switch (exp->type) {
		case ExpressionType::DefineVar: {
			auto def = std::static_pointer_cast<DefineVar>(exp);
			auto& varr = scope->variables[def->name];
			if (def->defineExpression) {
				auto val = getValue(def->defineExpression, scope, classs);
				varr = std::make_shared<Value>(val->value);
			} else {
				varr = std::make_shared<Value>();
			}
			return std::make_shared<ValueNode>(varr);
		}
		break;
		case ExpressionType::ResolveVar:
			return make_shared<ValueNode>(resolveVariable(std::static_pointer_cast<ResolveVar>(exp)->name, scope));
		case ExpressionType::MemberVariable: {
			auto expr = std::static_pointer_cast<MemberVariable>(exp);
			auto classToUse = expr->object ? getValue(expr->object, scope, classs)->getClass().get() : classs;
			return std::make_shared<ValueNode>(resolveVariable(expr->name, classToUse, scope));
		}
		case ExpressionType::MemberFunctionCall: {
			auto expr = std::static_pointer_cast<MemberFunctionCall>(exp);
			List args;
			for (auto&& sub : expr->subexpressions) {
				args.push_back(getValue(sub, scope, classs));
			}
			auto val = getValue(expr->object, scope, classs);
			if (val->getType() != Type::Class) {
				args.insert(args.begin(), val);
				return std::make_shared<ValueNode>(callFunction(resolveVariable(expr->functionName, scope)->getFunction(), scope, args, classs));
			}
			auto owningClass = val->getClass();
			return std::make_shared<ValueNode>(callFunction(resolveFunction(expr->functionName, owningClass.get(), scope), scope, args, owningClass));
		}
		case ExpressionType::Return:
			return std::make_shared<ValueNode>(getValue(std::static_pointer_cast<Return>(exp)->expression, scope, classs));
			break;
		case ExpressionType::FunctionCall:
		{
			List args;
			auto funcExpr = std::static_pointer_cast<FunctionExpression>(exp);
			
			for (auto&& sub : funcExpr->subexpressions) {
				args.push_back(getValue(sub, scope, classs));
			}

			if (funcExpr->function->getType() == Type::String) {
				funcExpr->function = resolveVariable(funcExpr->function->getString(), scope);
			}
			return std::make_shared<ValueNode>(callFunction(funcExpr->function->getFunction(), scope, args, classs));
		}
		break;
		case ExpressionType::Loop:
		{
			scope = newScope("loop", scope);
			auto loopexp = std::static_pointer_cast<Loop>(exp);
			if (loopexp->initExpression) {
				getValue(loopexp->initExpression, scope, classs);
			}
			ValuePtr returnVal = nullptr;
			while (returnVal == nullptr && getValue(loopexp->testExpression, scope, classs)->getBool()) {
				returnVal = needsToReturn(loopexp->subexpressions, scope, classs);
				if (returnVal == nullptr && loopexp->iterateExpression) {
					getValue(loopexp->iterateExpression, scope, classs);
				}
			}
			closeScope(scope);
			if (returnVal) {
				return std::make_shared<ValueNode>(returnVal, ExpressionType::Return);
			} else {
				return std::make_shared<ValueNode>(std::make_shared<Value>());
			}
		}
		break;
		case ExpressionType::ForEach:
		{
			scope = newScope("loop", scope);
			auto varr = resolveVariable(std::static_pointer_cast<Foreach>(exp)->iterateName, scope);
			auto list = getValue(std::static_pointer_cast<Foreach>(exp)->listExpression, scope, classs);
			auto& subs = std::static_pointer_cast<Foreach>(exp)->subexpressions;
			ValuePtr returnVal = nullptr;
			if (list->getType() == Type::Map) {
				for (auto&& in : *list->getDictionary().get()) {
					*varr = *in.second;
					returnVal = needsToReturn(subs, scope, classs);
				}
			} else if (list->getType() == Type::List) {
				for (auto&& in : list->getList()) {
					*varr = *in;
					returnVal = needsToReturn(subs, scope, classs);
				}
			} else if (list->getType() == Type::Array) {
				auto& arr = list->getArray();
				switch (arr.getType()) {
				case Type::Int: {
					auto vec = list->getStdVector<Int>();
					for (auto&& in : vec) {
						*varr = Value(in);
						returnVal = needsToReturn(subs, scope, classs);
					}
				}
				break;
				case Type::Float: {
					auto vec = list->getStdVector<Float>();
					for (auto&& in : vec) {
						*varr = Value(in);
						returnVal = needsToReturn(subs, scope, classs);
					}
				}
				break;
				//case Type::Vec3: {
				//	auto vec = list->getStdVector<vec3>();
				//	for (auto&& in : vec) {
				//		*varr = Value(in);
				//		returnVal = needsToReturn(subs, scope, classs);
				//	}
				//}
				//break;
				case Type::String: {
					auto vec = list->getStdVector<std::string>();
					for (auto&& in : vec) {
						*varr = Value(in);
						returnVal = needsToReturn(subs, scope, classs);
					}
				}
				break;
				default:
					break;
				}
			}
			closeScope(scope);
			if (returnVal) {
				return std::make_shared<ValueNode>(returnVal, ExpressionType::Return);
			} else {
				return std::make_shared<ValueNode>(std::make_shared<Value>(), ExpressionType::Value);
			}
		}
		break;
		case ExpressionType::IfElse:
		{
			ValuePtr returnVal = nullptr;
			for (auto& express : std::static_pointer_cast<IfElse>(exp)->states) {
				if (!express.testExpression || getValue(express.testExpression, scope, classs)->getBool()) {
					scope = newScope("ifelse", scope);
					returnVal = needsToReturn(express.subexpressions, scope, classs);
					closeScope(scope);
					break;
				}
			}
			if (returnVal) {
				return make_shared<ValueNode>(returnVal, ExpressionType::Return);
			} else {
				return std::make_shared<ValueNode>(std::make_shared<Value>(), ExpressionType::Value);
			}
		}
		break;
		default:
			break;
		}
		return std::make_shared<ValueNode>(std::static_pointer_cast<ValueNode>(exp), ExpressionType::Value);
	}

	// evaluate an expression from tokens
	ValuePtr IkigaiScriptInterpreter::getValue(const std::vector<std::string_view>& strings, ScopePtr scope, Class* classs) {
		return getValue(getExpression(strings, scope, classs), scope, classs);
	}

	// evaluate an expression from ExpressionPtr
	ValuePtr IkigaiScriptInterpreter::getValue(ExpressionPtr exp, ScopePtr scope, Class* classs) {
		// copy the expression so that we don't lose it when we consolidate
		return std::static_pointer_cast<ValueNode>(consolidated(exp, scope, classs))->value;
	}

	// since the 'else' block in  an if/elfe is technically in a different scope
	// ifelse espressions are not closed immediately and instead left dangling
	// until the next expression is anything other than an 'else' or the else is unconditional
	//void IkigaiScriptInterpreter::closeDanglingIfExpression() {
	//	if (currentExpression && currentExpression->type == ExpressionType::IfElse) {
	//		if (currentExpression->parent) {
	//			currentExpression = currentExpression->parent;
	//		} else {
	//			getValue(currentExpression, parseScope, nullptr);
	//			currentExpression = nullptr;
	//		}
	//	}
	//}

	bool IkigaiScriptInterpreter::closeCurrentExpression() {
		if (currentExpression) {
			if (currentExpression->type != ExpressionType::IfElse) {
				if (currentExpression->parent) {
					currentExpression = currentExpression->parent;
				} else {
					if (currentExpression->type != ExpressionType::FunctionDef) {
						getValue(currentExpression, parseScope, nullptr);
					}
					currentExpression = nullptr;
				}
				return true;
			}
		}
		return false;
	}

	ScopePtr IkigaiScriptInterpreter::newScope(const std::string& name, ScopePtr scope) {
		// if the scope exists we just use it as is
		auto iter = scope->scopes.find(name);
		if (iter != scope->scopes.end()) {
			return iter->second;
		}
		else {
			return scope->insertScope(make_shared<Scope>(name, scope));
		}
	}

	ScopePtr IkigaiScriptInterpreter::insertScope(ScopePtr existing, ScopePtr parent) {
		existing->parent = parent;
		return parent->insertScope(existing);
	}

	void IkigaiScriptInterpreter::closeScope(ScopePtr& scope) {
		if (scope->parent) {
			if (scope->isClassScope) {
				scope = scope->parent;
			}
			else {
				auto name = scope->name;
				scope->functions.clear();
				scope->variables.clear();
				scope->scopes.clear();
				scope = scope->parent;
				scope->scopes.erase(name);
			}
		}
	}

	ScopePtr IkigaiScriptInterpreter::newClassScope(const std::string& name, ScopePtr scope) {
		auto ref = newScope(name, scope);
		ref->isClassScope = true;
		return ref;
	}
}
