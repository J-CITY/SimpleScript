#include "types.hpp"

#include "ikigaiScript.h"
#include "scope.hpp"
#include "value.hpp"

using namespace IkigaiScript;


Class::Class(const Class& o): name(o.name), functionScope(o.functionScope), baseClasses(o.baseClasses) {
	for (auto&& v : o.variables) {
		variables[v.first] = std::make_shared<Value>(*v.second);
	}
}

Class::Class(const ScopePtr& o): name(o->name), functionScope(o), baseClasses(o->baseClasses) {
	for (auto&& v : o->variables) {
		variables[v.first] = std::make_shared<Value>(*v.second);
	}
}

Class::~Class() {
	auto iter = functionScope->functions.find("~"s + name);
	if (iter != functionScope->functions.end()) {
		functionScope->host->callFunction(iter->second, functionScope, {}, this);
	}
}

Function::Function(const std::string& name)
	: Function(name, [](List)
{
	return std::make_shared<Value>();
}) {
}
