#pragma once


#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "stringUtils.hpp"
#include "types.hpp"
#include "scope.hpp"
#include "modules.hpp"
#include "dependencyManager.hpp"
#include "concurrency/scheduler.hpp"
#include "concurrency/native_job_pool.hpp"
#include "bytecode/vm.hpp"
#include "bytecode/compiler.hpp"
#include "bytecode/disassembler.hpp"

#define TEST_MOD

#include "arena.hpp"
#include "value_pool.hpp"

namespace IkigaiScript {
namespace Native {
	template<typename T>
	class NativeClassBuilder;
}

	using namespace std::string_literals;

	class Parser;
	enum class ExecutionMode: uint8_t {
		Interpreter, Bytecode
	};

	struct BlockResult {
		LoopInterupt interrupt = LoopInterupt::None;
		ValuePtr explicitReturn = nullptr;
		ValuePtr yieldReturn = nullptr;
		ValuePtr lastValue = nullptr;
	};

	// Observer interface for tracking script execution at the blueprint-node level.
	// Implement this and call setExecutionObserver() to receive callbacks whenever
	// a statement with a bpNodeId != 0 begins or ends executing.
	struct ExecutionObserver {
		virtual ~ExecutionObserver() = default;
		virtual void onEnterNode(int bpNodeId) {}
		virtual void onExitNode(int bpNodeId) {}
	};

	class IkigaiScriptInterpreter {
	public:
		// valuePool MUST be the first member so it is the last destroyed,
		// after all ValuePtr-holding members (modules, globalScope, vm stack, etc.)
		// that call pool->deallocate in their destructors.
		ValuePool valuePool;

		std::atomic_bool data_is_ready_{};

		inline static uint64_t currentLine = 0;

		ExecutionObserver* executionObserver = nullptr;
		void setExecutionObserver(ExecutionObserver* obs) { executionObserver = obs; }

		ExecutionMode executionMode = ExecutionMode::Interpreter;
		void setExecutionMode(ExecutionMode mode) {
			executionMode = mode;
			if (mode == ExecutionMode::Bytecode && !vm) {
				vm = std::make_unique<VM>(this);
			}
		}
		ExecutionMode getExecutionMode() const {
			return executionMode;
		}

		// True while a compiled-script context is active (compileScript / runCompiledScript).
		// Used to enable lazy bytecode compilation of newly-created generic monomorphs.
		bool bytecodeActive = false;

		bool compileFunctionToBytecode(FunctionRef fnc);

		// Produce a concrete monomorphization of a generic template for a given type map.
		// The specialization is re-parsed and registered in scope on first call; subsequent
		// calls for the same name return the cached entry. Returns nullptr if the template
		// has no genericParams.
		FunctionRef monomorphizeGeneric(FunctionRef fnc,
			const std::map<std::string, std::string>& typeMap,
			ScopePtr scope);

		ValuePtr runStatementsBytecode(const std::vector<ExpressionPtr>& stmts, ScopePtr scope);

		void disassembleAll(std::string& out);
		ConcurrencyScheduler scheduler;
		std::unique_ptr<NativeJobPool> nativePool;
		std::unique_ptr<VM> vm;

	private:
		friend Expression;
		friend class Parser;
		friend class DependencyManager;
		friend class VM;
		friend class BytecodeCompiler;
		template<typename T>
		friend class Native::NativeClassBuilder;
		std::vector<Module> modules;
		std::vector<Module> optionalModules;
		ScopePtr globalScope = std::make_shared<Scope>(this);

		std::unordered_map<std::string, size_t> moduleIndexByName;
		std::unordered_map<std::string, size_t> moduleIndexByPath;
		std::vector<std::string> moduleLoadStack;

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

		ScopePtr newClassScope(const std::string& name, ScopePtr scope = nullptr);
		void closeScope(ScopePtr& scope, bool runDeferred = true);
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
		DependencyManager dependencyManager{this};

		// Allocate a new heap Value via the per-interpreter pool.
		// Use these instead of std::make_shared<Value>() everywhere inside Library.
		template<typename... Args>
		ValuePtr makeValue(Args&&... args) {
			return IkigaiScript::makeValue(valuePool, std::forward<Args>(args)...);
		}
		ValuePtr copyValue(const Value& v) {
			return IkigaiScript::copyValue(valuePool, v);
		}

		ScopePtr getGlobalScope() const {
			return globalScope;
		}

		ScopePtr insertScope(ScopePtr existing, ScopePtr parent);
		ScopePtr newScope(const std::string& name, ScopePtr scope);
		ScopePtr newScope(const std::string& name) {
			return newScope(name, globalScope);
		}
		ScopePtr insertScope(ScopePtr existing) {
			return insertScope(existing, globalScope);
		}
		FunctionRef newClass(const std::string& name, ScopePtr scope, const std::unordered_map<std::string, ValuePtr>& variables, const ClassLambda& constructor, const std::unordered_map<std::string, ClassLambda>& functions);
		FunctionRef newClass(const std::string& name, const std::unordered_map<std::string, ValuePtr>& variables, const ClassLambda& constructor, const std::unordered_map<std::string, ClassLambda>& functions) {
			return newClass(name, globalScope, variables, constructor, functions);
		}
		FunctionRef newFunction(const std::string& name, ScopePtr scope, const Lambda& lam) {
			return newFunction(name, scope, make_shared<Function>(name, lam));
		}
		FunctionRef newFunction(const std::string& name, const Lambda& lam) {
			return newFunction(name, globalScope, lam);
		}
		FunctionRef newFunction(const std::string& name, ScopePtr scope, const ScopedLambda& lam) {
			return newFunction(name, scope, make_shared<Function>(name, lam));
		}
		FunctionRef newFunction(const std::string& name, const ScopedLambda& lam) {
			return newFunction(name, globalScope, lam);
		}
		FunctionRef newFunction(const std::string& name, ScopePtr scope, const ClassLambda& lam) {
			return newFunction(name, scope, make_shared<Function>(name, lam));
		}
		ScopePtr newModule(const std::string& name, ModulePrivilegeFlags flags, const std::unordered_map<std::string, Lambda>& functions);
		Module& loadScriptModule(const std::string& path, const std::string& importerPath = "");
		Module* findModuleByName(const std::string& name);
		bool isExported(const Module& mod, const std::string& symbol);
		void bindImportedSymbol(ScopePtr scope, const Module& mod, const std::string& name, const std::string& alias = "");
		void registerModuleName(const std::string& name, size_t index);
		void registerExport(ScopePtr scope, const std::string& name);
		ValuePtr callFunction(const std::string& name, ScopePtr scope, const List& args);
		ValuePtr callCoro(CoroRef coro);
		void callTask(TaskRef task);
		int pump(int maxSteps = -1) {
			if (nativePool) nativePool->drainCompletions(scheduler);
			return scheduler.pump(this, maxSteps);
		}

		void enableNativePool(size_t numThreads = 0);
		void registerNativeJob(const std::string& name, NativeJob job);

		TaskRef submitNativeJob(const std::string& name,
			const std::vector<ValuePtr>& args = {});
		ValuePtr callFunction(FunctionRef fnc, ScopePtr scope, const List& args, const std::map<std::string, ValuePtr>& namedArgs, Class* classs = nullptr);
		ValuePtr callFunction(FunctionRef fnc, ScopePtr scope, const List& args, Class* classs = nullptr) {
			return callFunction(fnc, scope, args, {}, classs);
		}
		ValuePtr callFunction(FunctionRef fnc, ScopePtr scope, const List& args, ClassRef classs) {
			return callFunction(fnc, scope, args, {}, classs.get());
		}
		ValuePtr callFunction(const std::string& name, const List& args) {
			return callFunction(name, globalScope, args);
		}
		ValuePtr callFunction(FunctionRef fnc, const List& args) {
			return callFunction(fnc, globalScope, args);
		}
		template <typename ... Ts>
		ValuePtr callFunctionWithArgs(FunctionRef fnc, ScopePtr scope, Ts...args) {
			List argsList = {makeValue(args)...};
			return callFunction(fnc, scope, argsList);
		}
		template <typename ... Ts>
		ValuePtr callFunctionWithArgs(FunctionRef fnc, Ts...args) {
			List argsList = {makeValue(args)...};
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
		ValuePtr& resolveVariableForWrite(const std::string& name, Class* classs, ScopePtr scope);
		ValuePtr& resolveVariableForWrite(const std::string& name, ScopePtr scope);
		void commitTransaction(ScopePtr txScope);
		void rollbackTransaction(ScopePtr txScope);
		FunctionRef resolveFunction(const std::string& name, Class* classs, ScopePtr scope);
		FunctionRef resolveFunction(const std::string& name, ScopePtr scope);
		bool checkTypesInFunction(FunctionRef func, const List& args);
		FunctionRef resolveOperator(const std::string& name, ScopePtr scope, const List& args);
		// Resolve the best-matching constructor overload for the given argument list.
		FunctionRef resolveConstructor(const std::string& name, ScopePtr scope, const List& args);
		// Register an additional constructor overload for an existing class.
		// Unlike newConstructor, this appends to the overload list rather than replacing.
		FunctionRef newConstructorOverload(const std::string& name, ScopePtr scope, FunctionRef func);
		ScopePtr resolveScope(const std::string& name, ScopePtr scope);
		TypeDescriptor checkTypeInScope(const std::string& name, ScopePtr scope);

		void registerDefer(ScopePtr scope, ExpressionPtr body);
		void runDefers(ScopePtr scope, Class* classs, size_t fromIndex = 0);
		void executeDeferBody(ExpressionPtr body, ScopePtr scope, Class* classs);

		ValuePtr resolveVariable(const std::string& name) {
			return resolveVariable(name, globalScope);
		}
		FunctionRef resolveFunction(const std::string& name) {
			return resolveFunction(name, globalScope);
		}
		ScopePtr resolveScope(const std::string& name) {
			return resolveScope(name, globalScope);
		}

		bool readLine(std::string_view text);
		bool evaluate(std::string_view script);
		bool evaluateFile(const std::string& path);
		bool readLine(std::string_view text, ScopePtr scope);
		bool evaluate(std::string_view script, ScopePtr scope);
		bool evaluateFile(const std::string& path, ScopePtr scope);

		using CompiledScriptRef = ChunkRef;
		CompiledScriptRef compileScript(std::string_view source);
		CompiledScriptRef compileScriptFile(const std::string& path);

		bool saveCompiledScript(const Chunk& chunk, const std::string& path);
		std::string serializeCompiledScript(const Chunk& chunk);
		CompiledScriptRef loadCompiledScriptFile(const std::string& path);
		CompiledScriptRef deserializeCompiledScript(std::string_view data);
		ValuePtr runCompiledScript(CompiledScriptRef chunk, ScopePtr scope = nullptr);
		ValuePtr runCompiledScriptFile(const std::string& path, ScopePtr scope = nullptr);
		ValuePtr runCompiledScriptString(std::string_view data, ScopePtr scope = nullptr);
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


