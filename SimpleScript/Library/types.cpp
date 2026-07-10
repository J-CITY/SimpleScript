#include "types.hpp"

#include "ikigaiScript.h"
#include "scope.hpp"
#include "value.hpp"

using namespace IkigaiScript;


Class::Class(const Class& o): name(o.name), functionScope(o.functionScope), baseClasses(o.baseClasses) {
	auto* host = functionScope ? functionScope->host : nullptr;
	for (auto&& v : o.variables) {
		variables[v.first] = host ? host->copyValue(*v.second) : std::make_shared<Value>(*v.second);
	}
}

Class::Class(const ScopePtr& o): name(o->name), functionScope(o), baseClasses(o->baseClasses) {
	auto* host = o->host;
	for (auto&& v : o->variables) {
		variables[v.first] = host ? host->copyValue(*v.second) : std::make_shared<Value>(*v.second);
	}
}

Class::~Class() {
	auto iter = functionScope->functions.find("~"s + name);
	if (iter != functionScope->functions.end()) {
		functionScope->host->callFunction(iter->second, functionScope, {}, this);
	}
}

Function::Function(const std::string& name)
	: name(name), opPrecedence(getPrecedence(name)), body(std::vector<ExpressionPtr>{}) {
}
