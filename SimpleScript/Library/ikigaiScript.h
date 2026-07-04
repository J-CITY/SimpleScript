#pragma once


#include <chrono>
#include <string>

#include "stringUtils.hpp"
#include "types.hpp"
#include "scope.hpp"
#include "modules.hpp"

#define TEST_MOD

#include "arena.hpp"

namespace IkigaiScript {
	using namespace std::string_literals;

	class Parser;

	struct BlockResult {
		LoopInterupt interrupt = LoopInterupt::None;
		ValuePtr explicitReturn = nullptr;
		ValuePtr lastValue = nullptr;
	};

	class IkigaiScriptInterpreter {
	public:
		std::atomic_bool data_is_ready_{};

		inline static uint64_t currentLine = 0;
	private:
		friend Expression;
		friend class Parser;
		std::vector<Module> modules;
		std::vector<Module> optionalModules;
		ScopePtr globalScope = std::make_shared<Scope>(this);
		
		ValuePtr listIndexFunctionVarLocation;
		ValuePtr identityFunctionVarLocation;

		ModulePrivilegeFlags allowedModulePrivileges;

		BlockResult needsToReturn(ExpressionPtr expr, ScopePtr scope, Class* classs);
		BlockResult needsToReturn(const std::vector<ExpressionPtr>& subexpressions, ScopePtr scope, Class* classs);
		
		ExpressionPtr consolidated(ExpressionPtr exp, ScopePtr scope, Class* classs);

		ValuePtr getValue(const std::vector<std::string_view>& strings, ScopePtr scope, Class* classs);
		ValuePtr getValue(ExpressionPtr expr, ScopePtr scope, Class* classs);

		ExpressionPtr getExpression(const std::vector<std::string_view>& strings, ScopePtr scope, Class* classs);
		ExpressionPtr getResolveVarExpression(const std::string& name, bool classScope);
		ExpressionPtr parseInterpolatedString(std::string val, ScopePtr scope, Class* classs);

		ScopePtr newClassScope(const std::string& name, ScopePtr scope);
		void closeScope(ScopePtr& scope);
		FunctionRef newFunction(const std::string& name, ScopePtr scope, FunctionRef func);
		FunctionRef newFunction(const std::string& name, ScopePtr scope, 
			const std::vector<std::string>& argNames,
			const std::map<std::string, TypeDescriptor> types,
			const std::map<std::string, ExpressionPtr> defValues);
		FunctionRef newOperator(const std::string& name, ScopePtr scope, FunctionRef func);
		FunctionRef newOperator(const std::string& name, ScopePtr scope, const std::vector<std::string>& argNames,
		                        std::map<std::string, TypeDescriptor> types,
		                        std::map<std::string, ExpressionPtr> defValues);
		FunctionRef newConstructor(const std::string& name, ScopePtr scope, FunctionRef func);
		FunctionRef newConstructor(const std::string& name, ScopePtr scope, 
			const std::vector<std::string>& argNames,
			const std::map<std::string, TypeDescriptor> types,
			const std::map<std::string, ExpressionPtr> defValues);
		Module* getOptionalModule(const std::string& name);
		void createStandardLibrary();
		void createOptionalModules();
	public:
		std::string __DEBUG_OUT__;
		ExceptionType __EXEPTION__{};

		ArenaAllocator arena;

		ScopePtr getGlobalScope() const { return globalScope; }

		ScopePtr insertScope(ScopePtr existing, ScopePtr parent);
		ScopePtr newScope(const std::string& name, ScopePtr scope);
		ScopePtr newScope(const std::string& name) { return newScope(name, globalScope); }
		ScopePtr insertScope(ScopePtr existing) { return insertScope(existing, globalScope); }
		FunctionRef newClass(const std::string& name, ScopePtr scope, const std::unordered_map<std::string, ValuePtr>& variables, const ClassLambda& constructor, const std::unordered_map<std::string, ClassLambda>& functions);
		FunctionRef newClass(const std::string& name, const std::unordered_map<std::string, ValuePtr>& variables, const ClassLambda& constructor, const std::unordered_map<std::string, ClassLambda>& functions) { return newClass(name, globalScope, variables, constructor, functions); }
		FunctionRef newFunction(const std::string& name, ScopePtr scope, const Lambda& lam){ return newFunction(name, scope, make_shared<Function>(name, lam)); }
		FunctionRef newFunction(const std::string& name, const Lambda& lam) { return newFunction(name, globalScope, lam); }
		FunctionRef newFunction(const std::string& name, ScopePtr scope, const ScopedLambda& lam) { return newFunction(name, scope, make_shared<Function>(name, lam)); }
		FunctionRef newFunction(const std::string& name, const ScopedLambda& lam) { return newFunction(name, globalScope, lam); }
		FunctionRef newFunction(const std::string& name, ScopePtr scope, const ClassLambda& lam) { return newFunction(name, scope, make_shared<Function>(name, lam)); }
		ScopePtr newModule(const std::string& name, ModulePrivilegeFlags flags, const std::unordered_map<std::string, Lambda>& functions);
		ValuePtr callFunction(const std::string& name, ScopePtr scope, const List& args);
		ValuePtr callCoro(CoroRef coro);
		ValuePtr callFunction(FunctionRef fnc, ScopePtr scope, const List& args, const std::map<std::string, ValuePtr>& namedArgs, Class* classs = nullptr);
		ValuePtr callFunction(FunctionRef fnc, ScopePtr scope, const List& args, Class* classs = nullptr) { return callFunction(fnc, scope, args, {}, classs); }
		ValuePtr callFunction(FunctionRef fnc, ScopePtr scope, const List& args, ClassRef classs) { return callFunction(fnc, scope, args, {}, classs.get()); }
		ValuePtr callFunction(const std::string& name, const List& args) { return callFunction(name, globalScope, args); }
		ValuePtr callFunction(FunctionRef fnc, const List& args) { return callFunction(fnc, globalScope, args); }
		template <typename ... Ts>
		ValuePtr callFunctionWithArgs(FunctionRef fnc, ScopePtr scope, Ts...args) {
			List argsList = { make_shared<Value>(args)... };
			return callFunction(fnc, scope, argsList);
		}
		template <typename ... Ts>
		ValuePtr callFunctionWithArgs(FunctionRef fnc, Ts...args) {
			List argsList = { make_shared<Value>(args)... };
			return callFunction(fnc, globalScope, argsList);
		}

		std::vector<Metadata> getMetadata(ScopePtr scope, const std::string& memberName = "") {
			if (memberName.empty()) {
				return scope->scopeMetadata;
			}
			auto it = scope->membersMetadata.find(memberName);
			if (it != scope->membersMetadata.end()) {
				return it->second;
			}
			return {};
		}

		ValuePtr& resolveVariable(const std::string& name, Class* classs, ScopePtr scope);
		ValuePtr& resolveVariable(const std::string& name, ScopePtr scope);
		FunctionRef resolveFunction(const std::string& name, Class* classs, ScopePtr scope);
		FunctionRef resolveFunction(const std::string& name, ScopePtr scope);
		bool checkTypesInFunction(FunctionRef func, const List& args);
		FunctionRef resolveOperator(const std::string& name, ScopePtr scope, const List& args);
		ScopePtr resolveScope(const std::string& name, ScopePtr scope);
		TypeDescriptor checkTypeInScope(const std::string& name, ScopePtr scope);

		ValuePtr resolveVariable(const std::string& name) { return resolveVariable(name, globalScope); }
		FunctionRef resolveFunction(const std::string& name) { return resolveFunction(name, globalScope); }
		ScopePtr resolveScope(const std::string& name) { return resolveScope(name, globalScope); }
		
		bool readLine(std::string_view text);
		bool evaluate(std::string_view script);
		bool evaluateFile(const std::string& path);
		bool readLine(std::string_view text, ScopePtr scope);
		bool evaluate(std::string_view script, ScopePtr scope);
		bool evaluateFile(const std::string& path, ScopePtr scope);
		void clearState();
		IkigaiScriptInterpreter(ModulePrivilegeFlags priv);
		IkigaiScriptInterpreter(ModulePrivilege priv);
		IkigaiScriptInterpreter();
		
		Parser* parser = nullptr;
		~IkigaiScriptInterpreter();
	};
}

#if defined(__EMSCRIPTEN__) || defined(SCRIPT_INTERNAL_PRINT)
#define SCRIPT_DO_INTERNAL_PRINT
#endif


