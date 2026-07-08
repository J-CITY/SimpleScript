#pragma once
#include <mutex>
#include <unordered_map>

#include "types.hpp"

namespace IkigaiScript {
	class IkigaiScriptInterpreter;

	struct Scope {
        std::string name;
        ScopePtr parent;
        IkigaiScriptInterpreter* host;

    	std::mutex varInsert;
        std::mutex scopeInsert;
        std::mutex fncInsert;

        std::unordered_map<std::string, ValuePtr> variables;
        std::unordered_map<std::string, ScopePtr> scopes;
        std::unordered_map<std::string, FunctionRef> functions;
        std::unordered_map<std::string, std::list<FunctionRef>> operators;
        // Multiple constructor overloads keyed by class name (like operators)
        std::unordered_map<std::string, std::list<FunctionRef>> constructors;
        bool isClassScope = false;
        bool isGenericScope = false;
        bool isTransactionScope = false;
        
        std::vector<Metadata> scopeMetadata;
        std::unordered_map<std::string, std::vector<Metadata>> membersMetadata;

        std::vector<std::string> baseClasses;
        std::vector<ExpressionPtr> deferred;

        ValuePtr& insertVar(const std::string& n, ValuePtr val);
        ScopePtr insertScope(ScopePtr val);
        Scope(IkigaiScriptInterpreter* interpereter);
        Scope(const std::string& name_, IkigaiScriptInterpreter* interpereter);
        Scope(const std::string& name_, ScopePtr scope);
        Scope(const Scope& o);
        Scope(const std::string& name_, const std::unordered_map<std::string, ValuePtr>& variables_);
    };
}
