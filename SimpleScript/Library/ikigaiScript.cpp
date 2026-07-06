
#include <fstream>
#include <filesystem>
#include "parser.h"

#include "ikigaiScript.h"

#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

#include "exception.h"
#include "expressions.hpp"
#include "lexer.h"
#include "utf8Utils.hpp"

using namespace IkigaiScript;

namespace IkigaiScript {

bool isContainer(Type type) {
	const static std::set<Type> containers = { Type::Array, Type::List, Type::Set, Type::Map, Type::Result };
	return containers.contains(type);
};

bool checkNext(const std::vector<std::string_view>& vec, std::string_view expect) {
	if (!vec.empty() && vec.front() == expect) {
		return true;
	}
	return false;
};
bool expectNext(std::vector<std::string_view>& vec, std::string_view expect) {
	if (!vec.empty() && vec.front() == expect) {
		vec.erase(vec.begin());
		return true;
	}
	throw Exception("Malformed syntax: unexpected token");
}

bool checkConvertTypes(const TypeDescriptor& t1, const TypeDescriptor& t2, IkigaiScriptInterpreter* interpreter) {
	if (t1.isDynamic || t2.isDynamic) {
		return true;
	}

	if (t1.type == Type::Float && (t2.type == Type::Int || t2.type == Type::Bool)) {
		return true;
	}
	if (t1.type == Type::Int && t2.type == Type::Bool) {
		return true;
	}

	if (t1.isNullable && t2.type == Type::Null) {
		return true;
	}

	auto res = t1.type == t2.type;

	//class check
	if (t1.type == Type::Class && t2.type == Type::Class) {
		if (!t1.subtype || !t2.subtype) {
			return false;
		}
		auto& className1 = std::get<std::string>(t1.subtype.value());
		auto& className2 = std::get<std::string>(t2.subtype.value());
		if (className1.empty() || className2.empty()) {
			return false;
		}
		if (className1 == className2) {
			return true;
		}
		if (interpreter) {
			auto scope2 = interpreter->resolveScope(className2, interpreter->getGlobalScope());
			if (scope2) {
				for (const auto& base : scope2->baseClasses) {
					if (base == className1) {
						return true;
					}
				}
			}
		}
		return false;
	}

	//containers check
	if (res && isContainer(t1.type)) {
		//same container with dynamic content
		if (!t1.subtype && !t2.subtype) {
			return true;
		}
		bool b = true;
		if (t1.type == Type::Map || t1.type == Type::Result) {
			if (!t1.subtype2.has_value() || !t2.subtype2.has_value()) {
				return false;
			}
			b = b && checkConvertTypes(*t1.subtype2.value(), *t2.subtype2.value(), interpreter);
		}
		if (!t1.subtype.has_value() || !t2.subtype.has_value()) {
			return false;
		}
		return b && checkConvertTypes(*std::get<std::shared_ptr<TypeDescriptor>>(t1.subtype.value()), *std::get<std::shared_ptr<TypeDescriptor>>(t2.subtype.value()), interpreter);
	}
	return res;
}



// functions for figuring out the type of token

bool isStringLiteral(std::string_view token) {
	return (token.size() > 1 && token.front() == '\"' && token.back() == '\"');
}

bool isCharLiteral(std::string_view token) {
	return (token.size() > 1 && token.front() == '\'' && token.back() == '\'');
}

bool isFloatLiteral(std::string_view token) {
	return (token.size() > 1 && contains(token, '.'));
}

bool isBinaryLiteral(std::string_view token) {
	return (token.size() > 1 && (contains(token, 'b') || contains(token, 'B')));
}

bool isHexLiteral(std::string_view token) {
	return (token.size() > 1 && (contains(token, 'x') || contains(token, 'X')));
}

bool isVarOrFuncToken(std::string_view test) {
	return (test.size() > 0 && !contains(DisallowedIdentifierStartChars, test[0]));
}

bool isNumeric(std::string_view test) {
	if (test.size() > 1 && test[0] == '-') contains(NumStartChars, test[1]);
	return (test.size() > 0 && contains(NumStartChars, test[0]));
}

bool isMathOperator(std::string_view test) {
	if (test.size() == 1) {
		return contains("+-*/%<>=!"s, test[0]);
	}
	if (test.size() == 2) {
		if (test == ".." ) return true;
		return contains(MultiCharTokenStartChars, test[0]) && contains("=+-&|"s, test[1]);
	}
	if (test.size() == 3) {
		return test == "..=";
	}
	return false;
}

bool isUnaryMathOperator(std::string_view test) {
	if (test.size() == 1) {
		return '!' == test[0];
	}
	if (test.size() == 2) {
		return contains("+-"s, test[1]) && test[1] == test[0];
	}
	return false;
}

bool isMemberCall(std::string_view test) {
	if (test.size() == 1) {
		return '.' == test[0] || '?' == test[0];
	}
	return false;
}

bool isOpeningBracketOrParen(std::string_view test) {
	return (test.size() == 1 && (test[0] == '[' || test[0] == '('));
}

bool isClosingBracketOrParen(std::string_view test) {
	return (test.size() == 1 && (test[0] == ']' || test[0] == ')'));
}

} // namespace IkigaiScript

void IkigaiScriptInterpreter::createOptionalModules() {
	newModule("file", ModulePrivilege::fileSystemRead | ModulePrivilege::fileSystemWrite, {
		{ "saveFile", [](const List& args) {
			if (args.size() == 2
				&& args[0]->getType() == Type::String
				&& args[1]->getType() == Type::String
				) {
				auto t = std::ofstream(args[1]->getString(), std::ofstream::out);
				t << args[0]->getString();
				t.flush();
				return std::make_shared<Value>(true);
			}
			return std::make_shared<Value>(false);
		}},
		{ "readFile", [](const List& args) {
			if (args.size() == 1 && args[0]->getType() == Type::String) {
				std::stringstream buffer;
				buffer << std::ifstream(args[0]->getString()).rdbuf();
				return make_shared<Value>(buffer.str());
			}
			return std::make_shared<Value>();
		}},
		});

	newModule("thread", ModulePrivilege::fileSystemRead | ModulePrivilege::experimental, {
		{ "newThread", [this](const List& args) {
			if (args.size() == 1 && args[0]->getType() == Type::Function) {
				auto func = args[0]->getFunction();
				auto ptr = new std::thread([this, func]() {
					callFunction(func, {});
				});
				return make_shared<Value>(ptr);
			}
			return std::make_shared<Value>();
		}},
		{ "joinThread", [](const List& args) {
			if (args.size() == 1 && args[0]->getType() == Type::Pointer) {
				std::thread* ptr = (std::thread*)args[0]->getPointer();
				ptr->join();
				delete ptr;
			}
			return std::make_shared<Value>();
		}},
		});
}

ScopePtr IkigaiScriptInterpreter::newModule(const std::string& name, ModulePrivilegeFlags flags, const std::unordered_map<std::string, Lambda>& functions) {
	auto& modSource = flags ? optionalModules : modules;
	modSource.emplace_back(flags, std::make_shared<Scope>(name, this));
	auto scope = modSource.back().scope;

	for (auto& funcPair : functions) {
		newFunction(funcPair.first, scope, funcPair.second);
	}

	if (!flags) {
		registerModuleName(name, modules.size() - 1);
	}

	return modSource.back().scope;
}

namespace fs = std::filesystem;

static std::string resolvePath(const std::string& path, const std::string& importerPath) {
	try {
		fs::path p(path);
		if (p.is_absolute()) {
			return fs::weakly_canonical(p).generic_string();
		}
		if (!importerPath.empty()) {
			fs::path imp(importerPath);
			return fs::weakly_canonical(imp.parent_path() / p).generic_string();
		}
		return fs::weakly_canonical(fs::current_path() / p).generic_string();
	}
	catch (...) {
		return path;
	}
}

struct ParserStateSaver {
	Parser* parser;
	ParseState parseState;
	std::vector<std::string_view> parseStrings;
	int outerNestLayer;
	bool lastStatementClosedScope;
	ParseState prevState;
	int parseLambda;
	int parseLambdaBrakets;
	int genericBodyNesting;
	int getExpressionDepth;
	ExpressionPtr currentExpression;
	ExpressionPtr previousExpression;
	bool nextStatementIsExported;

	ParserStateSaver(Parser* p) : parser(p) {
		parseState = p->parseState;
		parseStrings = std::move(p->parseStrings);
		outerNestLayer = p->outerNestLayer;
		lastStatementClosedScope = p->lastStatementClosedScope;
		prevState = p->prevState;
		parseLambda = p->parseLambda;
		parseLambdaBrakets = p->parseLambdaBrakets;
		genericBodyNesting = p->genericBodyNesting;
		getExpressionDepth = p->getExpressionDepth;
		currentExpression = p->currentExpression;
		previousExpression = p->previousExpression;
		nextStatementIsExported = p->nextStatementIsExported;

		p->clearParseStacks();
		p->currentExpression = nullptr;
		p->previousExpression = nullptr;
		p->nextStatementIsExported = false;
	}

	~ParserStateSaver() {
		parser->parseState = parseState;
		parser->parseStrings = std::move(parseStrings);
		parser->outerNestLayer = outerNestLayer;
		parser->lastStatementClosedScope = lastStatementClosedScope;
		parser->prevState = prevState;
		parser->parseLambda = parseLambda;
		parser->parseLambdaBrakets = parseLambdaBrakets;
		parser->genericBodyNesting = genericBodyNesting;
		parser->getExpressionDepth = getExpressionDepth;
		parser->currentExpression = currentExpression;
		parser->previousExpression = previousExpression;
		parser->nextStatementIsExported = nextStatementIsExported;
	}
};

Module& IkigaiScriptInterpreter::loadScriptModule(const std::string& path, const std::string& importerPath) {
	ParserStateSaver saver(parser);
	std::string normPath = resolvePath(path, importerPath);

	if (std::find(moduleLoadStack.begin(), moduleLoadStack.end(), normPath) != moduleLoadStack.end()) {
		throw Exception("Circular import detected: " + normPath);
	}

	auto pathIt = moduleIndexByPath.find(normPath);
	if (pathIt != moduleIndexByPath.end()) {
		return modules[pathIt->second];
	}

	moduleLoadStack.push_back(normPath);

	std::string s;
	auto file = std::ifstream(normPath);
	if (!file) {
		moduleLoadStack.pop_back();
		throw Exception("Module file not found: " + normPath);
	}
	file.seekg(0, std::ios::end);
	s.reserve(file.tellg());
	file.seekg(0, std::ios::beg);
	if (normPath.size() > 3 && normPath.substr(normPath.size() - 3) == ".sh") {
		std::string tmp;
		getline(file, tmp);
	}
	s.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	std::string stemName = fs::path(normPath).stem().string();
	auto moduleScope = std::make_shared<Scope>(stemName, this);

	modules.emplace_back(0, moduleScope, normPath, std::unordered_set<std::string>{}, true);
	size_t modIdx = modules.size() - 1;

	moduleIndexByPath[normPath] = modIdx;
	moduleIndexByName[stemName] = modIdx;

	auto oldPath = parser->currentScriptPath;
	parser->currentScriptPath = normPath;

	auto oldScope = parser->parseScope;
	parser->parseScope = moduleScope;

	bool failed = false;
	try {
		failed = parser->evaluate(s);
	}
	catch (...) {
		parser->parseScope = oldScope;
		parser->currentScriptPath = oldPath;
		moduleLoadStack.pop_back();
		throw;
	}
	parser->parseScope = oldScope;
	parser->currentScriptPath = oldPath;

	moduleLoadStack.pop_back();
	if (failed) {
		throw Exception("Failed to evaluate module: " + normPath);
	}

	std::string finalName = moduleScope->name;
	if (finalName != stemName) {
		moduleIndexByName.erase(stemName);
		moduleIndexByName[finalName] = modIdx;
	}

	return modules[modIdx];
}

Module* IkigaiScriptInterpreter::findModuleByName(const std::string& name) {
	auto it = moduleIndexByName.find(name);
	if (it != moduleIndexByName.end()) {
		return &modules[it->second];
	}
	return nullptr;
}

bool IkigaiScriptInterpreter::isExported(const Module& mod, const std::string& symbol) {
	if (!mod.isScriptModule) {
		return true;
	}
	return mod.exports.contains(symbol);
}

void IkigaiScriptInterpreter::bindImportedSymbol(ScopePtr scope, const Module& mod, const std::string& name, const std::string& alias) {
	if (!isExported(mod, name)) {
		throw Exception("Symbol '" + name + "' is not exported from module '" + mod.scope->name + "'");
	}

	std::string targetName = alias.empty() ? name : alias;

	if (scope->variables.contains(targetName) || scope->functions.contains(targetName) || scope->scopes.contains(targetName)) {
		throw Exception("Import conflict: name '" + targetName + "' is already defined in the current scope");
	}

	bool found = false;

	auto classIt = mod.scope->scopes.find(name);
	if (classIt != mod.scope->scopes.end()) {
		scope->scopes[targetName] = classIt->second;
		auto funcIt = mod.scope->functions.find(name);
		if (funcIt != mod.scope->functions.end()) {
			scope->functions[targetName] = funcIt->second;
			auto funcvar = resolveVariable(targetName, scope);
			*funcvar = Value(funcIt->second);
		}
		found = true;
	}

	if (!found) {
		auto funcIt = mod.scope->functions.find(name);
		if (funcIt != mod.scope->functions.end()) {
			scope->functions[targetName] = funcIt->second;
			auto funcvar = resolveVariable(targetName, scope);
			*funcvar = Value(funcIt->second);
			found = true;
		}
	}

	if (!found) {
		auto varIt = mod.scope->variables.find(name);
		if (varIt != mod.scope->variables.end()) {
			scope->variables[targetName] = varIt->second;
			found = true;
		}
	}

	if (!found) {
		throw Exception("Symbol '" + name + "' not found in module '" + mod.scope->name + "'");
	}
}

void IkigaiScriptInterpreter::registerModuleName(const std::string& name, size_t index) {
	moduleIndexByName[name] = index;
}

void IkigaiScriptInterpreter::registerExport(ScopePtr scope, const std::string& name) {
	while (scope && scope->parent) {
		scope = scope->parent;
	}
	if (scope) {
		auto iter = std::find_if(modules.begin(), modules.end(), [&scope](const auto& mod) {
			return mod.scope == scope;
		});
		if (iter != modules.end()) {
			iter->exports.insert(name);
		}
	}
}

Module* IkigaiScriptInterpreter::getOptionalModule(const std::string& name) {
	auto iter = std::find_if(optionalModules.begin(), optionalModules.end(), [&name](const auto& mod) {return mod.scope->name == name; });
	if (iter != optionalModules.end()) {
		return &*iter;
	}
	return nullptr;
}


void IkigaiScriptInterpreter::registerDefer(ScopePtr scope, ExpressionPtr body) {
	scope->deferred.push_back(body);
}

void IkigaiScriptInterpreter::runDefers(ScopePtr scope, Class* classs, size_t fromIndex) {
	while (scope->deferred.size() > fromIndex) {
		auto body = scope->deferred.back();
		scope->deferred.pop_back();
		executeDeferBody(body, scope, classs);
	}
}

void IkigaiScriptInterpreter::executeDeferBody(ExpressionPtr body, ScopePtr scope, Class* classs) {
	BlockResult res;
	if (body->type == ExpressionType::Defer) {
		res = needsToReturn(static_cast<DeferExpression*>(body)->subexpressions, scope, classs);
	}
	else if (body->type == ExpressionType::Block) {
		res = needsToReturn(static_cast<BlockExpression*>(body)->subexpressions, scope, classs);
	}
	else {
		res = needsToReturn(body, scope, classs);
	}

	if (res.explicitReturn != nullptr || res.interrupt != LoopInterupt::None) {
		throw Exception("defer body cannot transfer control");
	}
}


void IkigaiScriptInterpreter::createStandardLibrary() {
	newModule("StandardLib"s, 0, {
		{"=", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("=");
			}
			//if (args[0]->typeDescriptor.isConst && args[0]->typeDescriptor.isInit) {
			//	throw Exception("Try assign to const value");
			//}
			//TODO:
			//if (!args[0]->typeDescriptor.isNullable && args[1]->typeDescriptor.type == Type::Null) {
			//	throw Exception("Can not assign null to variable");
			//}
			if (!args[1]->typeDescriptor.isInit) {
				throw Exception("Use not init variable in expression");
			}
			// For type-locked variables (isInit && !isDynamic), use strict type comparison.
			// Only Float←Int upcast is allowed. Bool←Int coercion disallowed for assignment.
			if (args[0]->typeDescriptor.isInit && !args[0]->typeDescriptor.isDynamic) {
				bool compatible = false;
				if (args[0]->typeDescriptor.type == Type::Class || isContainer(args[0]->typeDescriptor.type)) {
					compatible = checkConvertTypes(args[0]->typeDescriptor, args[1]->typeDescriptor, this);
				} else {
					compatible = (args[0]->typeDescriptor.type == args[1]->typeDescriptor.type)
						|| (args[0]->typeDescriptor.type == Type::Float && args[1]->typeDescriptor.type == Type::Int)
						|| (args[0]->typeDescriptor.isNullable && args[1]->typeDescriptor.type == Type::Null);
				}
				if (!compatible) {
					throw TypeConvertError(args[0]->typeDescriptor, args[1]->typeDescriptor);
				}
			} else {
				if (!checkConvertTypes(args[0]->typeDescriptor, args[1]->typeDescriptor, this)) {
					throw TypeConvertError(args[0]->typeDescriptor, args[1]->typeDescriptor);
				}
			}
			// Preserve owner flags across assignment
			TypeDescriptor ownerTd = args[0]->typeDescriptor;
			*args[0] = *args[1];
			args[0]->typeDescriptor.isDynamic = ownerTd.isDynamic;
			args[0]->typeDescriptor.isConst = ownerTd.isConst;
			args[0]->typeDescriptor.isNullable = ownerTd.isNullable;
			args[0]->typeDescriptor.isInit = true;
			if (!ownerTd.isDynamic && ownerTd.isInit) {
				args[0]->typeDescriptor.type = ownerTd.type;
				args[0]->typeDescriptor.subtype = ownerTd.subtype;
				args[0]->typeDescriptor.subtype2 = ownerTd.subtype2;
			}
			return args[0];
		}},

		{"+", [this](const List& args) {
			if (args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 1) {
				auto zero = Value(Int(0));
				upconvert(*args[0], zero);
				return std::make_shared<Value>(zero + *args[0]);
			}
			if (args.size() == 0) {
				return resolveVariable("+");
			}
			return std::make_shared<Value>(*args[0] + *args[1]);
		}},

		{"-", [this](const List& args) {
			if (args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("-");
			}
			if (args.size() == 1) {
				auto zero = Value(Int(0));
				upconvert(*args[0], zero);
				return std::make_shared<Value>(zero - *args[0]);
			}
			return std::make_shared<Value>(*args[0] - *args[1]);
		}},

		{"*", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("*");
			}
			return std::make_shared<Value>(*args[0] * *args[1]);
		}},

		{"/", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("/");
			}
			return std::make_shared<Value>(*args[0] / *args[1]);
		}},

		{"%", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("%");
			}
			return std::make_shared<Value>(*args[0] % *args[1]);
		}},

		{"==", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("==");
			}
			return std::make_shared<Value>((Int)(*args[0] == *args[1]));
		}},

		{"!=", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("!=");
			}
			return std::make_shared<Value>((Int)(*args[0] != *args[1]));
		}},

		{"||", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("||");
			}
			return std::make_shared<Value>((Int)(*args[0] || *args[1]));
		}},

		{"&&", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("&&");
			}
			return std::make_shared<Value>((Int)(*args[0] && *args[1]));
		}},

		{"++", [](const List& args) {
			if (args.size() == 0) {
				return std::make_shared<Value>();
			}
			if (args.size() > 1) {
				throw Exception("Must have 0 or 1 arguments");
			}
			auto i = args.size() - 1;
			if (i) {
				// prefix
				*args[i] += Value(Int(1));
				return args[i];
			}
			else {
				// postfix
				auto val = std::make_shared<Value>(*args[i]);
				*args[i] += Value(Int(1));
				return val;
			}
		}},

		{"--", [](const List& args) {
			if (args.size() == 0) {
				return std::make_shared<Value>();
			}
			if (args.size() > 1) {
				throw Exception("Must have 0 or 1 arguments");
			}
			auto i = args.size() - 1;
			if (i) {
				// prefix
				*args[i] -= Value(Int(1));
				return args[i];
			}
			else {
				// postfix
				auto val = std::make_shared<Value>(*args[i]);
				*args[i] -= Value(Int(1));
				return val;
			}
		}},

		{"+=", [](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return std::make_shared<Value>();
			}
			*args[0] += *args[1];
			return args[0];
		}},

		{"-=", [](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return std::make_shared<Value>();
			}
			*args[0] -= *args[1];
			return args[0];
		}},

		{"*=", [](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return std::make_shared<Value>();
			}
			*args[0] *= *args[1];
			return args[0];
		}},

		{"/=", [](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return std::make_shared<Value>();
			}
			*args[0] /= *args[1];
			return args[0];
		}},

		{">", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable(">");
			}
			return std::make_shared<Value>((Int)(*args[0] > *args[1]));
			}},

		{"<", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("<");
			}
			return std::make_shared<Value>((Int)(*args[0] < *args[1]));
		}},

		{">=", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable(">=");
			}
			return std::make_shared<Value>((Int)(*args[0] >= *args[1]));
		}},

		{"<=", [this](const List& args) {
			if (args.size() == 1 || args.size() > 2) {
				throw Exception("Must have 0 or 2 arguments");
			}
			if (args.size() == 0) {
				return resolveVariable("<=");
			}
			return std::make_shared<Value>((Int)(*args[0] <= *args[1]));
		}},

		{"!", [this](const List& args) {
			if (args.size() == 0) {
				return resolveVariable("!");
			}
			if (args.size() == 1) {
				return std::make_shared<Value>(Int(args[0]->getBool() ? 0 : 1));
			}
			throw Exception("! requires 1 argument");
			return std::make_shared<Value>();
		}},

		{"fact", [](const List& args) {
			if (args.size() != 1) {
				throw Exception("fact requires 1 argument");
			}
			if (args[0]->getType() != Type::Int) return std::make_shared<Value>();
			auto val = Int(1);
			for (auto i = Int(1); i <= args[0]->getInt(); ++i) {
				val *= i;
			}
			return std::make_shared<Value>(val);
		}},

		{"..", [](const List& args) {
			// start..end — exclusive: [start, end)
			if (args.size() != 2) throw Exception(".. requires 2 arguments");
			RangeValue rv;
			rv.start = args[0]->getInt();
			rv.end_ = args[1]->getInt();
			rv.inclusive = false;
			return std::make_shared<Value>(rv);
		}},

		{"..=", [](const List& args) {
			// start..=end — inclusive: [start, end]
			if (args.size() != 2) throw Exception("..= requires 2 arguments");
			RangeValue rv;
			rv.start = args[0]->getInt();
			rv.end_ = args[1]->getInt();
			rv.inclusive = true;
			return std::make_shared<Value>(rv);
		}},

			{ "#", [this](const List& args) {
				if (args.size() > 1) {
					throw Exception("Must have 0 or 1 arguments");
				}
				if (args.size() == 0) {
					return resolveVariable("#");
				}
				return std::make_shared<Value>((Int)args[0]->getHash());
			} },

				// aliases
			{"identity", [](List args) {
				if (args.size() == 0) {
					return std::make_shared<Value>();
				}
				return args[0];
			}},

			{"copy", [](List args) {
				if (args.size() == 0) {
					return std::make_shared<Value>();
				}
				if (args[0]->getType() == Type::Class) {
					return std::make_shared<Value>(std::make_shared<Class>(*args[0]->getClass()));
				}
				return std::make_shared<Value>(*args[0]);
			}},

		{"listindex", [](List args) {
				if (args.size() > 0) {
					if (args.size() == 1) {
						return args[0];
					}

					auto var = args[0];

					// Range slicing: arr[start..end] or arr[start..=end]
					if (args[1]->getType() == Type::Range) {
						auto& rv = args[1]->getRange();
						Int start = rv.start;
						Int end = rv.inclusive ? rv.end_ : rv.end_ - 1;
						if (var->getType() == Type::String) {
							auto& s = var->getString();
							Int cpLen = (Int)utf8Length(s);
							if (start < 0) start = 0;
							if (end >= cpLen) end = cpLen - 1;
							if (start > end) return std::make_shared<Value>(std::string(""));
							return std::make_shared<Value>(utf8Slice(s, (size_t)start, (size_t)end));
						}
						auto makeSlice = [&](auto& container) {
							Int len = (Int)container.size();
							if (start < 0) start = 0;
							if (end >= len) end = len - 1;
							List result;
							for (Int i = start; i <= end; ++i) {
								result.push_back(std::make_shared<Value>(*container[i]));
							}
							return std::make_shared<Value>(result);
						};
						if (var->getType() == Type::List || var->getType() == Type::Tuple) {
							return makeSlice(var->getList());
						}
						if (var->getType() == Type::Array) {
							auto& arr = var->getArray();
							Int len = (Int)arr.size();
							if (start < 0) start = 0;
							if (end >= len) end = len - 1;
							List result;
							for (Int i = start; i <= end; ++i) {
								switch (arr.getType()) {
								case Type::Int: result.push_back(std::make_shared<Value>(arr.value.asInt[i])); break;
								case Type::Float: result.push_back(std::make_shared<Value>(arr.value.asFloat[i])); break;
								case Type::String: result.push_back(std::make_shared<Value>(arr.value.asString[i])); break;
								default: break;
								}
							}
							return std::make_shared<Value>(result);
						}
						return std::make_shared<Value>();
					}

					if (args[1]->getType() != Type::Int) {
						var->upconvert(Type::Map);
					}

					switch (var->getType()) {
					case Type::Array: {
						auto ival = args[1]->getInt();
						auto& arr = var->getArray();
						if (ival < 0 || ival >= (Int)arr.size()) {
							throw Exception("Out of bounds array access index "s + std::to_string(ival) + ", array length " + std::to_string(arr.size()));
						}
						else {
							switch (arr.getType()) {
							case Type::Int:
								return std::make_shared<Value>(arr.value.asInt[ival]);
								break;
							case Type::Float:
								return std::make_shared<Value>(arr.value.asFloat[ival]);
								break;
							case Type::String:
								return std::make_shared<Value>(arr.value.asString[ival]);
								break;
							default:
								throw Exception("Attempting to access array of illegal type");
								break;
							}
						}
					}
					break;
					case Type::String: {
						// Unicode-aware: return Char at code point index
						auto ival = args[1]->getInt();
						char32_t cp = utf8At(var->getString(), (size_t)ival);
						return std::make_shared<Value>(cp);
					}
					break;
						default:
							var = std::make_shared<Value>(*var);
							var->upconvert(Type::List);
							[[fallthrough]];
						case Type::List: {
								auto ival = args[1]->getInt();
								auto& list = var->getList();
								if (ival < 0 || ival >= (Int)list.size()) {
									throw Exception("Out of bounds list access index "s + std::to_string(ival) + ", list length " + std::to_string(list.size()));
								}
								else {
									return list[ival];
								}
						}
						break;
						case Type::Class: {
							auto strval = args[1]->getString();
							auto& struc = var->getClass();
							auto iter = struc->variables.find(strval);
							if (iter == struc->variables.end()) {
								throw Exception("Class `"s + struc->name + "` does not contain member `" + strval + "`");
							}
							else {
								return iter->second;
							}
						}
						break;
						case Type::Map:
					{
						auto& dict = var->getDictionary();
						auto hash = args[1]->getHash();
						if (dict->find(hash) == dict->end()) {
							auto newVal = std::make_shared<Value>();
							newVal->typeDescriptor.isDynamic = true;
							newVal->typeDescriptor.isInit = false; // Need to be false so assignment allows converting type
							(*dict)[hash] = newVal;
						}
						return (*dict)[hash];
					}
					break;
						case Type::Set:
						{
							auto& dict = var->getSet();
							ValuePtr ref = nullptr;// (*dict)[args[1]->getHash()];
							if (ref == nullptr) {
								ref = std::make_shared<Value>();
							}
							return ref;
						}
						break;
						}
					}
					return std::make_shared<Value>();
				}},
// casting
			{"bool", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(Int(0));
					}
					auto val = *args[0];
					val.hardconvert(Type::Int);
					val.value.asInt = (Int)args[0]->getBool();
					return std::make_shared<Value>(val);
				}},

				{"int", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(Int(0));
					}
					auto val = *args[0];
					val.hardconvert(Type::Int);
					return std::make_shared<Value>(val);
				}},

				{"float", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(Float(0.0));
					}
					auto val = *args[0];
					val.hardconvert(Type::Float);
					return std::make_shared<Value>(val);
				}},

			//{"vec3", [](const List& args) {
			//	if (args.size() == 0) {
			//		return std::make_shared<Value>(vec3());
			//	}
			//	if (args.size() < 3) {
			//		auto val = *args[0];
			//		val.hardconvert(Type::Float);
			//		return std::make_shared<Value>(vec3((float)val.getFloat()));
			//	}
			//	auto x = *args[0];
			//	x.hardconvert(Type::Float);
			//	auto y = *args[1];
			//	y.hardconvert(Type::Float);
			//	auto z = *args[2];
			//	z.hardconvert(Type::Float);
			//	return std::make_shared<Value>(vec3((float)x.getFloat(), (float)y.getFloat(), (float)z.getFloat()));
			//	 }},

			{"string", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(""s);
					}
					auto val = *args[0];
					val.hardconvert(Type::String);
					return std::make_shared<Value>(val);
				}},

			{"array", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(Array());
					}
					auto list = std::make_shared<Value>(args);
					list->hardconvert(Type::Array);
					return list;
				}},

			{"list", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(List());
					}
					return std::make_shared<Value>(args);
				}},

				{ "set", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(std::make_shared<Set>());
					}
					if (args.size() == 1) {
						auto val = *args[0];
						val.hardconvert(Type::Set);
						return std::make_shared<Value>(val);
					}
					auto dict = std::make_shared<Value>(std::make_shared<Set>());
					for (auto&& arg : args) {
						auto val = *arg;
						val.hardconvert(Type::Set);
						dict->getSet()->merge(*val.getSet());
					}
					return dict;
				}},

			{"dictionary", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(std::make_shared<Dictionary>());
					}
					if (args.size() == 1) {
						auto val = *args[0];
						val.hardconvert(Type::Map);
						return std::make_shared<Value>(val);
					}
					auto dict = std::make_shared<Value>(std::make_shared<Dictionary>());
					for (auto&& arg : args) {
						auto val = *arg;
						val.hardconvert(Type::Map);
						dict->getDictionary()->merge(*val.getDictionary());
					}
					return dict;
				}},

			{"toarray", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(Array());
					}
					auto val = *args[0];
					val.hardconvert(Type::Array);
					return std::make_shared<Value>(val);
				}},

			{"tolist", [](const List& args) {
					if (args.size() == 0) {
						return std::make_shared<Value>(List());
					}
					auto val = *args[0];
					val.hardconvert(Type::List);
					return std::make_shared<Value>(val);
				}},

					// overal stdlib
						{"typeof", [](List args) {
							if (args.size() == 0) {
								return std::make_shared<Value>();
							}
							return std::make_shared<Value>(getTypeName(args[0]->getType()));
							}},

						{"sqrt", [](const List& args) {
							if (args.size() == 0) {
								return std::make_shared<Value>();
							}
							auto val = *args[0];
							val.hardconvert(Type::Float);
							return std::make_shared<Value>(sqrt(val.getFloat()));
							}},

						{"sin", [](const List& args) {
							if (args.size() == 0) {
								return std::make_shared<Value>();
							}
							auto val = *args[0];
							val.hardconvert(Type::Float);
							return std::make_shared<Value>(sin(val.getFloat()));
							}},

						{"cos", [](const List& args) {
							if (args.size() == 0) {
								return std::make_shared<Value>();
							}
							auto val = *args[0];
							val.hardconvert(Type::Float);
							return std::make_shared<Value>(cos(val.getFloat()));
							}},

						{"tan", [](const List& args) {
							if (args.size() == 0) {
								return std::make_shared<Value>();
							}
							auto val = *args[0];
							val.hardconvert(Type::Float);
							return std::make_shared<Value>(tan(val.getFloat()));
							}},

						{"pow", [](const List& args) {
							if (args.size() < 2) {
								return std::make_shared<Value>(Float(0));
							}
							auto val = *args[0];
							val.hardconvert(Type::Float);
							auto val2 = *args[1];
							val2.hardconvert(Type::Float);
							return std::make_shared<Value>(pow(val.getFloat(), val2.getFloat()));
							}},

						{"abs", [](const List& args) {
							if (args.size() == 0) {
								return std::make_shared<Value>();
							}
							switch (args[0]->getType()) {
							case Type::Int:
								return std::make_shared<Value>(Int(abs(args[0]->getInt())));
								break;
							case Type::Float:
								return std::make_shared<Value>(Float(fabs(args[0]->getFloat())));
								break;
							default:
								return std::make_shared<Value>();
								break;
							}
							}},

						{"min", [](const List& args) {
							if (args.size() < 2) {
								return std::make_shared<Value>();
							}
							auto val = *args[0];
							auto val2 = *args[1];
							upconvertThrowOnNonNumberToNumberCompare(val, val2);
							if (val > val2) {
								return std::make_shared<Value>(val2);
							}
							return std::make_shared<Value>(val);
							}},

						{"max", [](const List& args) {
							if (args.size() < 2) {
								return std::make_shared<Value>();
							}
							auto val = *args[0];
							auto val2 = *args[1];
							upconvertThrowOnNonNumberToNumberCompare(val, val2);
							if (val < val2) {
								return std::make_shared<Value>(val2);
							}
							return std::make_shared<Value>(val);
							}},

						{"swap", [](const List& args) {
							if (args.size() < 2) {
								return std::make_shared<Value>();
							}
							auto v = *args[0];
							*args[0] = *args[1];
							*args[1] = v;

							return std::make_shared<Value>();
							} },

							{ "print", [this](const List& args) {
							for (auto&& arg : args) {
#ifdef TEST_MOD
								__DEBUG_OUT__ += arg->getPrintString();
#else
								printf("%s", arg->getPrintString().c_str());
#endif
							}
#ifndef  TEST_MOD
							printf("\n");
#endif
							return std::make_shared<Value>();
							}},

						{"optionalHas", [](const List& args) {
							if (args.empty() || args[0]->getType() != Type::Optional) {
								return std::make_shared<Value>(false);
							}
							return std::make_shared<Value>(args[0]->getOptional() != nullptr);
							}},

						{"optionalGet", [](const List& args) {
							if (args.empty() || args[0]->getType() != Type::Optional) {
								throw Exception("optionalGet expected an Optional value");
							}
							auto inner = args[0]->getOptional();
							if (!inner) {
								throw Exception("Attempt to get value from empty Optional");
							}
							return inner;
							}},

						{"optionalEmpty", [](const List& args) {
							return std::make_shared<Value>(Value::makeOptional(nullptr));
							}},

						{"resultOk", [](const List& args) {
							if (args.empty()) {
								return std::make_shared<Value>(Value::makeResultOk(std::make_shared<Value>()));
							}
							return std::make_shared<Value>(Value::makeResultOk(args[0]));
							}},

						{"resultErr", [](const List& args) {
							if (args.empty()) {
								return std::make_shared<Value>(Value::makeResultErr(std::make_shared<Value>()));
							}
							return std::make_shared<Value>(Value::makeResultErr(args[0]));
							}},

						{"resultIsOk", [](const List& args) {
							if (args.empty() || args[0]->getType() != Type::Result) {
								return std::make_shared<Value>(false);
							}
							return std::make_shared<Value>(args[0]->resultIsOk());
							}},

						{"resultGet", [](const List& args) {
							if (args.empty() || args[0]->getType() != Type::Result) {
								throw Exception("resultGet expected a Result value");
							}
							if (!args[0]->resultIsOk()) {
								throw Exception("Attempt to get Ok value from Err Result: " + args[0]->resultPayload()->getPrintString());
							}
							return args[0]->resultPayload();
							}},

						{"resultGetErr", [](const List& args) {
							if (args.empty() || args[0]->getType() != Type::Result) {
								throw Exception("resultGetErr expected a Result value");
							}
							if (args[0]->resultIsOk()) {
								throw Exception("Attempt to get Err value from Ok Result: " + args[0]->resultPayload()->getPrintString());
							}
							return args[0]->resultPayload();
							}},

						{"getline", [](const List& args) {
							std::string s;
							// blocking calls are fine
							getline(std::cin, s);
							if (args.size() > 0) {
								*args[0] = Value(s);
							}
							return std::make_shared<Value>(s);
							}},

						{"map", [this](const List& args) {
							if (args.size() < 2 || args[1]->getType() != Type::Function) {
								return std::make_shared<Value>();
							}
							auto ret = std::make_shared<Value>(List());
							auto& retList = ret->getList();
							auto func = args[1]->getFunction();

							if (args[0]->getType() == Type::Array) {
								auto val = *args[0];
								val.upconvert(Type::List);
								for (auto&& v : val.getList()) {
									retList.push_back(callFunction(func, { v }));
								}
								return ret;
							}

							for (auto&& v : args[0]->getList()) {
								retList.push_back(callFunction(func, { v }));
							}
							return ret;

							}},

						{"fold", [this](const List& args) {
							if (args.size() < 3 || args[1]->getType() != Type::Function) {
								return std::make_shared<Value>();
							}

							auto func = args[1]->getFunction();
							auto iter = args[2];

							if (args[0]->getType() == Type::Array) {
								auto val = *args[0];
								val.upconvert(Type::List);
								for (auto&& v : val.getList()) {
									iter = callFunction(func, { iter, v });
								}
								return iter;
							}

							for (auto&& v : args[0]->getList()) {
								iter = callFunction(func, { iter, v });
							}
							return iter;
							}},

						{"clock", [](const List&) {
							return std::make_shared<Value>(Int(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
							}},

						{"getduration", [](const List& args) {
							if (args.size() == 2 && args[0]->getType() == Type::Int && args[1]->getType() == Type::Int) {
								std::chrono::duration<double> duration = std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[1]->getInt())) -
									std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[0]->getInt()));
								return std::make_shared<Value>(Float(duration.count()));
							}
							return std::make_shared<Value>();
							}},

						{"timesince", [](const List& args) {
							if (args.size() == 1 && args[0]->getType() == Type::Int) {
								std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() -
									std::chrono::high_resolution_clock::time_point(std::chrono::nanoseconds(args[0]->getInt()));
								return std::make_shared<Value>(Float(duration.count()));
							}
							return std::make_shared<Value>();
							}},

								// collection functions
								{"length", [](const List& args) {
									if (args.size() == 0 || (int)args[0]->getType() < (int)Type::String) {
										return std::make_shared<Value>(Int(0));
									}
									if (args[0]->getType() == Type::String) {
										// Return Unicode code point count, not byte count
										return std::make_shared<Value>((Int)utf8Length(args[0]->getString()));
									}
									if (args[0]->getType() == Type::Array) {
										return std::make_shared<Value>((Int)args[0]->getArray().size());
									}
									if (args[0]->getType() == Type::Set) {
										return std::make_shared<Value>((Int)args[0]->getSet()->size());
									}
									if (args[0]->getType() == Type::Map) {
										return std::make_shared<Value>((Int)args[0]->getDictionary()->size());
									}
										if (args[0]->getType() == Type::List) {
											return std::make_shared<Value>((Int)args[0]->getList().size());
										}
										throw Exception("Malformed syntax: unexpected token");
										}},

									{"find", [](const List& args) {
										if (args.size() < 2) {
											return std::make_shared<Value>();
										}
										// String substring search — returns code point index or null
										if (args[0]->getType() == Type::String) {
											if (args[1]->getType() != Type::String) {
												return std::make_shared<Value>();
											}
											size_t startCp = (args.size() >= 3 && args[2]->getType() == Type::Int)
												? (size_t)args[2]->getInt() : 0;
											int idx = utf8Find(args[0]->getString(), args[1]->getString(), startCp);
											if (idx < 0) return std::make_shared<Value>();
											return std::make_shared<Value>((Int)idx);
										}
										if ((int)args[0]->getType() < (int)Type::Array) {
											return std::make_shared<Value>();
										}
										if (args[0]->getType() == Type::Array) {
											if (args[1]->getType() == args[0]->getArray().getType()) {
												switch (args[0]->getArray().getType()) {
												case Type::Int:
												{
													auto& arry = args[0]->getStdVector<Int>();
													auto iter = find(arry.begin(), arry.end(), args[1]->getInt());
													if (iter == arry.end()) {
														return std::make_shared<Value>();
													}
													return std::make_shared<Value>((Int)(iter - arry.begin()));
												}
												break;
												case Type::Float:
												{
													auto& arry = args[0]->getStdVector<Float>();
													auto iter = find(arry.begin(), arry.end(), args[1]->getFloat());
													if (iter == arry.end()) {
														return std::make_shared<Value>();
													}
													return std::make_shared<Value>((Int)(iter - arry.begin()));
												}
												break;
												//case Type::Vec3:
												//{
												//	auto& arry = args[0]->getStdVector<vec3>();
												//	auto iter = find(arry.begin(), arry.end(), args[1]->getVec3());
												//	if (iter == arry.end()) {
												//		return std::make_shared<Value>();
												//	}
												//	return std::make_shared<Value>((Int)(iter - arry.begin()));
												//}
												break;
												case Type::String:
												{
													auto& arry = args[0]->getStdVector<std::string>();
													auto iter = find(arry.begin(), arry.end(), args[1]->getString());
													if (iter == arry.end()) {
														return std::make_shared<Value>();
													}
													return std::make_shared<Value>((Int)(iter - arry.begin()));
												}
												break;
												case Type::Function:
												{
													auto& arry = args[0]->getStdVector<FunctionRef>();
													auto iter = find(arry.begin(), arry.end(), args[1]->getFunction());
													if (iter == arry.end()) {
														return std::make_shared<Value>();
													}
													return std::make_shared<Value>((Int)(iter - arry.begin()));
												}
												break;
												default:
													break;
												}
											}
											return std::make_shared<Value>();
										}
										auto& list = args[0]->getList();
										for (size_t i = 0; i < list.size(); ++i) {
											if (*list[i] == *args[1]) {
												return std::make_shared<Value>((Int)i);
											}
										}
										return std::make_shared<Value>();
										}},

									{"erase", [](const List& args) {
										if (args.size() < 2 || (int)args[0]->getType() < (int)Type::Array || args[1]->getType() != Type::Int) {
											return std::make_shared<Value>();
										}

										if (args[0]->getType() == Type::Array) {
											switch (args[0]->getArray().getType()) {
											case Type::Int:
												args[0]->getStdVector<Int>().erase(args[0]->getStdVector<Int>().begin() + args[1]->getInt());
												break;
											case Type::Float:
												args[0]->getStdVector<Float>().erase(args[0]->getStdVector<Float>().begin() + args[1]->getInt());
												break;
												//case Type::Vec3:
												//	args[0]->getStdVector<vec3>().erase(args[0]->getStdVector<vec3>().begin() + args[1]->getInt());
												//	break;
												case Type::String:
													args[0]->getStdVector<std::string>().erase(args[0]->getStdVector<std::string>().begin() + args[1]->getInt());
													break;
												case Type::Function:
													args[0]->getStdVector<FunctionRef>().erase(args[0]->getStdVector<FunctionRef>().begin() + args[1]->getInt());
													break;
												default:
													break;
												}
											}
										else if (args[0]->getType() == Type::Array) {
											args[0]->getDictionary()->erase(args[1]->getHash());
										} else  {
											args[0]->getList().erase(args[0]->getList().begin() + args[1]->getInt());
										}
											return std::make_shared<Value>();
										}},

					  {"pushback", [](const List& args) {
						  if (args.size() < 2 || (int)args[0]->getType() < (int)Type::Array) {
							  return std::make_shared<Value>();
						  }

						  if (args[0]->getType() == Type::Array) {
							  if (args[0]->getArray().getType() == args[1]->getType()) {
								  switch (args[0]->getArray().getType()) {
								  case Type::Int:
									  args[0]->getStdVector<Int>().push_back(args[1]->getInt());
									  break;
								  case Type::Float:
									  args[0]->getStdVector<Float>().push_back(args[1]->getFloat());
									  break;
									  //case Type::Vec3:
									  //	args[0]->getStdVector<vec3>().push_back(args[1]->getVec3());
									  //	break;
									  case Type::String:
										  args[0]->getStdVector<std::string>().push_back(args[1]->getString());
										  break;
									  case Type::Function:
										  args[0]->getStdVector<FunctionRef>().push_back(args[1]->getFunction());
										  break;
									  default:
										  break;
									  }
								  }
							  }
			   else {
				args[0]->getList().push_back(args[1]);
			}
			return std::make_shared<Value>();
			}},

		{"popback", [](const List& args) {
			if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
				return std::make_shared<Value>();
			}
			if (args[0]->getType() == Type::Array) {
				args[0]->getArray().pop_back();
			}
else {
 args[0]->getList().pop_back();
}
return std::make_shared<Value>();
}},

{"popfront", [](const List& args) {
	if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
		return std::make_shared<Value>();
	}
	if (args[0]->getType() == Type::Array) {
		switch (args[0]->getArray().getType()) {
		case Type::Int:
		{
			auto& arry = args[0]->getStdVector<Int>();
			arry.erase(arry.begin());
		}
		break;
		case Type::Float:
		{
			auto& arry = args[0]->getStdVector<Float>();
			arry.erase(arry.begin());
		}
		break;
		//case Type::Vec3:
		//{
		//	auto& arry = args[0]->getStdVector<vec3>();
		//	arry.erase(arry.begin());
		//}
		break;
		case Type::Function:
		{
			auto& arry = args[0]->getStdVector<FunctionRef>();
			arry.erase(arry.begin());
		}
		break;
		case Type::String:
		{
			auto& arry = args[0]->getStdVector<std::string>();
			arry.erase(arry.begin());
		}
		break;

		default:
			break;
		}
		return args[0];
	}
else {
 auto& list = args[0]->getList();
 list.erase(list.begin());
}
return std::make_shared<Value>();
}},

{"front", [](const List& args) {
	if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
		return std::make_shared<Value>();
	}
	if (args[0]->getType() == Type::Array) {
		switch (args[0]->getArray().getType()) {
		case Type::Int:
			return std::make_shared<Value>(args[0]->getStdVector<Int>().front());
		case Type::Float:
			return std::make_shared<Value>(args[0]->getStdVector<Float>().front());
			//case Type::Vec3:
			//   return std::make_shared<Value>(args[0]->getStdVector<vec3>().front());
			case Type::Function:
				return std::make_shared<Value>(args[0]->getStdVector<FunctionRef>().front());
			case Type::String:
				return std::make_shared<Value>(args[0]->getStdVector<std::string>().front());
			default:
				break;
			}
			return std::make_shared<Value>();
		}
else {
 return args[0]->getList().front();
}
}},

{"back", [](const List& args) {
	if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
		return std::make_shared<Value>();
	}
	if (args[0]->getType() == Type::Array) {
		switch (args[0]->getArray().getType()) {
		case Type::Int:
			return std::make_shared<Value>(args[0]->getStdVector<Int>().back());
		case Type::Float:
			return std::make_shared<Value>(args[0]->getStdVector<Float>().back());
			//case Type::Vec3:
			//	return std::make_shared<Value>(args[0]->getStdVector<vec3>().back());
			case Type::Function:
				return std::make_shared<Value>(args[0]->getStdVector<FunctionRef>().back());
			case Type::String:
				return std::make_shared<Value>(args[0]->getStdVector<std::string>().back());
			default:
				break;
			}
			return std::make_shared<Value>();
		}
else {
 return args[0]->getList().back();
}
}},

{"reverse", [](const List& args) {
	if (args.size() < 1 || (int)args[0]->getType() < (int)Type::String) {
		return std::make_shared<Value>();
	}
	auto copy = std::make_shared<Value>(*args[0]);

	if (args[0]->getType() == Type::String) {
		return std::make_shared<Value>(utf8Reverse(args[0]->getString()));
	}
else if (args[0]->getType() == Type::Array) {
 switch (copy->getArray().getType()) {
 case Type::Int:
 {
	 auto& vl = copy->getStdVector<Int>();
	 std::reverse(vl.begin(), vl.end());
	 return copy;
 }
	 break;
 case Type::Float:
 {
	 auto& vl = copy->getStdVector<Float>();
	 std::reverse(vl.begin(), vl.end());
	 return copy;
 }
	 break;
	 //case Type::Vec3:
	 //{
	 //	auto& vl = copy->getStdVector<vec3>();
	 //	std::reverse(vl.begin(), vl.end());
	 //	return copy;
	 //}
	 //	break;
	 case Type::String:
	 {
		 auto& vl = copy->getStdVector<std::string>();
		 std::reverse(vl.begin(), vl.end());
		 return copy;
	 }
		 break;
	 case Type::Function:
	 {
		 auto& vl = copy->getStdVector<FunctionRef>();
		 std::reverse(vl.begin(), vl.end());
		 return copy;
	 }
		 break;
	 default:
		 break;
	 }
 }
else if (args[0]->getType() == Type::List) {
 auto& vl = copy->getList();
 std::reverse(vl.begin(), vl.end());
 return copy;
}
return std::make_shared<Value>();
}},

{"range", [](const List& args) {
	if (args.size() == 2 && args[0]->getType() == args[1]->getType()) {
		if (args[0]->getType() == Type::Int) {
			auto ret = std::make_shared<Value>(Array(std::vector<Int>{}));
			auto& arry = ret->getStdVector<Int>();
			auto a = args[0]->getInt();
			auto b = args[1]->getInt();
			if (b > a) {
				arry.reserve(b - a);
				for (Int i = a; i <= b; i++) {
					arry.push_back(i);
				}
			}
else {
 arry.reserve(a - b);
 for (Int i = a; i >= b; i--) {
	 arry.push_back(i);
 }
}
return ret;
}
else if (args[0]->getType() == Type::Float) {
 auto ret = std::make_shared<Value>(Array(std::vector<Float>{}));
 auto& arry = ret->getStdVector<Float>();
 Float a = args[0]->getFloat();
 Float b = args[1]->getFloat();
 if (b > a) {
	 arry.reserve((Int)(b - a));
	 for (Float i = a; i <= b; i++) {
		 arry.push_back(i);
	 }
 }
else {
 arry.reserve((Int)(a - b));
 for (Float i = a; i >= b; i--) {
	 arry.push_back(i);
 }
}
return ret;
}
}
if (args.size() < 3 || (int)args[0]->getType() < (int)Type::String) {
	return std::make_shared<Value>();
}
auto indexA = *args[1];
indexA.hardconvert(Type::Int);
auto indexB = *args[2];
indexB.hardconvert(Type::Int);
auto intdexA = indexA.getInt();
auto intdexB = indexB.getInt();

if (args[0]->getType() == Type::String) {
	// intdexA = start code point, intdexB = count of code points
	return std::make_shared<Value>(utf8Substr(args[0]->getString(), (size_t)intdexA, (size_t)intdexB));
}
else if (args[0]->getType() == Type::Array) {
 if (args[0]->getArray().getType() == args[1]->getType()) {
	 switch (args[0]->getArray().getType()) {
	 case Type::Int:
		 return std::make_shared<Value>(Array(std::vector<Int>(args[0]->getStdVector<Int>().begin() + intdexA, args[0]->getStdVector<Int>().begin() + intdexB)));
		 break;
	 case Type::Float:
		 return std::make_shared<Value>(Array(std::vector<Float>(args[0]->getStdVector<Float>().begin() + intdexA, args[0]->getStdVector<Float>().begin() + intdexB)));
		 break;
		 //case Type::Vec3:
		 //	return std::make_shared<Value>(Array(vector<vec3>(args[0]->getStdVector<vec3>().begin() + intdexA, args[0]->getStdVector<vec3>().begin() + intdexB)));
		 //	break;
		 case Type::String:
			 return std::make_shared<Value>(Array(std::vector<std::string>(args[0]->getStdVector<std::string>().begin() + intdexA, args[0]->getStdVector<std::string>().begin() + intdexB)));
			 break;
		 case Type::Function:
			 return std::make_shared<Value>(Array(std::vector<FunctionRef>(args[0]->getStdVector<FunctionRef>().begin() + intdexA, args[0]->getStdVector<FunctionRef>().begin() + intdexB)));
			 break;
		 default:
			 break;
		 }
	 }
 }
else {
 return std::make_shared<Value>(List(args[0]->getList().begin() + intdexA, args[0]->getList().begin() + intdexB));
}
return std::make_shared<Value>();
}},

{"replace", [](const List& args) {
	if (args.size() < 3 || args[0]->getType() != Type::String || args[1]->getType() != Type::String || args[2]->getType() != Type::String) {
		return std::make_shared<Value>();
	}

	std::string& input = args[0]->getString();
	const std::string& lookfor = args[1]->getString();
	const std::string& replacewith = args[2]->getString();
	size_t pos = 0;
	size_t lpos = 0;
	while ((pos = input.find(lookfor, lpos)) != std::string::npos) {
		input.replace(pos, lookfor.size(), replacewith);
		lpos = pos + replacewith.size();
	}

	return std::make_shared<Value>(input);
	}},

{"startswith", [](const List& args) {
	if (args.size() < 2 || args[0]->getType() != Type::String || args[1]->getType() != Type::String) {
		return std::make_shared<Value>();
	}
	return std::make_shared<Value>(Int(startswith(args[0]->getString(), args[1]->getString())));
	}},

{"endswith", [](const List& args) {
	if (args.size() < 2 || args[0]->getType() != Type::String || args[1]->getType() != Type::String) {
		return std::make_shared<Value>();
	}
	return std::make_shared<Value>(Int(endswith(args[0]->getString(), args[1]->getString())));
	}},

{"contains", [](const List& args) {
	if (args.size() < 2 || (int)args[0]->getType() < (int)Type::Array) {
		return std::make_shared<Value>(Int(0));
	}
	if (args[0]->getType() == Type::Array) {
		auto item = *args[1];
		switch (args[0]->getArray().getType()) {
		case Type::Int:
			item.hardconvert(Type::Int);
			return std::make_shared<Value>((Int)contains(args[0]->getStdVector<Int>(), item.getInt()));
		case Type::Float:
			item.hardconvert(Type::Float);
			return std::make_shared<Value>((Int)contains(args[0]->getStdVector<Float>(), item.getFloat()));
			//case Type::Vec3:
			//	item.hardconvert(Type::Vec3);
			//	return std::make_shared<Value>((Int)contains(args[0]->getStdVector<vec3>(), item.getVec3()));
			case Type::String:
				item.hardconvert(Type::String);
				return std::make_shared<Value>((Int)contains(args[0]->getStdVector<std::string>(), item.getString()));
			default:
				break;
			}
			return std::make_shared<Value>(Int(0));
		}
		auto& list = args[0]->getList();
		for (size_t i = 0; i < list.size(); ++i) {
			if (*list[i] == *args[1]) {
				return std::make_shared<Value>(Int(1));
			}
		}
		return std::make_shared<Value>(Int(0));
		}},

	{"split", [](const List& args) {
		if (args.size() > 0 && args[0]->getType() == Type::String) {
			if (args.size() == 1) {
				// Split by code points, not bytes
				return std::make_shared<Value>(Array(utf8SplitCodePoints(args[0]->getString())));
			}
			return std::make_shared<Value>(Array(split(args[0]->getString(), args[1]->getPrintString())));
		}
		return std::make_shared<Value>();
		}},

	{"sort", [](const List& args) {
		if (args.size() < 1 || (int)args[0]->getType() < (int)Type::Array) {
			return std::make_shared<Value>();
		}
		if (args[0]->getType() == Type::Array) {
			switch (args[0]->getArray().getType()) {
			case Type::Int:
			{
				auto& arry = args[0]->getStdVector<Int>();
				std::sort(arry.begin(), arry.end());
			}
			break;
			case Type::Float:
			{
				auto& arry = args[0]->getStdVector<Float>();
				std::sort(arry.begin(), arry.end());
			}
			break;
			case Type::String:
			{
				auto& arry = args[0]->getStdVector<std::string>();
				std::sort(arry.begin(), arry.end());
			}
			break;
			//case Type::Vec3:
			//{
			//	auto& arry = args[0]->getStdVector<vec3>();
			//	std::sort(arry.begin(), arry.end(), [](const vec3& a, const vec3& b) { return a.x < b.x; });
			//}
			//break;
			default:
				break;
			}
			return args[0];
		}
		auto& list = args[0]->getList();
		std::sort(list.begin(), list.end(), [](const ValuePtr& a, const ValuePtr& b) { return *a < *b; });
		return args[0];
		}},
	{ "applyfunction", [this](List args) {
		if (args.size() < 2 || args[1]->getType() != Type::Class) {
			auto func = args[0]->getType() == Type::Function ? args[0] : args[0]->getType() == Type::String ? resolveVariable(args[0]->getString()) : throw Exception("Cannot call non existant function: null");
			auto list = List();
			for (size_t i = 1; i < args.size(); ++i) {
				list.push_back(args[i]);
			}
			return callFunction(func->getFunction(), list);
		}
		return std::make_shared<Value>();
		}},
		});

	listIndexFunctionVarLocation = resolveVariable("listindex", modules.back().scope);
	identityFunctionVarLocation = resolveVariable("identity", modules.back().scope);
}



ValuePtr IkigaiScriptInterpreter::callFunction(const std::string& name, ScopePtr scope, const List& args) {
	return callFunction(resolveFunction(name, scope), scope, args);
}

void each(ExpressionPtr collection, std::function<void(ExpressionPtr)> func) {
	func(collection);
	for (auto&& ex : *collection) {
		each(ex, func);
	}
}

ValuePtr IkigaiScriptInterpreter::callCoro(CoroRef coro) {
	//if (!fnc->isCoro) {
	//	throw;
	//}
	//if (fnc->type == FunctionType::Constructor) {
	//	throw;
	//}
	if (!coro->isActive) {
		return std::make_shared<Value>();
	}
	switch (coro->func->getBodyType()) {
	case FunctionBodyType::Subexpressions: {
		auto& subexpressions = std::get<std::vector<ExpressionPtr>>(coro->func->body);

		ValuePtr returnVal = nullptr;
		int startExpr = coro->expressionId;
		for (int i = startExpr; i < subexpressions.size(); i++) {
			auto sub = subexpressions[i];
			if (sub->type == ExpressionType::Return) {
				returnVal = getValue(sub, coro->scope, coro->classs);
				coro->isActive = false;
				closeScope(coro->scope);
				break;
			}
			else if (sub->type == ExpressionType::Yeld) {
				returnVal = getValue(sub, coro->scope, coro->classs);
				coro->expressionId = i + 1;
				break;
			}
			else {
				auto result = consolidated(sub, coro->scope, coro->classs);
				if (result->type == ExpressionType::Return) {
					returnVal = static_cast<ValueNode*>(result)->value;
					coro->isActive = false;
					closeScope(coro->scope);
					break;
				}
				if (result->type == ExpressionType::Yeld) {
					returnVal = static_cast<ValueNode*>(result)->value;
					coro->expressionId = i + 1;
					break;
				}
			}
		}
		return returnVal ? returnVal : std::make_shared<Value>();
	}
	default: throw Exception("Malformed syntax: unexpected token");
	}

	//empty func
	return std::make_shared<Value>();
}

ValuePtr IkigaiScriptInterpreter::callFunction(FunctionRef fnc, ScopePtr scope, const List& args, const std::map<std::string, ValuePtr>& namedArgs, Class* classs) {
	if (fnc->getBodyType() == FunctionBodyType::Lambda) {
		return get<Lambda>(fnc->body)(args);
	}
	if (!fnc->genericParams.empty()) {
		std::map<std::string, std::string> genericTypeMap;
		for (size_t i = 0; i < fnc->argNames.size(); ++i) {
			auto argName = fnc->argNames[i];
			if (i < args.size()) {
				auto typeDesc = fnc->types[argName];
				if (typeDesc.type == Type::Class && typeDesc.subtype) {
					auto genName = std::get<std::string>(*typeDesc.subtype);
                    if (std::find(fnc->genericParams.begin(), fnc->genericParams.end(), genName) != fnc->genericParams.end()) {
					std::string argTypeName;
					switch (args[i]->getType()) {
						case Type::Int: argTypeName = "Int"; break;
						case Type::Float: argTypeName = "Float"; break;
						case Type::String: argTypeName = "String"; break;
						case Type::Bool: argTypeName = "Bool"; break;
						case Type::List: argTypeName = "List"; break;
						default: argTypeName = "Dynamic"; break;
					}
					genericTypeMap[genName] = argTypeName;
                    }
				}
			}
		}
		
		std::string newName = fnc->name + "__";
		for (size_t i = 0; i < fnc->genericParams.size(); ++i) {
			newName += genericTypeMap[fnc->genericParams[i]];
			if (i + 1 < fnc->genericParams.size()) newName += "_";
		}

		auto iter = scope->functions.find(newName);
		if (iter != scope->functions.end()) {
			return callFunction(iter->second, scope, args, classs);
		}

		std::string newBody = fnc->genericBodyRaw;
		for (auto& param : fnc->genericParams) {
			std::string search = param;
			std::string replace = genericTypeMap[param];
			size_t pos = 0;
			while ((pos = newBody.find(search, pos)) != std::string::npos) {
				bool leftOk = (pos == 0 || !isalnum(newBody[pos - 1]));
				bool rightOk = (pos + search.length() == newBody.length() || !isalnum(newBody[pos + search.length()]));
				if (leftOk && rightOk) {
					newBody.replace(pos, search.length(), replace);
					pos += replace.length();
				} else {
					pos += search.length();
				}
			}
		}

		std::string sig = "fun " + newName + "(";
		auto typeToStr = [&](TypeDescriptor td) -> std::string {
			if (td.type == Type::Class && td.subtype) {
				auto s = std::get<std::string>(*td.subtype);
				if (std::find(fnc->genericParams.begin(), fnc->genericParams.end(), s) != fnc->genericParams.end()) {
					return genericTypeMap[s];
				}
				return s;
			}
			switch(td.type) {
				case Type::Int: return "Int";
				case Type::Float: return "Float";
				case Type::String: return "String";
				case Type::Bool: return "Bool";
				case Type::List: return "List";
				case Type::Array: return "Array";
				case Type::Set: return "Set";
				case Type::Map: return "Map";
				default: return "Dynamic";
			}
		};

		for (size_t i = 0; i < fnc->argNames.size(); ++i) {
			sig += fnc->argNames[i] + ": ";
			sig += typeToStr(fnc->types[fnc->argNames[i]]);
			if (i + 1 < fnc->argNames.size()) sig += ",";
		}
		sig += ")";
		if (fnc->returnType) {
			sig += ": " + typeToStr(*fnc->returnType);
		}
		sig += " { " + newBody + " }";

		Parser p(this);
		p.evaluate(sig);

		return callFunction(resolveFunction(newName, scope), scope, args, classs);
	}

	switch (fnc->getBodyType()) {
	case FunctionBodyType::Subexpressions: {
		auto& subexpressions = get<std::vector<ExpressionPtr>>(fnc->body);
		// get function scope
			//TODO: if corutine try get scope else create new one

		if (fnc->isCoro) { //always create scope for coro
			scope = newScope(fnc->name, scope);
		}
		else {
			// For member/common function calls use a unique call-frame scope to avoid
			// reusing a stale scope from a previous invocation (which caused hangs).
			static unsigned callFrameId = 0;
			std::string callFrame = fnc->name + "__call_" + std::to_string(++callFrameId);
			scope = fnc->type == FunctionType::Constructor ? resolveScope(fnc->name, scope) : newScope(callFrame, scope);
		}

		// Inject explicit captures (fun[a, b, cLocal=c]) before arg binding
		// so that params shadow captures if names collide
		if (!fnc->captures.empty()) {
			for (auto& [name, cap] : fnc->captures) {
				scope->variables[name] = cap.value;
			}
		}

		if (args.size() > fnc->argNames.size()) {
			throw Exception("Malformed syntax: unexpected token (ikigaiScript.cpp:1808) args size " + std::to_string(args.size()) + " expected " + std::to_string(fnc->argNames.size()));
		}

		bool isTyped = false;
		if (!fnc->types.empty()) {
			isTyped = true;
		}

		std::vector<std::string> newVars;
		for (size_t i = 0; i < fnc->argNames.size(); ++i) {
			if (fnc->argNames.size() - 1 == i && fnc->variableArgsParam) {
				break;
			}

			auto& argName = fnc->argNames[i];
			//auto& argType = fnc->types.at(argName);
			auto& ref = scope->variables[argName];
			if (ref == nullptr) {
				newVars.push_back(fnc->argNames[i]);
			}
			if (i < args.size()) {
				// Positional argument provided
				if (isTyped && !checkConvertTypes(args[i]->typeDescriptor, fnc->types.at(argName), this)) {
					throw Exception("Malformed syntax: unexpected token (ikigaiScript.cpp:1831)");
				}
				ref = args[i];
			} else if (namedArgs.count(argName)) {
				// Named argument provided
				auto val = namedArgs.at(argName);
				if (isTyped && !checkConvertTypes(val->typeDescriptor, fnc->types.at(argName), this)) {
					throw Exception("Malformed syntax: unexpected token (ikigaiScript.cpp:1838)");
				}
				ref = val;
			} else if (fnc->defValues.contains(argName)) {
				// Default value
				auto val = getValue(fnc->defValues[argName], scope, classs);
				if (isTyped && !checkConvertTypes(val->typeDescriptor, fnc->types.at(argName), this)) {
					throw Exception("Malformed syntax: unexpected token (ikigaiScript.cpp:1845)");
				}
				ref = val;
			} else {
				throw Exception("Malformed syntax: unexpected token (ikigaiScript.cpp:1849)");
			}
		}

		if (fnc->variableArgsParam) {
			List vargs;
			auto& argName = fnc->argNames[fnc->argNames.size() - 1];
			auto& ref = scope->variables[argName];
			if (ref == nullptr) {
				newVars.push_back(fnc->argNames[fnc->argNames.size() - 1]);
			}
			for (size_t i = fnc->argNames.size() - 1; i < args.size(); i++) {
				if (isTyped && !checkConvertTypes(args[i]->typeDescriptor, fnc->variableArgsParamType.value(), this)) {
					throw Exception("Malformed syntax: unexpected token (ikigaiScript.cpp:1862)");
				}
				vargs.push_back(args[i]);
			}
			ref = std::make_shared<Value>(vargs);
		}

		if (fnc->isCoro) {//return coro instance
			auto coroRes = std::make_shared<Coro>();
			coroRes->isActive = true;
			coroRes->func = fnc;
			coroRes->expressionId = 0;
			coroRes->scope = scope;
			coroRes->classs = classs;
			return std::make_shared<Value>(coroRes);
		}

		ValuePtr returnVal = nullptr;

		try {
			if (fnc->type == FunctionType::Constructor) {
				auto hasExplicitSuperCall = [](const std::vector<ExpressionPtr>& subs) {
					for (auto&& sub : subs) {
						if (sub->type == ExpressionType::FunctionCall) {
							auto funcExpr = static_cast<FunctionExpression*>(sub);
							if (funcExpr->function->getType() == Type::String && funcExpr->function->getString() == "super") {
								return true;
							}
						}
					}
					return false;
				};

				bool isBaseConstructorCall = (classs != nullptr);
				if (isBaseConstructorCall) {
					for (auto&& sub : subexpressions) {
						getValue(sub, scope, classs);
					}
					returnVal = std::make_shared<Value>();
				}
				else {
					returnVal = std::make_shared<Value>(make_shared<Class>(scope));
					returnVal->typeDescriptor.subtype = fnc->name;
					auto activeClasss = returnVal->getClass().get();
					if (!hasExplicitSuperCall(subexpressions)) {
						for (const auto& baseClassName : activeClasss->baseClasses) {
							auto baseScope = resolveScope(baseClassName, globalScope);
							auto baseCtor = resolveFunction(baseClassName, baseScope);
							callFunction(baseCtor, scope, {}, activeClasss);
						}
					}
					for (auto&& sub : subexpressions) {
						getValue(sub, scope, activeClasss);
					}
				}
			}
			else {
				for (auto&& sub : subexpressions) {
					if (sub->type == ExpressionType::Return) {
						returnVal = getValue(sub, scope, classs);
						break;
					}
					else {
						auto result = consolidated(sub, scope, classs);
						if (result->type == ExpressionType::Return) {
							returnVal = static_cast<ValueNode*>(result)->value;
							break;
						}
					}
				}
			}

			if (fnc->type == FunctionType::Constructor && returnVal->getType() == Type::Class) {
				for (auto&& vr : newVars) {
					scope->variables.erase(vr);
					returnVal->getClass()->variables.erase(vr);
				}
			}
		}
		catch (...) {
			closeScope(scope);
			throw;
		}

		if (returnVal) {
			returnVal = std::make_shared<Value>(*returnVal);
		}
		closeScope(scope);
		return returnVal ? returnVal : std::make_shared<Value>();
	}
	case FunctionBodyType::Lambda: {
		scope = newScope(fnc->name, scope);
		auto returnVal = get<Lambda>(fnc->body)(args);
		closeScope(scope);
		return returnVal ? returnVal : std::make_shared<Value>();
	}
	case FunctionBodyType::ScopedLambda: {
		scope = newScope(fnc->name, scope);
		auto returnVal = get<ScopedLambda>(fnc->body)(scope, args);
		closeScope(scope);
		return returnVal ? returnVal : std::make_shared<Value>();
	}
	case FunctionBodyType::ClassLambda: {
		scope = resolveScope(fnc->name, scope);
		if (fnc->type == FunctionType::Constructor) {
			bool isBaseConstructorCall = (classs != nullptr);
			if (isBaseConstructorCall) {
				get<ClassLambda>(fnc->body)(classs, scope, args);
				closeScope(scope);
				return std::make_shared<Value>();
			}
			auto returnVal = make_shared<Value>(make_shared<Class>(scope));
			returnVal->typeDescriptor.subtype = fnc->name;
			get<ClassLambda>(fnc->body)(returnVal->getClass().get(), scope, args);
			closeScope(scope);
			return returnVal;
		}
		else if (fnc->type == FunctionType::Common && args.size() >= 2 && args[1]->getType() == Type::Class) {
			// apply function
			classs = args[1]->getClass().get();
		}
		auto ret = get<ClassLambda>(fnc->body)(classs, scope, args);
		closeScope(scope);
		return ret;
	}
	}

	//empty func
	return std::make_shared<Value>();
}

FunctionRef IkigaiScriptInterpreter::newFunction(const std::string& name, ScopePtr scope, FunctionRef func) {
	auto& ref = scope->functions[name];
	ref = func;
	if (ref->type == FunctionType::Common && scope->isClassScope) {
		ref->type = FunctionType::Member;
	}
	auto funcvar = resolveVariable(name, scope);
	*funcvar = Value(ref);
	return ref;
}

FunctionRef IkigaiScriptInterpreter::newFunction(
	const std::string& name,
	ScopePtr scope,
	const std::vector<std::string>& argNames,
	const std::map<std::string, TypeDescriptor> types,
	const std::map<std::string, ExpressionPtr> defValues
) {
	return newFunction(name, scope, std::make_shared<Function>(name, argNames, types, defValues));
}

FunctionRef IkigaiScriptInterpreter::newOperator(const std::string& name, ScopePtr scope, FunctionRef func) {
	//if (func->type == FunctionType::Operator && scope->isClassScope) {
	//	//Operator could not be in class
	//	throw;
	//}
	if (!scope->operators.contains(name)) {
		scope->operators[name] = {};
	}
	auto& ref = scope->operators[name];
	ref.push_back(func);
	auto funcvar = resolveVariable(name, scope);
	*funcvar = Value(ref.back());
	return ref.back();
}

FunctionRef IkigaiScriptInterpreter::newOperator(
	const std::string& name,
	ScopePtr scope,
	const std::vector<std::string>& argNames,
	const std::map<std::string, TypeDescriptor> types,
	const std::map<std::string, ExpressionPtr> defValues
) {
	return newOperator(name, scope, std::make_shared<Function>(name, argNames, types, defValues, FunctionType::Operator));
}

FunctionRef IkigaiScriptInterpreter::newConstructor(const std::string& name, ScopePtr scope, FunctionRef func) {
	auto& ref = scope->functions[name];
	ref = func;
	ref->type = FunctionType::Constructor;
	auto funcvar = resolveVariable(name, scope);
	*funcvar = Value(ref);
	return ref;
}

FunctionRef IkigaiScriptInterpreter::newConstructor(
	const std::string& name,
	ScopePtr scope,
	const std::vector<std::string>& argNames,
	const std::map<std::string, TypeDescriptor> types,
	const std::map<std::string, ExpressionPtr> defValues
) {
	return newConstructor(name, scope, std::make_shared<Function>(name, argNames, types, defValues));
}

FunctionRef IkigaiScriptInterpreter::newClass(const std::string& name, ScopePtr scope, const std::unordered_map<std::string, ValuePtr>& variables, const ClassLambda& constructor, const std::unordered_map<std::string, ClassLambda>& functions) {
	scope = newClassScope(name, scope);

	scope->variables = variables;
	FunctionRef ret = newConstructor(name, scope->parent, std::make_shared<Function>(name, constructor));

	for (auto& func : functions) {
		newFunction(func.first, scope, func.second);
	}

	closeScope(scope);

	return ret;
}

// name resolution for variables
ValuePtr& IkigaiScriptInterpreter::resolveVariable(const std::string& name, ScopePtr scope) {
	if (name == "super") {
		static ValuePtr superVal = std::make_shared<Value>("super");
		return superVal;
	}
	auto initialScope = scope;
	while (scope) {
		auto iter = scope->variables.find(name);
		if (iter != scope->variables.end()) {
			return iter->second;
		}
		else {
			scope = scope->parent;
		}
	}
	if (!scope) {
		for (auto m : modules) {
			auto iter = m.scope->variables.find(name);
			if (iter != m.scope->variables.end()) {
				return iter->second;
			}
		}
	}
	return initialScope->insertVar(name, std::make_shared<Value>());
}

ValuePtr& IkigaiScriptInterpreter::resolveVariable(const std::string& name, Class* classs, ScopePtr scope) {
	if (classs) {
		auto iter = classs->variables.find(name);
		if (iter != classs->variables.end()) {
			return iter->second;
		}
	}
	return resolveVariable(name, scope);
}

ValuePtr& IkigaiScriptInterpreter::resolveVariableForWrite(const std::string& name, ScopePtr scope) {
	if (name == "super") {
		static ValuePtr superVal = std::make_shared<Value>("super");
		return superVal;
	}
	auto initialScope = scope;
	while (scope) {
		auto iter = scope->variables.find(name);
		if (iter != scope->variables.end()) {
			if (initialScope != scope) {
				auto s = initialScope;
				while (s && s != scope) {
					if (s->isTransactionScope) {
						s->variables[name] = std::make_shared<Value>(*iter->second);
						return s->variables[name];
					}
					s = s->parent;
				}
			}
			return iter->second;
		}
		else {
			scope = scope->parent;
		}
	}
	for (auto m : modules) {
		auto iter = m.scope->variables.find(name);
		if (iter != m.scope->variables.end()) {
			auto s = initialScope;
			while (s) {
				if (s->isTransactionScope) {
					s->variables[name] = std::make_shared<Value>(*iter->second);
					return s->variables[name];
				}
				s = s->parent;
			}
			return iter->second;
		}
	}
	auto s = initialScope;
	while (s) {
		if (s->isTransactionScope) {
			return s->insertVar(name, std::make_shared<Value>());
		}
		s = s->parent;
	}
	return initialScope->insertVar(name, std::make_shared<Value>());
}

ValuePtr& IkigaiScriptInterpreter::resolveVariableForWrite(const std::string& name, Class* classs, ScopePtr scope) {
	if (classs) {
		auto iter = classs->variables.find(name);
		if (iter != classs->variables.end()) {
			return iter->second;
		}
	}
	return resolveVariableForWrite(name, scope);
}

void IkigaiScriptInterpreter::commitTransaction(ScopePtr txScope) {
	if (!txScope) return;
	for (auto& [name, val] : txScope->variables) {
		auto parentScope = txScope->parent;
		bool found = false;
		while (parentScope) {
			auto iter = parentScope->variables.find(name);
			if (iter != parentScope->variables.end()) {
				*iter->second = *val;
				iter->second->typeDescriptor = val->typeDescriptor;
				found = true;
				break;
			}
			parentScope = parentScope->parent;
		}
		if (!found) {
			for (auto m : modules) {
				auto iter = m.scope->variables.find(name);
				if (iter != m.scope->variables.end()) {
					*iter->second = *val;
					iter->second->typeDescriptor = val->typeDescriptor;
					found = true;
					break;
				}
			}
		}
	}
}

void IkigaiScriptInterpreter::rollbackTransaction(ScopePtr txScope) {
	if (!txScope) return;
	txScope->variables.clear();
}

FunctionRef IkigaiScriptInterpreter::resolveFunction(const std::string& name, Class* classs, ScopePtr scope) {
	if (classs) {
		auto iter = classs->functionScope->functions.find(name);
		if (iter != classs->functionScope->functions.end()) {
			return iter->second;
		}
	}
	return resolveFunction(name, scope);
}

// name lookup for callfunction api method
FunctionRef IkigaiScriptInterpreter::resolveFunction(const std::string& name, ScopePtr scope) {
	auto initialScope = scope;
	while (scope) {
		auto iter = scope->functions.find(name);
		if (iter != scope->functions.end()) {
			return iter->second;
		}
		else {
			scope = scope->parent;
		}
	}
	if (!scope) {
		for (auto m : modules) {
			auto iter = m.scope->functions.find(name);
			if (iter != m.scope->functions.end()) {
				return iter->second;
			}
		}
	}
	auto& func = initialScope->functions[name];
	func = std::make_shared<Function>(name);
	return func;
}

//this should call after set default arguments and pack args...
bool IkigaiScriptInterpreter::checkTypesInFunction(FunctionRef func, const List& args) {
	if (func->argNames.size() != args.size()) {
		throw Exception("Malformed syntax: unexpected token (ikigaiScript.cpp:2146)");
	}
	if (func->argTypes.empty()) {
		// it must be last in funcs overrides
		// function is dynamic
		return true;
	}
	int i = 0;
	for (const auto& type : func->argTypes) {
		if (!checkConvertTypes(args[i]->typeDescriptor, type, this)) {
			return false;
		}
		i++;
	}
	return true;
}

FunctionRef IkigaiScriptInterpreter::resolveOperator(const std::string& name, ScopePtr scope, const List& args) {
	auto initialScope = scope;
	while (scope) {
		auto iter = scope->operators.find(name);
		if (iter != scope->operators.end()) {
			for (auto oper : iter->second) {
				if (checkTypesInFunction(oper, args)) {
					return oper;
				}
			}
		}
		else {
			scope = scope->parent;
		}
	}
	if (!scope) {
		for (auto m : modules) {
			auto iter = m.scope->operators.find(name);
			if (iter != m.scope->operators.end()) {
				for (auto oper : iter->second) {
					if (checkTypesInFunction(oper, args)) {
						return oper;
					}
				}
			}
		}
	}
	//auto& func = initialScope->functions[name];
	//func = std::make_shared<Function>(name);
	//return func;
	return nullptr;
}

ScopePtr IkigaiScriptInterpreter::resolveScope(const std::string& name, ScopePtr scope) {
	auto initialScope = scope;
	while (scope) {
		auto iter = scope->scopes.find(name);
		if (iter != scope->scopes.end()) {
			// Only re-parent if it wouldn't create a cycle
			if (initialScope != iter->second && iter->second->parent != initialScope) {
				iter->second->parent = initialScope;
			}
			return iter->second;
		}
		else {
			if (!scope->parent) {
				if (scope->name == name) {
					// Don't re-parent to itself or create a self-loop
					if (initialScope != scope && scope->parent != initialScope) {
						scope->parent = initialScope;
					}
					return scope;
				}
			}
			scope = scope->parent;
		}
	}
	return initialScope->insertScope(make_shared<Scope>(name, initialScope));
}


TypeDescriptor IkigaiScriptInterpreter::checkTypeInScope(const std::string& name, ScopePtr scope) {
	TypeDescriptor desc;
	if (name == "Bool") {
		desc.type = Type::Bool; return desc;
	}
	else if (name == "Int") {
		desc.type = Type::Int; return desc;
	}
	else if (name == "Float") {
		desc.type = Type::Float; return desc;
	}
	else if (name == "String") {
		desc.type = Type::String; return desc;
	}
	else if (name == "List") {
		desc.type = Type::List; return desc;
	}
	else if (name == "Array") {
		desc.type = Type::Array; return desc;
	}
	else if (name == "Set") {
		desc.type = Type::Set; return desc;
	}
	else if (name == "Map") {
		desc.type = Type::Map; return desc;
	}
	else if (name == "Dynamic") {
		desc.isDynamic = true; return desc;
	}
	else if (name == "Char") {
		desc.type = Type::Char; return desc;
	}
	else if (name == "Tuple") {
		desc.type = Type::Tuple; return desc;
	}
	else if (name == "Optional") {
		desc.type = Type::Optional; return desc;
	}
	else if (name == "Result") {
		desc.type = Type::Result; return desc;
	}
	else {
		desc.type = Type::Class;
		desc.subtype = name;
	}

	auto initialScope = scope;
	while (scope) {
		auto iter = scope->scopes.find(name);
		if (iter != scope->scopes.end()) {
			return desc;
		}
		else {
			if (!scope->parent) {
				if (scope->name == name) {
					return desc;
				}
			}
			scope = scope->parent;
		}
	}
	throw Exception("Malformed syntax: unexpected token");
}

BlockResult IkigaiScriptInterpreter::needsToReturn(ExpressionPtr exp, ScopePtr scope, Class* classs) {
	if (exp->type == ExpressionType::Return) {
		return {LoopInterupt::None, getValue(exp, scope, classs), nullptr};
	}
	else {
		auto result = consolidated(exp, scope, classs);
		if (result->type == ExpressionType::Continue) {
			return {LoopInterupt::Continue, nullptr, nullptr};
		}
		if (result->type == ExpressionType::Break) {
			return {LoopInterupt::Break, nullptr, nullptr};
		}
		if (result->type == ExpressionType::Return) {
			auto val = static_cast<ValueNode*>(result)->value;
			return {LoopInterupt::None, std::move(val), nullptr};
		}
		if (result->type == ExpressionType::Value) {
			auto val = static_cast<ValueNode*>(result)->value;
			return {LoopInterupt::None, nullptr, std::move(val)};
		}
	}
	return {LoopInterupt::None, nullptr, nullptr};
}

BlockResult IkigaiScriptInterpreter::needsToReturn(const std::vector<ExpressionPtr>& subexpressions, ScopePtr scope, Class* classs) {
	ValuePtr lastVal = nullptr;
	for (auto&& sub : subexpressions) {
		if (sub->type == ExpressionType::Continue) {
			return {LoopInterupt::Continue, nullptr, lastVal};
		}
		if (sub->type == ExpressionType::Break) {
			return {LoopInterupt::Break, nullptr, lastVal};
		}
		auto returnVal = needsToReturn(sub, scope, classs);
		if (returnVal.explicitReturn || returnVal.interrupt == LoopInterupt::Break || returnVal.interrupt == LoopInterupt::Continue) {
			return returnVal;
		}
		lastVal = returnVal.lastValue;
	}
	return {LoopInterupt::None, nullptr, lastVal};
}

#include <chrono>
// walk the tree depth first and replace any function expressions 
// with a value expression of their results
ExpressionPtr IkigaiScriptInterpreter::consolidated(ExpressionPtr exp, ScopePtr scope, Class* classs) {
	//FOR DEBUGGER
	//using namespace std::chrono_literals;
	//while (!data_is_ready_) {
	//	std::this_thread::sleep_for(0.1s);
	//}
	//data_is_ready_ = false;
	

	LoopInterupt checkLoopInterupt = LoopInterupt::None;
	auto returnDefExpr = [&checkLoopInterupt, this]() -> ExpressionPtr {
		switch (checkLoopInterupt) {
			case LoopInterupt::None: return arena.make<ValueNode>(std::make_shared<Value>(), ExpressionType::Value);
			case LoopInterupt::Break: return arena.make<Break>();
			case LoopInterupt::Continue: return arena.make<Continue>();
		}
		return arena.make<ValueNode>(std::make_shared<Value>(), ExpressionType::Value);
	};
	switch (exp->type) {
	case ExpressionType::DefineVar: {
		auto def = static_cast<DefineVar*>(exp);
		auto val = getValue(def->defineExpression, scope, classs);

		// --- Tuple destructuring: var (a, b) = expr ---
		if (!def->patternNames.empty()) {
			if (val->getType() != Type::Tuple) {
				throw Exception("Destructuring requires a tuple value");
			}
			auto& items = val->getTuple();
			if (def->patternNames.size() != items.size()) {
				throw Exception("Tuple size " + std::to_string(items.size()) +
					" does not match pattern size " + std::to_string(def->patternNames.size()));
			}
			ValuePtr lastBound;
			for (size_t i = 0; i < def->patternNames.size(); ++i) {
				if (def->patternNames[i] == "_") continue;
				auto elem = std::make_shared<Value>(*items[i]);
				TypeDescriptor td = def->typeDescriptor;
				if (td.type == Type::Null) {
					td.type = elem->getType();
					if (!td.subtype && elem->typeDescriptor.subtype) td.subtype = elem->typeDescriptor.subtype;
					if (!td.subtype2 && elem->typeDescriptor.subtype2) td.subtype2 = elem->typeDescriptor.subtype2;
				} else {
					if (!def->typeDescriptor.isDynamic) {
						if (td.type == Type::Float && elem->getType() == Type::Int) {
							elem = std::make_shared<Value>((Float)elem->getInt());
						} else if (!checkConvertTypes(td, elem->typeDescriptor, this)) {
							throw TypeConvertError(td, elem->typeDescriptor);
						}
					}
				}
				td.isInit = true;
				elem->typeDescriptor = td;
				scope->variables[def->patternNames[i]] = elem;
				lastBound = elem;
			}
			return arena.make<ValueNode>(lastBound ? lastBound : std::make_shared<Value>());
		}

		// If the declaration has an explicit static type (not Dynamic, not untyped Null-default),
		// enforce the type constraint at definition time.
		// Upconvert compatible literal types: Float declared var accepts Int literal.
		if (!def->typeDescriptor.isDynamic && def->typeDescriptor.type != Type::Null
			&& def->typeDescriptor.type != val->getType()) {
			if (def->typeDescriptor.type == Type::Float && val->getType() == Type::Int) {
				val = std::make_shared<Value>((Float)val->getInt());
			}
		}
		if (!def->typeDescriptor.isDynamic && def->typeDescriptor.type != Type::Null) {
			if (!checkConvertTypes(def->typeDescriptor, val->typeDescriptor, this)) {
				throw TypeConvertError(def->typeDescriptor, val->typeDescriptor);
			}
		}
		// Apply owner flags from declaration to the value
		TypeDescriptor td = def->typeDescriptor;
		if (td.type == Type::Null) {
			// No explicit type: infer from RHS
			td.type = val->getType();
			if (!td.subtype && val->typeDescriptor.subtype) td.subtype = val->typeDescriptor.subtype;
			if (!td.subtype2 && val->typeDescriptor.subtype2) td.subtype2 = val->typeDescriptor.subtype2;
		}
		td.isInit = true;
		val->typeDescriptor = td;
		scope->variables[def->name] = val;
		return arena.make<ValueNode>(val);
	}
		break;
	case ExpressionType::ResolveVar:
		return arena.make<ValueNode>(resolveVariable(static_cast<ResolveVar*>(exp)->name, scope));
	case ExpressionType::Continue:
	case ExpressionType::Break:
		return exp;
	case ExpressionType::MemberVariable: {
		auto expr = static_cast<MemberVariable*>(exp);

		if (expr->object && expr->object->type == ExpressionType::ResolveVar) {
			auto objName = static_cast<ResolveVar*>(expr->object)->name;
			if (auto* mod = findModuleByName(objName)) {
				if (!isExported(*mod, expr->name)) {
					throw Exception("Symbol '" + expr->name + "' is not exported from module '" + objName + "'");
				}
				return arena.make<ValueNode>(resolveVariable(expr->name, mod->scope));
			}
		}

		auto isSuperExpr = [](ExpressionPtr obj) {
			if (!obj) return false;
			if (obj->type == ExpressionType::ResolveVar) {
				return static_cast<ResolveVar*>(obj)->name == "super";
			}
			if (obj->type == ExpressionType::MemberVariable) {
				auto mv = static_cast<MemberVariable*>(obj);
				return mv->object == nullptr && mv->name == "super";
			}
			return false;
		};

		bool isSuper = isSuperExpr(expr->object);

		if (isSuper) {
			if (!classs || classs->baseClasses.empty()) {
				throw Exception("Cannot use 'super' here");
			}
			return arena.make<ValueNode>(resolveVariable(expr->name, classs, scope));
		}

		auto objVal = expr->object ? getValue(expr->object, scope, classs) : nullptr;
		// Tuple element access via .0, .1, etc.
		if (objVal && (objVal->getType() == Type::Tuple)) {
			bool allDigits = !expr->name.empty();
			for (char c : expr->name) { if (!isdigit(c)) { allDigits = false; break; } }
			if (allDigits) {
				Int idx = (Int)std::stoi(expr->name);
				auto& items = objVal->getTuple();
				if (idx < 0 || idx >= (Int)items.size())
					throw Exception("Tuple index " + std::to_string(idx) + " out of range (size " + std::to_string(items.size()) + ")");
				return arena.make<ValueNode>(items[idx]);
			}
		}
		auto classToUse = objVal ? objVal->getClass().get() : classs;
		return arena.make<ValueNode>(resolveVariable(expr->name, classToUse, scope));
	}
	case ExpressionType::MemberFunctionCall: {
		auto expr = static_cast<MemberFunctionCall*>(exp);
		List args;
		for (auto&& sub : expr->subexpressions) {
			args.push_back(getValue(sub, scope, classs));
		}

		if (expr->object && expr->object->type == ExpressionType::ResolveVar) {
			auto objName = static_cast<ResolveVar*>(expr->object)->name;
			if (auto* mod = findModuleByName(objName)) {
				if (!isExported(*mod, expr->functionName)) {
					throw Exception("Symbol '" + expr->functionName + "' is not exported from module '" + objName + "'");
				}
				auto fnc = resolveFunction(expr->functionName, mod->scope);
				return arena.make<ValueNode>(callFunction(fnc, scope, args, nullptr));
			}
		}

		auto isSuperExpr = [](ExpressionPtr obj) {
			if (!obj) return false;
			if (obj->type == ExpressionType::ResolveVar) {
				return static_cast<ResolveVar*>(obj)->name == "super";
			}
			if (obj->type == ExpressionType::MemberVariable) {
				auto mv = static_cast<MemberVariable*>(obj);
				return mv->object == nullptr && mv->name == "super";
			}
			return false;
		};

		bool isSuper = isSuperExpr(expr->object);

		if (isSuper) {
			if (!classs || classs->baseClasses.empty()) {
				throw Exception("Cannot use 'super' here");
			}
			auto& baseClassName = classs->baseClasses.front();
			auto baseScope = resolveScope(baseClassName, globalScope);
			auto fnc = resolveFunction(expr->functionName, baseScope);
			return arena.make<ValueNode>(callFunction(fnc, scope, args, classs));
		}

		auto val = getValue(expr->object, scope, classs);
		if (val->getType() != Type::Class) {
			args.insert(args.begin(), val);
			return arena.make<ValueNode>(callFunction(resolveVariable(expr->functionName, scope)->getFunction(), scope, args, classs));
		}
		auto owningClass = val->getClass();
		return arena.make<ValueNode>(callFunction(resolveFunction(expr->functionName, owningClass.get(), scope), scope, args, owningClass));
	}
	case ExpressionType::Return:
		return arena.make<ValueNode>(getValue(static_cast<Return*>(exp)->expression, scope, classs));
		break;
	case ExpressionType::Yeld:
		return arena.make<ValueNode>(getValue(static_cast<Yeld*>(exp)->expression, scope, classs));
		break;
	case ExpressionType::FunctionCall:
	{
		List args;
		std::map<std::string, ValuePtr> namedArgs;
		auto funcExpr = static_cast<FunctionExpression*>(exp);

		// Short-circuit evaluation for && and ||
		{
			std::string opName;
			if (funcExpr->function->getType() == Type::Function) {
				auto fnc = funcExpr->function->getFunction();
				if (fnc) opName = fnc->name;
			} else if (funcExpr->function->getType() == Type::String) {
				opName = funcExpr->function->getString();
			}
			if ((opName == "&&" || opName == "||") && funcExpr->subexpressions.size() == 2) {
				auto lhs = getValue(funcExpr->subexpressions[0], scope, classs);
				bool lhsTruthy = lhs->getBool();
				if (opName == "&&" && !lhsTruthy)
					return arena.make<ValueNode>(std::make_shared<Value>(Int(0)));
				if (opName == "||" && lhsTruthy)
					return arena.make<ValueNode>(std::make_shared<Value>(Int(1)));
				auto rhs = getValue(funcExpr->subexpressions[1], scope, classs);
				return arena.make<ValueNode>(std::make_shared<Value>(Int(rhs->getBool() ? 1 : 0)));
			}
		}

		bool isAssignment = false;
		{
			std::string opName;
			if (funcExpr->function->getType() == Type::Function) {
				auto fnc = funcExpr->function->getFunction();
				if (fnc) opName = fnc->name;
			} else if (funcExpr->function->getType() == Type::String) {
				opName = funcExpr->function->getString();
			}
			if (opName == "=") {
				isAssignment = true;
			}
		}

		size_t argIdx = 0;
		for (auto&& sub : funcExpr->subexpressions) {
			if (sub->type == ExpressionType::NamedArgument) {
				auto namedArg = static_cast<NamedArgumentExpression*>(sub);
				namedArgs[namedArg->name] = getValue(namedArg->expression, scope, classs);
			} else {
				if (isAssignment && argIdx == 0) {
					if (sub->type == ExpressionType::ResolveVar) {
						auto rvar = static_cast<ResolveVar*>(sub);
						args.push_back(resolveVariableForWrite(rvar->name, classs, scope));
					} else {
						args.push_back(getValue(sub, scope, classs));
					}
				} else {
					args.push_back(getValue(sub, scope, classs));
				}
				argIdx++;
			}
		}

		if (funcExpr->function->getType() == Type::String) {
			if (funcExpr->function->getString() == "super") {
				if (!classs || classs->baseClasses.empty()) {
					throw Exception("Cannot call super constructor here");
				}
				auto& baseClassName = classs->baseClasses.front();
				auto baseScope = resolveScope(baseClassName, globalScope);
				auto baseCtor = resolveFunction(baseClassName, baseScope);
				return arena.make<ValueNode>(callFunction(baseCtor, scope, args, namedArgs, classs));
			}
			funcExpr->function = resolveVariable(funcExpr->function->getString(), classs, scope);
		}
		if (funcExpr->function->getType() == Type::String && funcExpr->function->getString() == "super") {
			if (!classs || classs->baseClasses.empty()) {
				throw Exception("Cannot call super constructor here");
			}
			auto& baseClassName = classs->baseClasses.front();
			auto baseScope = resolveScope(baseClassName, globalScope);
			auto baseCtor = resolveFunction(baseClassName, baseScope);
			return arena.make<ValueNode>(callFunction(baseCtor, scope, args, namedArgs, classs));
		}
		if (funcExpr->function->getType() == Type::Coro) {
			return arena.make<ValueNode>(callCoro(funcExpr->function->getCoro()));
		}
		return arena.make<ValueNode>(callFunction(funcExpr->function->getFunction(), scope, args, namedArgs, classs));
	}
	break;
	case ExpressionType::Loop:
	{
		scope = newScope("loop", scope);
		auto loopexp = static_cast<Loop*>(exp);
		if (loopexp->initExpression) {
			getValue(loopexp->initExpression, scope, classs);
		}
		ValuePtr returnVal = nullptr;
		List resultList;

		auto _getValue = [&]() {
			if (!loopexp->testExpression) {
				return true;
			}
			return getValue(loopexp->testExpression, scope, classs)->getBool();
		};

		while (returnVal == nullptr && _getValue()) {
			size_t deferMark = scope->deferred.size();
			auto res = needsToReturn(loopexp->subexpressions, scope, classs);
			returnVal = res.explicitReturn;
			if (res.interrupt == LoopInterupt::Break) {
				runDefers(scope, classs, deferMark);
				break;
			}
			if (res.lastValue != nullptr) {
				resultList.push_back(res.lastValue);
			}
			if (returnVal == nullptr && loopexp->iterateExpression) {
				getValue(loopexp->iterateExpression, scope, classs);
			}
			runDefers(scope, classs, deferMark);
		}
		closeScope(scope);
		if (returnVal) {
			return arena.make<ValueNode>(returnVal, ExpressionType::Return);
		}
		else {
			return arena.make<ValueNode>(std::make_shared<Value>(resultList));
		}
	}
	break;
	case ExpressionType::ForEach:
	{
		scope = newScope("loop", scope);
		auto foreachExp = static_cast<Foreach*>(exp);
		auto list = getValue(foreachExp->listExpression, scope, classs);
		auto& subs = foreachExp->subexpressions;
		ValuePtr returnVal = nullptr;
		
		std::vector<ValuePtr> results;
		
		auto handleIteration = [&](size_t idx, ValuePtr key, ValuePtr val) {
			if (foreachExp->iterateNames.size() == 1) {
				scope->variables[foreachExp->iterateNames[0]] = std::make_shared<Value>(*val);
			} else if (foreachExp->iterateNames.size() == 2) {
				if (list->getType() == Type::Map) {
					scope->variables[foreachExp->iterateNames[0]] = std::make_shared<Value>(*key);
					scope->variables[foreachExp->iterateNames[1]] = std::make_shared<Value>(*val);
				} else {
					scope->variables[foreachExp->iterateNames[0]] = std::make_shared<Value>(Int(idx));
					scope->variables[foreachExp->iterateNames[1]] = std::make_shared<Value>(*val);
				}
			} else if (foreachExp->iterateNames.size() == 3 && list->getType() == Type::Map) {
				scope->variables[foreachExp->iterateNames[0]] = std::make_shared<Value>(Int(idx));
				scope->variables[foreachExp->iterateNames[1]] = std::make_shared<Value>(*key);
				scope->variables[foreachExp->iterateNames[2]] = std::make_shared<Value>(*val);
			}
			
			size_t deferMark = scope->deferred.size();
			auto r = needsToReturn(subs, scope, classs);
			checkLoopInterupt = r.interrupt;
			returnVal = r.explicitReturn;
			if (r.lastValue) {
				results.push_back(r.lastValue);
			}
			runDefers(scope, classs, deferMark);
		};

		size_t idx = 0;
		if (list->getType() == Type::Map) {
			for (auto&& in : *list->getDictionary().get()) {
				handleIteration(idx++, std::make_shared<Value>(in.first), in.second);
				if (checkLoopInterupt == LoopInterupt::Break || returnVal) break;
			}
		} else if (list->getType() == Type::Set) {
			for (auto&& in : *list->getSet().get()) {
				handleIteration(idx++, nullptr, in);
				if (checkLoopInterupt == LoopInterupt::Break || returnVal) break;
			}
		} else if (list->getType() == Type::List) {
			for (auto&& in : list->getList()) {
				handleIteration(idx++, nullptr, in);
				if (checkLoopInterupt == LoopInterupt::Break || returnVal) break;
			}
		} else if (list->getType() == Type::Array) {
			auto& arr = list->getArray();
			switch (arr.getType()) {
			case Type::Int: {
				for (auto&& in : list->getStdVector<Int>()) {
					handleIteration(idx++, nullptr, std::make_shared<Value>(in));
					if (checkLoopInterupt == LoopInterupt::Break || returnVal) break;
				}
			} break;
			case Type::Float: {
				for (auto&& in : list->getStdVector<Float>()) {
					handleIteration(idx++, nullptr, std::make_shared<Value>(in));
					if (checkLoopInterupt == LoopInterupt::Break || returnVal) break;
				}
			} break;
			case Type::String: {
				for (auto&& in : list->getStdVector<std::string>()) {
					handleIteration(idx++, nullptr, std::make_shared<Value>(in));
					if (checkLoopInterupt == LoopInterupt::Break || returnVal) break;
				}
			} break;
			default: break;
			}
		} else if (list->getType() == Type::Range) {
			auto& rv = list->getRange();
			Int limit = rv.inclusive ? rv.end_ : rv.end_ - 1;
			for (Int i = rv.start; i <= limit; ++i) {
				handleIteration(idx++, nullptr, std::make_shared<Value>(i));
				if (checkLoopInterupt == LoopInterupt::Break || returnVal) break;
			}
		} else if (list->getType() == Type::Tuple) {
			for (auto&& in : list->getTuple()) {
				handleIteration(idx++, nullptr, in);
				if (checkLoopInterupt == LoopInterupt::Break || returnVal) break;
			}
		}
		
		closeScope(scope);
		if (returnVal) {
			return arena.make<ValueNode>(returnVal, ExpressionType::Return);
		} else {
			// Find the common type for the array, fallback to dynamic/untyped list if heterogeneous
			if (!results.empty()) {
				auto firstType = results[0]->getType();
				bool homogeneous = true;
				for (auto& r : results) {
					if (r->getType() != firstType) {
						homogeneous = false;
						break;
					}
				}
				if (homogeneous) {
					if (firstType == Type::Int) {
						std::vector<Int> res;
						for (auto& r : results) res.push_back(r->getInt());
						return arena.make<ValueNode>(std::make_shared<Value>(Array(res)), ExpressionType::Value);
					} else if (firstType == Type::Float) {
						std::vector<Float> res;
						for (auto& r : results) res.push_back(r->getFloat());
						return arena.make<ValueNode>(std::make_shared<Value>(Array(res)), ExpressionType::Value);
					} else if (firstType == Type::String) {
						std::vector<std::string> res;
						for (auto& r : results) res.push_back(r->getString());
						return arena.make<ValueNode>(std::make_shared<Value>(Array(res)), ExpressionType::Value);
					}
				}
				// Default to List if heterogeneous or complex type
				List lst;
				for (auto& r : results) lst.push_back(r);
				return arena.make<ValueNode>(std::make_shared<Value>(lst), ExpressionType::Value);
			}
			return arena.make<ValueNode>(std::make_shared<Value>(Array(std::vector<Int>{})), ExpressionType::Value);
		}
	}
	break;
	case ExpressionType::IfElse:
	{
		ValuePtr returnVal = nullptr;
		ValuePtr lastEval = nullptr;
		for (auto& express : static_cast<IfElse*>(exp)->states) {
			if (express.testExpression) {
			}
			if (!express.testExpression || getValue(express.testExpression, scope, classs)->getBool()) {
				scope = newScope("ifelse", scope);
				auto r = needsToReturn(express.subexpressions, scope, classs);
				checkLoopInterupt = r.interrupt;
				returnVal = r.explicitReturn;
				lastEval = r.lastValue;
				closeScope(scope);
				break;
			}
		}
		if (returnVal) {
			return arena.make<ValueNode>(returnVal, ExpressionType::Return);
		}
		else if (lastEval) {
			return arena.make<ValueNode>(lastEval, ExpressionType::Value);
		}
		else {
			return returnDefExpr();
			//return arena.make<ValueNode>(std::make_shared<Value>(), ExpressionType::Value);
		}
	}
	break;
	case ExpressionType::Match:
	{
		auto* matchExpr = static_cast<MatchExpression*>(exp);
		ValuePtr target = getValue(matchExpr->target, scope, classs);
		for (size_t armIdx = 0; armIdx < matchExpr->arms.size(); ++armIdx) {
			auto& arm = matchExpr->arms[armIdx];
			bool matches = false;
			if (arm.pattern == nullptr) {
				matches = true;  // default arm
			} else {
				ValuePtr pattern = getValue(arm.pattern, scope, classs);
				// Range pattern: if pattern is a Range, check if target is in [start, end]
				if (pattern->getType() == Type::Range) {
					auto& rv = pattern->getRange();
					Int tInt = 0;
					if (target->getType() == Type::Int) tInt = target->getInt();
					else if (target->getType() == Type::Float) tInt = (Int)target->getFloat();
					else { matches = false; goto nextArm; }
					matches = rv.inclusive ? (tInt >= rv.start && tInt <= rv.end_)
					                       : (tInt >= rv.start && tInt < rv.end_);
					goto nextArm;
				}
				// Simple equality match
				if (target->getType() != pattern->getType()) {
					if (target->getType() == Type::Int && pattern->getType() == Type::Float)
						matches = ((Float)target->getInt() == pattern->getFloat());
					else if (target->getType() == Type::Float && pattern->getType() == Type::Int)
						matches = (target->getFloat() == (Float)pattern->getInt());
				} else {
					switch (target->getType()) {
					case Type::Int:    matches = target->getInt()    == pattern->getInt();    break;
					case Type::Float:  matches = target->getFloat()  == pattern->getFloat();  break;
					case Type::Bool:   matches = target->getBool()   == pattern->getBool();   break;
					case Type::Char:   matches = target->getChar()   == pattern->getChar();   break;
					case Type::String: matches = target->getString() == pattern->getString(); break;
					default: break;
					}
				}
			}
			nextArm:
			if (matches) {
				scope = newScope("match_arm", scope);
				auto r = needsToReturn(arm.body, scope, classs);
				checkLoopInterupt = r.interrupt;
				closeScope(scope);
				if (r.explicitReturn) return arena.make<ValueNode>(r.explicitReturn, ExpressionType::Return);
				if (r.lastValue)      return arena.make<ValueNode>(r.lastValue, ExpressionType::Value);
				return returnDefExpr();
			}
		}
		throw Exception("Match: no arm matched for the given value");
	}
	break;
	case ExpressionType::TupleLiteral:
	{
		auto* tupleExpr = static_cast<TupleLiteralExpression*>(exp);
		List items;
		for (auto& elem : tupleExpr->elements) {
			items.push_back(getValue(elem, scope, classs));
		}
		return arena.make<ValueNode>(std::make_shared<Value>(Value::makeTuple(std::move(items))), ExpressionType::Value);
	}
	break;
	case ExpressionType::DestructuringAssign:
	{
		auto* da = static_cast<DestructuringAssign*>(exp);
		auto val = getValue(da->valueExpression, scope, classs);
		if (val->getType() != Type::Tuple) {
			throw Exception("Destructuring requires a tuple value");
		}
		auto& items = val->getTuple();
		if (da->patternNames.size() != items.size()) {
			throw Exception("Tuple size " + std::to_string(items.size()) +
				" does not match pattern size " + std::to_string(da->patternNames.size()));
		}
		// Evaluate RHS fully before any assignment (already done above via getValue).
		// Copy elements so (a,b) = (b,a) works correctly.
		std::vector<std::shared_ptr<Value>> copies;
		copies.reserve(items.size());
		for (auto& item : items) copies.push_back(std::make_shared<Value>(*item));

		ValuePtr lastAssigned;
		for (size_t i = 0; i < da->patternNames.size(); ++i) {
			if (da->patternNames[i] == "_") continue;
			auto lhs = resolveVariable(da->patternNames[i], scope);
			auto& rhs = copies[i];
			if (!rhs->typeDescriptor.isInit) {
				throw Exception("Use not init variable in expression");
			}
			if (lhs->typeDescriptor.isConst && lhs->typeDescriptor.isInit) {
				throw Exception("Cannot assign to const variable '" + da->patternNames[i] + "'");
			}
			if (lhs->typeDescriptor.isInit && !lhs->typeDescriptor.isDynamic) {
				bool compatible = (lhs->typeDescriptor.type == rhs->typeDescriptor.type)
					|| (lhs->typeDescriptor.type == Type::Float && rhs->typeDescriptor.type == Type::Int)
					|| (lhs->typeDescriptor.isNullable && rhs->typeDescriptor.type == Type::Null);
				if (!compatible) {
					throw TypeConvertError(lhs->typeDescriptor, rhs->typeDescriptor);
				}
			} else {
				if (!checkConvertTypes(lhs->typeDescriptor, rhs->typeDescriptor, this)) {
					throw TypeConvertError(lhs->typeDescriptor, rhs->typeDescriptor);
				}
			}
			TypeDescriptor ownerTd = lhs->typeDescriptor;
			*lhs = *rhs;
			lhs->typeDescriptor.isDynamic = ownerTd.isDynamic;
			lhs->typeDescriptor.isConst = ownerTd.isConst;
			lhs->typeDescriptor.isNullable = ownerTd.isNullable;
			lhs->typeDescriptor.isInit = true;
			if (!ownerTd.isDynamic && ownerTd.isInit) {
				lhs->typeDescriptor.type = ownerTd.type;
			}
			lastAssigned = lhs;
		}
		return arena.make<ValueNode>(lastAssigned ? lastAssigned : std::make_shared<Value>());
	}
	break;
	case ExpressionType::Defer:
	{
		registerDefer(scope, exp);
		return arena.make<ValueNode>(std::make_shared<Value>());
	}
	break;
	case ExpressionType::SafeBlock:
	{
		auto sblock = static_cast<SafeBlockExpression*>(exp);
		auto txScope = std::make_shared<Scope>("__tx", scope);
		txScope->isTransactionScope = true;
		
		ValuePtr resultValue;
		bool success = false;
		BlockResult r;
		try {
			insertScope(txScope, scope);
			r = needsToReturn(sblock->subexpressions, txScope, classs);
			runDefers(txScope, classs);
			commitTransaction(txScope);
			closeScope(txScope, false);
			success = true;
		}
		catch (const Exception& e) {
			rollbackTransaction(txScope);
			closeScope(txScope, false);
			success = false;
		}
		catch (const std::exception& e) {
			rollbackTransaction(txScope);
			closeScope(txScope, false);
			success = false;
		}
		catch (...) {
			rollbackTransaction(txScope);
			closeScope(txScope, false);
			success = false;
		}

		if (sblock->producesValue) {
			if (success) {
				ValuePtr innerVal = nullptr;
				if (r.explicitReturn) {
					innerVal = r.explicitReturn;
				} else if (r.lastValue) {
					innerVal = r.lastValue;
				} else {
					innerVal = std::make_shared<Value>();
				}
				// Copy output to preserve return safety
				resultValue = std::make_shared<Value>(Value::makeOptional(std::make_shared<Value>(*innerVal)));
			} else {
				resultValue = std::make_shared<Value>(Value::makeOptional(nullptr));
			}
		} else {
			resultValue = std::make_shared<Value>(success);
		}

		return arena.make<ValueNode>(resultValue);
	}
	break;
	case ExpressionType::Block:
	{
		scope = newScope("block", scope);
		auto r = needsToReturn(static_cast<BlockExpression*>(exp)->subexpressions, scope, classs);
		checkLoopInterupt = r.interrupt;
		closeScope(scope);
		if (r.explicitReturn) {
			return arena.make<ValueNode>(r.explicitReturn, ExpressionType::Return);
		}
		else if (r.lastValue) {
			return arena.make<ValueNode>(r.lastValue, ExpressionType::Value);
		}
		else {
			return returnDefExpr();
		}
	}
	break;
	default:
		break;
	}
	auto val = static_cast<ValueNode*>(exp);
	//if (val->value->getType() == Type::Coro) {
	//	auto coroExpr = val->value->getCoro();
	//	return arena.make<ValueNode>(callCoro(coroExpr));
	//}
	return arena.make<ValueNode>(val, ExpressionType::Value);
}

// evaluate an expression from tokens
ValuePtr IkigaiScriptInterpreter::getValue(const std::vector<std::string_view>& strings, ScopePtr scope, Class* classs) {
	return getValue(parser->getExpression(strings, scope, classs), scope, classs);
}

// evaluate an expression from ExpressionPtr
ValuePtr IkigaiScriptInterpreter::getValue(ExpressionPtr exp, ScopePtr scope, Class* classs) {
	if (exp == nullptr) {
		return nullptr;
	}
	// copy the expression so that we don't lose it when we consolidate
	return static_cast<ValueNode*>(consolidated(exp, scope, classs))->value;
}

// since the 'else' block in  an if/elfe is technically in a different scope
// ifelse espressions are not closed immediately and instead left dangling
// until the next expression is anything other than an 'else' or the else is unconditional
//bool IkigaiScriptInterpreter::closeDanglingIfExpression() {
//	if (currentExpression && currentExpression->type == ExpressionType::IfElse) {
//		if (currentExpression->parent) {
//			currentExpression = currentExpression->parent;
//		}
//		else {
//			getValue(currentExpression, parseScope, nullptr);
//			currentExpression = nullptr;
//		}
//		return true;
//	}
//	return false;
//}


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

void IkigaiScriptInterpreter::closeScope(ScopePtr& scope, bool runDeferred) {
	if (runDeferred) {
		runDefers(scope, nullptr);
	}
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


IkigaiScriptInterpreter::~IkigaiScriptInterpreter() { delete parser; }
bool IkigaiScriptInterpreter::evaluate(std::string_view script) { return parser->evaluate(script); }
bool IkigaiScriptInterpreter::evaluateFile(const std::string& path) { return parser->evaluateFile(path); }
bool IkigaiScriptInterpreter::readLine(std::string_view text, ScopePtr scope) { return parser->readLine(text, scope); }
bool IkigaiScriptInterpreter::readLine(std::string_view text) { return parser->readLine(text); }
bool IkigaiScriptInterpreter::evaluate(std::string_view script, ScopePtr scope) { return parser->evaluate(script, scope); }
bool IkigaiScriptInterpreter::evaluateFile(const std::string& path, ScopePtr scope) { return parser->evaluateFile(path, scope); }
void IkigaiScriptInterpreter::clearState() { parser->clearState(); }

IkigaiScriptInterpreter::IkigaiScriptInterpreter(ModulePrivilegeFlags priv) : allowedModulePrivileges(priv) {
	parser = new Parser(this);
	createStandardLibrary();
	if (priv) { createOptionalModules(); }
}
IkigaiScriptInterpreter::IkigaiScriptInterpreter(ModulePrivilege priv) : IkigaiScriptInterpreter(static_cast<ModulePrivilegeFlags>(priv)) {}
IkigaiScriptInterpreter::IkigaiScriptInterpreter() : IkigaiScriptInterpreter(ModulePrivilegeFlags()) {}

