#include "scope.hpp"

#include "value.hpp"

namespace IkigaiScript {
	ValuePtr& Scope::insertVar(const std::string& n, ValuePtr val) {
		auto l = std::unique_lock(varInsert);
		auto& ref = variables[n];
		ref = val;
		return ref;
	}

	ScopePtr Scope::insertScope(ScopePtr val) {
		auto l = std::unique_lock(scopeInsert);
		scopes[val->name] = val;
		return val;
	}

	Scope::Scope(IkigaiScriptInterpreter* interpereter): name("global"), parent(nullptr), host(interpereter) {
	}
	Scope::Scope(const std::string& name_, IkigaiScriptInterpreter* interpereter): name(name_), parent(nullptr), host(interpereter) {
	}
	Scope::Scope(const std::string& name_, ScopePtr scope): name(name_), parent(scope), host(scope->host) {
	}
	Scope::Scope(const Scope& o): name(o.name), parent(o.parent), scopes(o.scopes), functions(o.functions), host(o.host) {
		// copy vars by value when cloning a scope
		for (auto&& v : o.variables) {
			variables[v.first] = std::make_shared<Value>(*v.second);
		}
	}
	Scope::Scope(const std::string& name_, const std::unordered_map<std::string, ValuePtr>& variables_): name(name_), variables(variables_) {
	}
}
