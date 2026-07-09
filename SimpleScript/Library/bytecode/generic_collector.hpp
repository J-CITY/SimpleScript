#pragma once
#include "../expressions.hpp"
#include "../types.hpp"
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace IkigaiScript {

class IkigaiScriptInterpreter;

// ─── Static type inference ────────────────────────────────────────────────────
//
// Infers a type-name string (e.g. "Int", "String") from a single AST node
// without executing it.  Returns nullopt whenever inference is impossible
// (dynamic types, unknown calls, etc.).

struct StaticTypeInferencer {
	const ScopePtr& scope;

	std::optional<std::string> infer(ExpressionPtr node) const {
		if (!node) return std::nullopt;

		switch (node->type) {
			case ExpressionType::Value: {
				auto* vn = static_cast<ValueNode*>(node);
				if (!vn->value) return std::nullopt;
				return typeNameOf(vn->value->getType());
			}
			case ExpressionType::ResolveVar: {
				auto* rv = static_cast<ResolveVar*>(node);
				auto it = scope->variables.find(rv->name);
				if (it == scope->variables.end() || !it->second) return std::nullopt;
				auto& val = it->second;
				if (!val->typeDescriptor.isInit || val->typeDescriptor.isDynamic)
					return std::nullopt;
				return typeNameOf(val->getType());
			}
			default:
				return std::nullopt;
		}
	}

private:
	static std::optional<std::string> typeNameOf(Type t) {
		switch (t) {
			case Type::Int: return "Int";
			case Type::Float: return "Float";
			case Type::String: return "String";
			case Type::Bool: return "Bool";
			case Type::List: return "List";
			default: return std::nullopt;
		}
	}
};

// ─── Generic instantiation descriptor ────────────────────────────────────────

struct GenericInstantiation {
	FunctionRef templateFn;
	std::map<std::string, std::string> typeMap;

	bool operator==(const GenericInstantiation& o) const {
		return templateFn == o.templateFn && typeMap == o.typeMap;
	}
};

// ─── Collector ────────────────────────────────────────────────────────────────
//
// Walks a list of top-level AST statements and recursively visits all function
// bodies, collecting every call-site that targets a generic template function
// with statically-inferrable argument types.

class GenericInstantiationCollector {
public:
	GenericInstantiationCollector(const ScopePtr& scope) : scope_(scope) {}

	// Returns de-duplicated list of (template, typeMap) pairs found in stmts
	// plus all non-generic function bodies reachable from scope.
	std::vector<GenericInstantiation> collect(
		const std::vector<ExpressionPtr>& stmts)
	{
		for (auto& stmt : stmts) visitNode(stmt);

		// Also walk bodies of all regular (non-generic) functions in scope.
		for (auto& [name, fnc] : scope_->functions) {
			if (!fnc || !fnc->genericParams.empty()) continue;
			if (fnc->getBodyType() != FunctionBodyType::Subexpressions) continue;
			auto& body = std::get<std::vector<ExpressionPtr>>(fnc->body);
			for (auto& stmt : body) visitNode(stmt);
		}

		return results_;
	}

private:
	const ScopePtr& scope_;
	std::vector<GenericInstantiation> results_;

	// Deduplicate before recording.
	void record(GenericInstantiation inst) {
		for (auto& existing : results_) {
			if (existing == inst) return;
		}
		results_.push_back(std::move(inst));
	}

	void visitNode(ExpressionPtr node) {
		if (!node) return;

		if (node->type == ExpressionType::FunctionCall) {
			tryCollectCall(static_cast<FunctionExpression*>(node));
		}

		// Recurse into children.
		switch (node->type) {
			case ExpressionType::FunctionCall:
			case ExpressionType::FunctionDef: {
				auto* fe = static_cast<FunctionExpression*>(node);
				for (auto& sub : fe->subexpressions) visitNode(sub);
				break;
			}
			case ExpressionType::IfElse: {
				auto* ie = static_cast<IfElse*>(node);
				for (auto& branch : ie->states) {
					visitNode(branch.testExpression);
					for (auto& s : branch.subexpressions) visitNode(s);
				}
				break;
			}
			case ExpressionType::Loop: {
				auto* lp = static_cast<Loop*>(node);
				visitNode(lp->initExpression);
				visitNode(lp->testExpression);
				visitNode(lp->iterateExpression);
				for (auto& s : lp->subexpressions) visitNode(s);
				break;
			}
			case ExpressionType::ForEach: {
				auto* fe = static_cast<Foreach*>(node);
				visitNode(fe->listExpression);
				for (auto& s : fe->subexpressions) visitNode(s);
				break;
			}
			case ExpressionType::Return: {
				visitNode(static_cast<Return*>(node)->expression);
				break;
			}
			case ExpressionType::Yield: {
				visitNode(static_cast<Yield*>(node)->expression);
				break;
			}
			case ExpressionType::DefineVar: {
				visitNode(static_cast<DefineVar*>(node)->defineExpression);
				break;
			}
			case ExpressionType::Defer: {
				auto* de = static_cast<DeferExpression*>(node);
				for (auto& s : de->subexpressions) visitNode(s);
				break;
			}
			case ExpressionType::NamedArgument: {
				visitNode(static_cast<NamedArgumentExpression*>(node)->expression);
				break;
			}
			default:
				break;
		}
	}

	void tryCollectCall(FunctionExpression* fe) {
		if (!fe->function) return;
		FunctionRef fnc = fe->function->getFunction();
		if (!fnc || fnc->genericParams.empty()) return;

		// Try to infer a type for every generic type parameter from the arguments.
		StaticTypeInferencer inferencer{scope_};
		std::map<std::string, std::string> typeMap;
		bool complete = true;

		for (size_t i = 0; i < fnc->argNames.size(); ++i) {
			auto& argName = fnc->argNames[i];
			auto typeDescIt = fnc->types.find(argName);
			if (typeDescIt == fnc->types.end()) continue;
			auto& td = typeDescIt->second;

			if (td.type != Type::Class || !td.subtype) continue;
			auto genParam = std::get<std::string>(*td.subtype);
			if (std::find(fnc->genericParams.begin(), fnc->genericParams.end(), genParam)
				== fnc->genericParams.end()) continue;

			// The argument AST node (skipping any NamedArgumentExpression wrapper).
			ExpressionPtr argNode = (i < fe->subexpressions.size())
				? fe->subexpressions[i]
				: nullptr;
			if (argNode && argNode->type == ExpressionType::NamedArgument)
				argNode = static_cast<NamedArgumentExpression*>(argNode)->expression;

			auto inferred = inferencer.infer(argNode);
			if (!inferred) { complete = false; break; }
			typeMap[genParam] = *inferred;
		}

		if (complete && !typeMap.empty()) {
			record({fnc, std::move(typeMap)});
		}
	}
};

} // namespace IkigaiScript
