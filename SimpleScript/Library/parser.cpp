#include "parser.h"
#include "ikigaiScript.h"
#include "lexer.h"
#include "exception.h"
#include "expressions.hpp"
#include "utf8Utils.hpp"
#include <fstream>
#include <iostream>

namespace IkigaiScript {

bool isContainer(Type type);
bool checkNext(const std::vector<std::string_view>& vec, std::string_view expect);
bool expectNext(std::vector<std::string_view>& vec, std::string_view expect);
bool checkConvertTypes(const TypeDescriptor& t1, const TypeDescriptor& t2);
bool isStringLiteral(std::string_view token);
bool isCharLiteral(std::string_view token);
bool isFloatLiteral(std::string_view token);
bool isBinaryLiteral(std::string_view token);
bool isHexLiteral(std::string_view token);
bool isVarOrFuncToken(std::string_view test);
bool isNumeric(std::string_view test);
bool isMathOperator(std::string_view test);
bool isMemberCall(std::string_view test);
bool isOpeningBracketOrParen(std::string_view test);
bool isClosingBracketOrParen(std::string_view test);
bool isUnaryMathOperator(std::string_view test);

Parser::Parser(IkigaiScriptInterpreter* interpreter) : interpreter(interpreter) {
	parseScope = interpreter->globalScope;
}

bool needsUnaryPlacementFix(const std::vector<std::string_view>& strings, size_t i) {
	return (isUnaryMathOperator(strings[i]) && (i == 0 || !(isClosingBracketOrParen(strings[i - 1]) || isVarOrFuncToken(strings[i - 1]) || isNumeric(strings[i - 1]))));
}

bool checkPrecedence(OperatorPrecedence curr, OperatorPrecedence neww) {
	return (int)curr < (int)neww || (neww == curr && (neww == OperatorPrecedence::incdec || neww == OperatorPrecedence::assign));
}

ExpressionPtr Parser::getResolveVarExpression(const std::string& name, bool classScope) {
	if (classScope) {
		return interpreter->arena.make<MemberVariable>(nullptr, name);
	}
	return interpreter->arena.make<ResolveVar>(name);
}

// Parse an inline lambda: tokens must begin with "fun" (optionally with capture list [...]
// before "(") and contain the full fun(...){...} token sequence.
// Registers the function in the current scope under a unique name and returns a ResolveVar for it.
ExpressionPtr Parser::parseInlineLambda(std::vector<std::string_view> tokens, ScopePtr scope) {
	static int uniqId = 0;
	++uniqId;
	static std::vector<std::string> namePool;
	namePool.push_back("__anon_lambda__" + std::to_string(uniqId));
	const std::string& lambdaName = namePool.back();

	// Extract capture list tokens (between [ and ]) if present, then remove them from the token stream
	// Remaining tokens are: fun (args) { body }
	std::vector<std::string_view> captureTokens;
	std::vector<std::string_view> funcTokens;
	{
		size_t i = 0;
		funcTokens.push_back(tokens[i++]); // "fun"
		if (i < tokens.size() && tokens[i] == "[") {
			++i; // skip "["
			while (i < tokens.size() && tokens[i] != "]") {
				captureTokens.push_back(tokens[i++]);
			}
			if (i < tokens.size()) ++i; // skip "]"
		}
		while (i < tokens.size()) funcTokens.push_back(tokens[i++]);
	}

	// Build capture entries by resolving variables in the current scope now
	std::vector<std::pair<std::string, Function::CaptureEntry>> pendingCaptures;
	for (size_t i = 0; i < captureTokens.size(); ) {
		if (captureTokens[i] == ",") { ++i; continue; }
		std::string localName = std::string(captureTokens[i++]);
		if (i < captureTokens.size() && captureTokens[i] == "=") {
			++i;
			std::string outerName = (i < captureTokens.size()) ? std::string(captureTokens[i++]) : localName;
			// Copy-capture: snapshot
			auto& outer = interpreter->resolveVariable(outerName, scope);
			Function::CaptureEntry entry;
			entry.kind = Function::CaptureEntry::Kind::Copy;
			entry.value = std::make_shared<Value>(*outer);
			pendingCaptures.push_back({localName, std::move(entry)});
		} else {
			// Ref-capture: share ValuePtr
			auto& outer = interpreter->resolveVariable(localName, scope);
			Function::CaptureEntry entry;
			entry.kind = Function::CaptureEntry::Kind::Ref;
			entry.value = outer;
			pendingCaptures.push_back({localName, std::move(entry)});
		}
	}

	// Insert generated name after "fun"
	std::vector<std::string_view> patched;
	patched.reserve(funcTokens.size() + 1);
	bool nameInserted = false;
	for (auto& tok : funcTokens) {
		patched.push_back(tok);
		if (!nameInserted && tok == "fun") {
			patched.push_back(std::string_view(lambdaName));
			nameInserted = true;
		}
	}

	auto savedParseScope = parseScope;
	auto savedState = parseState;
	auto savedStrings = std::move(parseStrings);
	auto savedExpr = currentExpression;
	auto savedPrevExpr = previousExpression;

	// Always register inline lambdas in the global scope (not in a block scope
	// that may be closed before the lambda is called).
	parseScope = interpreter->globalScope;
	currentExpression = nullptr;
	clearParseStacks();

	for (auto& tok : patched) {
		parse(tok);
	}
	parse(";");

	parseScope = savedParseScope;
	parseState = savedState;
	parseStrings = std::move(savedStrings);
	currentExpression = savedExpr;
	previousExpression = savedPrevExpr;

	// Apply captures to the registered function (registered in globalScope)
	if (!pendingCaptures.empty()) {
		auto fnc = interpreter->resolveFunction(lambdaName, interpreter->globalScope);
		for (auto& [name, entry] : pendingCaptures) {
			fnc->captures[name] = std::move(entry);
		}
	}

	return getResolveVarExpression(lambdaName, scope->isClassScope);
}

ExpressionPtr Parser::parseInterpolatedString(std::string val, ScopePtr scope, Class* classs) {
	size_t pos = 0;
	while (pos < val.size()) {
		if (val[pos] == '\\') {
			pos += 2;
			continue;
		}
		if (val[pos] == '{') {
			break;
		}
		pos++;
	}

	if (pos >= val.size()) {
		return interpreter->arena.make<ValueNode>(std::make_shared<Value>(val), ExpressionType::Value);
	}

	std::string prefix = val.substr(0, pos);
	ExpressionPtr finalExpr = nullptr;
	if (!prefix.empty()) {
		finalExpr = interpreter->arena.make<ValueNode>(std::make_shared<Value>(prefix), ExpressionType::Value);
	}

	size_t braceDepth = 1;
	size_t startExpr = pos + 1;
	size_t endExpr = startExpr;
	bool inString = false;
	bool inChar = false;
	bool inMultiline = false;

	while (endExpr < val.size()) {
		if (val[endExpr] == '\\') {
			endExpr += 2;
			continue;
		}
		if (inString) {
			if (val[endExpr] == '"') inString = false;
		} else if (inMultiline) {
			if (val[endExpr] == '"' && endExpr + 2 < val.size() && val[endExpr+1] == '"' && val[endExpr+2] == '"') {
				inMultiline = false;
				endExpr += 2;
			}
		} else if (inChar) {
			if (val[endExpr] == '\'') inChar = false;
		} else {
			if (val[endExpr] == '"') {
				if (endExpr + 2 < val.size() && val[endExpr+1] == '"' && val[endExpr+2] == '"') {
					inMultiline = true;
					endExpr += 2;
				} else {
					inString = true;
				}
			} else if (val[endExpr] == '\'') {
				inChar = true;
			} else if (val[endExpr] == '{') {
				braceDepth++;
			} else if (val[endExpr] == '}') {
				braceDepth--;
				if (braceDepth == 0) {
					break;
				}
			}
		}
		endExpr++;
	}

	if (endExpr >= val.size()) {
		throw Exception("Unterminated string interpolation");
	}

	std::string inner_expr = val.substr(startExpr, endExpr - startExpr);
	std::string suffix = val.substr(endExpr + 1);

	auto innerTokens = Lexer::Tokenize(inner_expr);
	auto innerExpr = getExpression(innerTokens, scope, classs);

	if (finalExpr) {
		auto addExpr = interpreter->arena.make<FunctionExpression>(interpreter->resolveVariable("+"));
		addExpr->subexpressions.push_back(finalExpr);
		addExpr->subexpressions.push_back(innerExpr);
		finalExpr = addExpr;
	} else {
		finalExpr = innerExpr;
	}

	if (!suffix.empty()) {
		auto suffixExpr = parseInterpolatedString(suffix, scope, classs);
		auto addExpr = interpreter->arena.make<FunctionExpression>(interpreter->resolveVariable("+"));
		addExpr->subexpressions.push_back(finalExpr);
		addExpr->subexpressions.push_back(suffixExpr);
		finalExpr = addExpr;
	}

	return finalExpr;
}

// recursively build an expression tree from a list of tokens
ExpressionPtr Parser::getExpression(std::vector<std::string_view> strings, ScopePtr scope, Class* classs) {
	auto parseArg = [&](std::vector<std::string_view> sub) -> ExpressionPtr {
		if (sub.size() >= 3 && isVarOrFuncToken(sub[0]) && sub[1] == "=") {
			std::string name = std::string(sub[0]);
			std::vector<std::string_view> rhs(sub.begin() + 2, sub.end());
			return interpreter->arena.make<NamedArgumentExpression>(name, getExpression(std::move(rhs), scope, classs));
		}
		return getExpression(std::move(sub), scope, classs);
	};
	ExpressionPtr root = nullptr;
	size_t i = 0;
	while (i < strings.size()) {
		if (isMathOperator(strings[i])) {
			auto prev = root;
			root = interpreter->arena.make<FunctionExpression>(interpreter->resolveVariable(std::string(strings[i]), interpreter->modules[0].scope));
			auto curr = prev;
			if (curr) {
				// find operations of lesser precedence
				if (curr->type == ExpressionType::FunctionCall) {
					auto rootExpression = static_cast<FunctionExpression*>(root);
					auto funcExpr = static_cast<FunctionExpression*>(curr)->function;
					if (funcExpr->getType() != Type::Function) {
						rootExpression->subexpressions.push_back(curr);
					}
					else {
						auto curfunc = funcExpr->getFunction();
						auto newfunc = rootExpression->function->getFunction();
						if (curfunc && checkPrecedence(curfunc->opPrecedence, newfunc->opPrecedence)) {
							while (static_cast<FunctionExpression*>(curr)->subexpressions.back()->type == ExpressionType::FunctionCall) {
								auto fnc = static_cast<FunctionExpression*>(static_cast<FunctionExpression*>(curr)->subexpressions.back())->function;
								if (fnc->getType() != Type::Function) {
									break;
								}
								curfunc = fnc->getFunction();
								if (curfunc && checkPrecedence(curfunc->opPrecedence, newfunc->opPrecedence)) {
									curr = static_cast<FunctionExpression*>(curr)->subexpressions.back();
								}
								else {
									break;
								}
							}
							auto currExpression = static_cast<FunctionExpression*>(curr);
							// swap values around to correct the otherwise incorect order of operations (except unary)
							if (needsUnaryPlacementFix(strings, i)) {
								rootExpression->subexpressions.insert(rootExpression->subexpressions.begin(),
									interpreter->arena.make<ValueNode>(std::make_shared<Value>(), root));
							}
							else {
								rootExpression->subexpressions.push_back(currExpression->subexpressions.back());
								currExpression->subexpressions.pop_back();
							}

							// gather any subexpressions from list literals/indexing or function call args
							if (i + 1 < strings.size() && !isMathOperator(strings[i + 1])) {
								std::vector<std::string_view> minisub = { strings[++i] };
								// list literal or parenthesis expression
								char checkstr = 0;
								if (isOpeningBracketOrParen(strings[i])) {
									checkstr = strings[i][0];
								}
								// list index or function call
								else if (strings.size() > i + 1 && isOpeningBracketOrParen(strings[i + 1])) {
									++i;
									checkstr = strings[i][0];
									minisub.push_back(strings[i]);
								}
								// gather tokens until the end of the sub expression
								if (checkstr != 0) {
									auto endstr = checkstr == '[' ? ']' : ')';
									int nestLayers = 1;
									while (nestLayers > 0 && ++i < strings.size()) {
										if (strings[i].size() == 1) {
											if (strings[i][0] == endstr) {
												--nestLayers;
											}
											else if (strings[i][0] == checkstr) {
												++nestLayers;
											}
										}
										minisub.push_back(strings[i]);
										if (nestLayers == 0) {
											if (i + 1 < strings.size() && isOpeningBracketOrParen(strings[i + 1])) {
												++nestLayers;
												checkstr = strings[++i][0];
												endstr = checkstr == '[' ? ']' : ')';
											}
										}
									}
								}
								rootExpression->subexpressions.push_back(getExpression(std::move(minisub), scope, classs));
							}
							currExpression->subexpressions.push_back(root);
							root = prev;
						}
						else {
							rootExpression->subexpressions.push_back(curr);
						}
					}
				}
				else {
					static_cast<FunctionExpression*>(root)->subexpressions.push_back(curr);
				}
			}
			else {
				if (needsUnaryPlacementFix(strings, i)) {
					//auto rootExpression = static_cast<FunctionExpression*>(root);
					//rootExpression->subexpressions.insert(rootExpression->subexpressions.begin(),
					//	interpreter->arena.make<ValueNode>(std::make_shared<Value>(), root));
				}
			}
		}
		else if (isStringLiteral(strings[i])) {
			// trim quotation marks
			bool multiline = (strings[i].size() >= 6 && strings[i].substr(0, 3) == "\"\"\"");
			auto val = multiline ? std::string(strings[i].substr(3, strings[i].size() - 6)) 
								 : std::string(strings[i].substr(1, strings[i].size() - 2));
			replaceWhitespaceLiterals(val);
			auto newExpr = parseInterpolatedString(val, scope, classs);
			if (root) {
				static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
			}
			else {
				root = newExpr;
			}
		}
		else if (isCharLiteral(strings[i])) {
			auto val = std::string(strings[i].substr(1, strings[i].size() - 2));
			replaceWhitespaceLiterals(val);
			char32_t c = decodeCharLiteral(val);
			auto newExpr = interpreter->arena.make<ValueNode>(std::make_shared<Value>(c), ExpressionType::Value);
			if (root) {
				static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
			}
			else {
				root = newExpr;
			}
		}
		else if (strings[i] == "{") {
			int nestLayers = 1;
			std::vector<std::string_view> minisub;
			while (nestLayers > 0 && ++i < strings.size()) {
				if (strings[i] == "{") nestLayers++;
				else if (strings[i] == "}") nestLayers--;
				if (nestLayers > 0) minisub.push_back(strings[i]);
			}
			
			auto blockExpr = interpreter->arena.make<BlockExpression>();
            
			auto oldParseScope = parseScope;
			auto oldCurrentExpr = currentExpression;
			auto oldParseState = parseState;
			auto oldParseStrings = parseStrings;
            
			parseScope = interpreter->newScope("__anon"s, scope);
			currentExpression = blockExpr;
			clearParseStacks();
            
			for (auto& tok : minisub) {
				parse(tok);
			}
			if (!parseStrings.empty() || parseState != ParseState::BeginExpression) {
				parse(";");
			}
            
			interpreter->closeScope(parseScope);
            
			parseScope = oldParseScope;
			currentExpression = oldCurrentExpr;
			parseState = oldParseState;
			parseStrings = oldParseStrings;
			
			auto newExpr = blockExpr;
			if (root) {
				static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
			}
			else {
				root = newExpr;
			}
		}
		else if (strings[i] == "if" || strings[i] == "for" || strings[i] == "while" || strings[i] == "match") {
			auto oldParseScope = parseScope;
			auto oldCurrentExpr = currentExpression;
			auto oldParseState = parseState;
			auto oldParseStrings = parseStrings;
            
			currentExpression = nullptr;
			clearParseStacks();
			++getExpressionDepth;
			for (size_t j = i; j < strings.size(); ++j) {
				parse(strings[j]);
			}
			if (!parseStrings.empty() || parseState != ParseState::BeginExpression) {
				parse(";");
			}
            
			while (currentExpression) {
				closeCurrentExpression();
			}
			--getExpressionDepth;

			auto parsedStmt = previousExpression;
            
			parseScope = oldParseScope;
			currentExpression = oldCurrentExpr;
			parseState = oldParseState;
			parseStrings = oldParseStrings;
			
			auto newExpr = parsedStmt;
			if (root) {
				static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
			}
			else {
				root = newExpr;
			}
			break; // We consumed all remaining tokens for the statement!
		}
		else if (strings[i] == "(" || strings[i] == "[" || isVarOrFuncToken(strings[i])) {
			// Inline lambda: fun[...](args){body} or fun(args){body}
			if (strings[i] == "fun" && i + 1 < strings.size() &&
				(strings[i + 1] == "(" || strings[i + 1] == "[")) {
				// Collect all tokens that form this lambda definition until the matching closing "}"
				std::vector<std::string_view> lambdaTokens;
				lambdaTokens.push_back(strings[i]); // "fun"
				++i;
				// skip optional capture list [...]
				if (strings[i] == "[") {
					int depth = 1;
					lambdaTokens.push_back(strings[i]);
					while (depth > 0 && ++i < strings.size()) {
						if (strings[i] == "[") ++depth;
						else if (strings[i] == "]") --depth;
						lambdaTokens.push_back(strings[i]);
					}
					++i; // skip past "]"
				}
				// collect (args)
				{
					int depth = 1;
					lambdaTokens.push_back(strings[i]); // "("
					while (depth > 0 && ++i < strings.size()) {
						if (strings[i] == "(") ++depth;
						else if (strings[i] == ")") --depth;
						lambdaTokens.push_back(strings[i]);
					}
				}
				// collect return type annotation if present: skip optional ": Type"
				if (i + 2 < strings.size() && strings[i + 1] == ":") {
					lambdaTokens.push_back(strings[++i]); // ":"
					lambdaTokens.push_back(strings[++i]); // type name
				}
				// collect {body}
				if (i + 1 < strings.size() && strings[i + 1] == "{") {
					++i;
					int depth = 1;
					lambdaTokens.push_back(strings[i]); // "{"
					while (depth > 0 && ++i < strings.size()) {
						if (strings[i] == "{") ++depth;
						else if (strings[i] == "}") --depth;
						lambdaTokens.push_back(strings[i]);
					}
				}
				auto lambdaExpr = parseInlineLambda(std::move(lambdaTokens), scope);
				if (root) {
					static_cast<FunctionExpression*>(root)->subexpressions.push_back(lambdaExpr);
				} else {
					root = lambdaExpr;
				}
				++i;
				continue;
			}
		if (strings[i] == "(" || i + 2 < strings.size() && strings[i + 1] == "(") {
			// function
			ExpressionPtr cur = nullptr;
			if (strings[i] == "(") {
				// Detect tuple literal: (a, b, ...) at root level
				if (!root) {
					// Pre-scan for top-level commas
					int depth = 0;
					bool hasTupleComma = false;
					for (size_t j = i + 1; j < strings.size(); ++j) {
						if (strings[j] == "(" || strings[j] == "[") ++depth;
						else if (strings[j] == ")" || strings[j] == "]") {
							if (depth == 0) break;
							--depth;
						}
						else if (strings[j] == "," && depth == 0) {
							hasTupleComma = true;
							break;
						}
					}
					if (hasTupleComma) {
						// Build TupleLiteralExpression
						auto tupleExpr = interpreter->arena.make<TupleLiteralExpression>();
						root = tupleExpr;
						std::vector<std::string_view> minisub;
						int nestLayers = 1;
						while (nestLayers > 0 && ++i < strings.size()) {
							if (nestLayers == 1 && strings[i] == ",") {
								if (minisub.size()) {
									tupleExpr->elements.push_back(parseArg(std::move(minisub)));
									minisub.clear();
								}
							}
							else if (isClosingBracketOrParen(strings[i])) {
								if (--nestLayers > 0) { minisub.push_back(strings[i]); }
								else if (minisub.size()) {
									tupleExpr->elements.push_back(parseArg(std::move(minisub)));
									minisub.clear();
								}
							}
							else if (isOpeningBracketOrParen(strings[i])) { ++nestLayers; minisub.push_back(strings[i]); }
							else { minisub.push_back(strings[i]); }
						}
						++i;
						continue;
					}
				}
				if (root) {
						if (root->type == ExpressionType::FunctionCall) {
							if (static_cast<FunctionExpression*>(root)->function->getFunction()->opPrecedence == OperatorPrecedence::func) {
								cur = interpreter->arena.make<FunctionExpression>(std::make_shared<Value>());
								static_cast<FunctionExpression*>(cur)->subexpressions.push_back(root);
								root = cur;
							}
							else {
								static_cast<FunctionExpression*>(root)->subexpressions.push_back(interpreter->arena.make<FunctionExpression>(interpreter->identityFunctionVarLocation));
								cur = static_cast<FunctionExpression*>(root)->subexpressions.back();
							}
						}
						else {
							static_cast<MemberFunctionCall*>(root)->subexpressions.push_back(interpreter->arena.make<FunctionExpression>(interpreter->identityFunctionVarLocation));
							cur = static_cast<MemberFunctionCall*>(root)->subexpressions.back();
						}
					}
					else {
						root = interpreter->arena.make<FunctionExpression>(interpreter->identityFunctionVarLocation);
						cur = root;
					}
				}
				else {
					auto funccall = interpreter->arena.make<FunctionExpression>(std::make_shared<Value>(std::string(strings[i])));
					if (root) {
						auto t = root->type;
						static_cast<FunctionExpression*>(root)->subexpressions.push_back(funccall);
						cur = static_cast<FunctionExpression*>(root)->subexpressions.back();
					}
					else {
						root = funccall;
						cur = root;
					}
					++i;
				}
				std::vector<std::string_view> minisub;
				int nestLayers = 1;
				while (nestLayers > 0 && ++i < strings.size()) {
					if (nestLayers == 1 && strings[i] == ",") {
						if (minisub.size()) {
							static_cast<FunctionExpression*>(cur)->subexpressions.push_back(parseArg(std::move(minisub)));
							minisub.clear();
						}
					}
					else if (isClosingBracketOrParen(strings[i])) {
						if (--nestLayers > 0) {
							minisub.push_back(strings[i]);
						}
						else {
							if (minisub.size()) {
								static_cast<FunctionExpression*>(cur)->subexpressions.push_back(parseArg(std::move(minisub)));
								minisub.clear();
							}
						}
					}
					else if (isOpeningBracketOrParen(strings[i]) || !(strings[i].size() == 1 && contains("+-*%/"s, strings[i][0])) && i + 2 < strings.size() && strings[i + 1] == "(") {
						++nestLayers;
						if (strings[i] == "(") {
							minisub.push_back(strings[i]);
						}
						else {
							minisub.push_back(strings[i]);
							++i;
							if (isClosingBracketOrParen(strings[i])) {
								--nestLayers;
							}
							if (nestLayers > 0) {
								minisub.push_back(strings[i]);
							}
						}
					}
					else {
						minisub.push_back(strings[i]);
					}
				}

			}
			else if (strings[i] == "[" || i + 2 < strings.size() && strings[i + 1] == "[") {
				// list
				bool indexOfIndex = i > 0 && (isClosingBracketOrParen(strings[i - 1]) || strings[i - 1].back() == '\"') || (i > 1 && strings[i - 2] == ".");
				ExpressionPtr cur = nullptr;
				if (!indexOfIndex && strings[i] == "[") {
					// list literal / collection literal
					if (root) {
						static_cast<FunctionExpression*>(root)->subexpressions.push_back(
							interpreter->arena.make<ValueNode>(std::make_shared<Value>(List()), ExpressionType::Value));
						cur = static_cast<FunctionExpression*>(root)->subexpressions.back();
					}
					else {
						root = interpreter->arena.make<ValueNode>(std::make_shared<Value>(List()), ExpressionType::Value);
						cur = root;
					}
					std::vector<std::string_view> minisub;
					int nestLayers = 1;
					while (nestLayers > 0 && ++i < strings.size()) {
						if (nestLayers == 1 && strings[i] == ",") {
							if (minisub.size()) {
								auto val = *interpreter->getValue(std::move(minisub), scope, classs);
								static_cast<ValueNode*>(cur)->value->getList().push_back(std::make_shared<Value>(val));
								minisub.clear();
							}
						}
						else if (isClosingBracketOrParen(strings[i])) {
							if (--nestLayers > 0) {
								minisub.push_back(strings[i]);
							}
							else {
								if (minisub.size()) {
									auto val = *interpreter->getValue(std::move(minisub), scope, classs);
									static_cast<ValueNode*>(cur)->value->getList().push_back(std::make_shared<Value>(val));
									minisub.clear();
								}
							}
						}
						else if (isOpeningBracketOrParen(strings[i])) {
							++nestLayers;
							minisub.push_back(strings[i]);
						}
						else {
							minisub.push_back(strings[i]);
						}
					}
					auto& list = static_cast<ValueNode*>(cur)->value->getList();
					if (list.size()) {
						bool canBeArray = true;
						auto type = list[0]->getType();
						for (auto& val : list) {
							if (val->getType() == Type::Null || val->getType() != type || (int)val->getType() >= (int)Type::Array) {
								canBeArray = false;
								break;
							}
						}
						if (canBeArray) {
							static_cast<ValueNode*>(cur)->value->hardconvert(Type::Array);
						}
					}
				}
				else {
					// list access
					auto indexexpr = interpreter->arena.make<FunctionExpression>(interpreter->listIndexFunctionVarLocation);
					if (indexOfIndex) {
						cur = root;
						auto parent = root;
						while (cur->type == ExpressionType::FunctionCall
							&& static_cast<FunctionExpression*>(cur)->function->getType() == Type::Function
							&& static_cast<FunctionExpression*>(cur)->function->getFunction()->opPrecedence != OperatorPrecedence::func) {
							parent = cur;
							cur = static_cast<FunctionExpression*>(cur)->subexpressions.back();
						}
						static_cast<FunctionExpression*>(indexexpr)->subexpressions.push_back(cur);
						if (cur == root) {
							root = indexexpr;
							cur = indexexpr;
						}
						else {
							static_cast<FunctionExpression*>(parent)->subexpressions.pop_back();
							static_cast<FunctionExpression*>(parent)->subexpressions.push_back(indexexpr);
							cur = static_cast<FunctionExpression*>(parent)->subexpressions.back();
						}
					}
					else {
						if (root) {
							static_cast<FunctionExpression*>(root)->subexpressions.push_back(indexexpr);
							cur = static_cast<FunctionExpression*>(root)->subexpressions.back();
						}
						else {
							root = indexexpr;
							cur = root;
						}
						static_cast<FunctionExpression*>(cur)->subexpressions.push_back(getResolveVarExpression(std::string(strings[i]), parseScope->isClassScope));
						++i;
					}

					std::vector<std::string_view> minisub;
					int nestLayers = 1;
					while (nestLayers > 0 && ++i < strings.size()) {
						if (strings[i] == "]") {
							if (--nestLayers > 0) {
								minisub.push_back(strings[i]);
							}
							else {
								if (minisub.size()) {
									static_cast<FunctionExpression*>(cur)->subexpressions.push_back(parseArg(std::move(minisub)));
									minisub.clear();
								}
							}
						}
						else if (strings[i] == "[") {
							++nestLayers;
							minisub.push_back(strings[i]);
						}
						else {
							minisub.push_back(strings[i]);
						}
					}
				}
			}
			else {
				// variable
				ExpressionPtr newExpr;
				if (strings[i] == "true") {
					newExpr = interpreter->arena.make<ValueNode>(std::make_shared<Value>(true), ExpressionType::Value);
				}
				else if (strings[i] == "false") {
					newExpr = interpreter->arena.make<ValueNode>(std::make_shared<Value>(false), ExpressionType::Value);
				}
				else if (strings[i] == "null") {
					newExpr = interpreter->arena.make<ValueNode>(std::make_shared<Value>(), ExpressionType::Value);
				}
				else {
					newExpr = getResolveVarExpression(std::string(strings[i]), parseScope->isClassScope);
				}

				if (root) {
					if (root->type == ExpressionType::ResolveVar || root->type == ExpressionType::MemberVariable) {
						throw Exception("Syntax Error: unexpected series of values at "s + std::string(strings[i]) + ", possible missing `,`");
					}
					static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
				}
				else {
					root = newExpr;
				}
			}
		}
		else if (isMemberCall(strings[i])) {
			// member var
			bool isfunc = strings.size() > i + 2 && strings[i + 2] == "("s;
			if (isfunc) {
				ExpressionPtr expr;
				{
					//check null operator
					//if (strings[i] == "?" && ) {//TODO: check that function is null
					//	return root;
					//}
					if (root->type == ExpressionType::FunctionCall && static_cast<FunctionExpression*>(root)->subexpressions.size()) {
						expr = interpreter->arena.make<MemberFunctionCall>(static_cast<FunctionExpression*>(root)->subexpressions.back(), std::string(strings[++i]), std::vector<ExpressionPtr>());
						static_cast<FunctionExpression*>(root)->subexpressions.pop_back();
						static_cast<FunctionExpression*>(root)->subexpressions.push_back(expr);
					}
					else {
						expr = interpreter->arena.make<MemberFunctionCall>(root, std::string(strings[++i]), std::vector<ExpressionPtr>());
						root = expr;
					}
				}
				bool addedArgs = false;
				auto previ = i;
				++i;
				std::vector<std::string_view> minisub;
				int nestLayers = 1;
				while (nestLayers > 0 && ++i < strings.size()) {
					if (nestLayers == 1 && strings[i] == ",") {
						if (minisub.size()) {
							static_cast<MemberFunctionCall*>(expr)->subexpressions.push_back(parseArg(std::move(minisub)));
							minisub.clear();
							addedArgs = true;
						}
					}
					else if (strings[i] == ")") {
						if (--nestLayers > 0) {
							minisub.push_back(strings[i]);
						}
						else {
							if (minisub.size()) {
								static_cast<MemberFunctionCall*>(expr)->subexpressions.push_back(parseArg(std::move(minisub)));
								minisub.clear();
							}
							addedArgs = true; // closing ')' reached — set even with 0 args
						}
					}
					else if (strings[i] == "(") {
						++nestLayers;
						minisub.push_back(strings[i]);
					}
					else {
						minisub.push_back(strings[i]);
					}
				}
				if (!addedArgs) {
					i = previ;
				}
			}
			else {
				if (root->type == ExpressionType::FunctionCall && static_cast<FunctionExpression*>(root)->subexpressions.size()) {
					auto expr = interpreter->arena.make<MemberVariable>(static_cast<FunctionExpression*>(root)->subexpressions.back(), std::string(strings[++i]));
					static_cast<FunctionExpression*>(root)->subexpressions.pop_back();
					static_cast<FunctionExpression*>(root)->subexpressions.push_back(expr);
				}
				else {
					root = interpreter->arena.make<MemberVariable>(root, std::string(strings[++i]));
				}
			}
		}
		else {
			// number (supports decimal, 0x hex, 0b binary)
			auto val = parseNumericLiteral(strings[i]);
			bool isFloat = contains(strings[i], '.') && !isHexLiteral(strings[i]) && !isBinaryLiteral(strings[i]);
			auto newExpr = interpreter->arena.make<ValueNode>(ValuePtr(isFloat ? new Value((Float)val) : new Value((Int)val)), ExpressionType::Value);
			if (root) {
				static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
			}
			else {
				root = newExpr;
			}
		}
		++i;
	}

	return root;
}

bool isContainer(std::string_view type) {
	const static std::set<std::string> containers = { "Array", "List", "Map", "Set" };
	return containers.contains(std::string(type));
}

void Parser::parseType(TypeDescriptor& td) {
	auto typeName = std::string(parseStrings.front());
	parseStrings.erase(parseStrings.begin()); // type
	td.isNullable = false;
	if (checkNext(parseStrings, "?")) {
		td.isNullable = true;
	}

	auto desc = interpreter->checkTypeInScope(typeName, parseScope);
	td.isDynamic = false;
	td.type = desc.type;
	
	if (isContainer(td.type)) {
		parseContainerSubtype(td);
	}
}

void Parser::parseContainerSubtype(TypeDescriptor& td) {
	if (!checkNext(parseStrings, "<")) {
		return;
	}

	auto subTypeDescriptor = std::make_shared<TypeDescriptor>();
	td.subtype = subTypeDescriptor;

	auto _subtypeName = std::string(parseStrings.front());
	subTypeDescriptor->type = interpreter->checkTypeInScope(_subtypeName, parseScope).type;
	parseStrings.erase(parseStrings.begin()); // subtype
	
	parseType(*subTypeDescriptor);
	
	if (td.type == Type::Map) {
		expectNext(parseStrings, ",");
		td.subtype2 = std::make_shared<TypeDescriptor>();
		parseType(*td.subtype2.value());
	}
	expectNext(parseStrings, ">");
	td.isNullable = false;
	if (checkNext(parseStrings, "?")) {
		td.isNullable = true;
	}
}

// parse one token at a time, uses the state machine
void Parser::parse(std::string_view token) {
	auto tempState = parseState;
	switch (parseState) {
	case ParseState::BeginExpression:
	{
		if (lastStatementClosedScope && previousExpression) {
			// Skip evaluation if the upcoming token is part of an ongoing compound expression
			bool skipEval = (token == "else" || token == "if" || token == "case" || token == "default");
			// Inside getExpression's inner parse loop, skip evaluating a Match when its
			// outer closing '}' is seen — the match will be returned as an AST node to the caller
			if (!skipEval && token == "}" && getExpressionDepth > 0
				&& previousExpression->type == ExpressionType::Match) {
				skipEval = true;
			}
			if (!skipEval) {
				interpreter->getValue(previousExpression, parseScope, nullptr);
			}
			if (token == "if" && previousExpression->type == ExpressionType::IfElse) {
				interpreter->getValue(previousExpression, parseScope, nullptr);
			}
		}

		bool closedExpr = false;

		if (token == "@") {
			parseState = ParseState::Decorator;
		}
		else if (token == "fun") {
			parseState = ParseState::DefineFunc;
		}
		else if (token == "coro") {
			parseState = ParseState::DefineCoro;
		}
		else if (token == "operator") {
			parseState = ParseState::DefineOperator;
		}
		else if (token == "var") {
			parseState = ParseState::DefineVar;
		}
		else if (token == "const") {
			parseState = ParseState::DefineConst;
		}
		else if (token == "dynamic") {
			parseState = ParseState::DefineDynamic;
		}
		else if (token == "for" || token == "while") {
			parseState = ParseState::LoopCall;
			if (currentExpression) {
				auto newexpr = interpreter->arena.make<Loop>(currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			}
			else {
				currentExpression = interpreter->arena.make<Loop>();
			}
		}
		else if (token == "foreach") {
			parseState = ParseState::ForEach;
			if (currentExpression) {
				auto newexpr = interpreter->arena.make<Foreach>(currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			}
			else {
				currentExpression = interpreter->arena.make<Foreach>();
			}
		}
		else if (token == "if") {
			clearParseStacks();
			parseState = ParseState::IfCall;
			if (currentExpression) {
				auto dummy = std::vector<If>{};
				auto newexpr = interpreter->arena.make<IfElse>(dummy, currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			}
			else {
				currentExpression = interpreter->arena.make<IfElse>();
			}
		}
		else if (token == "else") {
			parseState = ParseState::ExpectIfEnd;
			currentExpression = previousExpression;
		}
		else if (token == "match") {
			clearParseStacks();
			parseState = ParseState::MatchCall;
			if (currentExpression) {
				auto newexpr = interpreter->arena.make<MatchExpression>(currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			}
			else {
				currentExpression = interpreter->arena.make<MatchExpression>();
			}
		}
		else if (token == "case") {
			// Restore match expression if between arms
			if (!currentExpression && previousExpression && previousExpression->type == ExpressionType::Match) {
				currentExpression = previousExpression;
			}
			if (currentExpression && currentExpression->type == ExpressionType::Match) {
				static_cast<MatchExpression*>(currentExpression)->arms.push_back(MatchArm{});
				parseState = ParseState::MatchCasePattern;
			}
		}
		else if (token == "default") {
			// Restore match expression if between arms
			if (!currentExpression && previousExpression && previousExpression->type == ExpressionType::Match) {
				currentExpression = previousExpression;
			}
			if (currentExpression && currentExpression->type == ExpressionType::Match) {
				static_cast<MatchExpression*>(currentExpression)->arms.push_back(MatchArm{});  // null pattern = default
				parseState = ParseState::MatchDefault;
			}
		}
		else if (token == "class") {
			parseState = ParseState::DefineClass;
		}
		else if (token == "{") {
			parseScope = interpreter->newScope("__anon"s, parseScope);
			clearParseStacks();
		}
		else if (token == "}") {
			closedExpr = closeCurrentExpression();
			if (!closedExpr || parseScope->name == "__anon") {
				interpreter->closeScope(parseScope);
			}
			if (previousExpression && previousExpression->type != ExpressionType::IfElse
				&& previousExpression->type != ExpressionType::Match) {
				closedExpr = false;
			}
		}
		else if (token == "return") {
			parseState = ParseState::ReturnLine;
		}
		else if (token == "yeld") {
			parseState = ParseState::YeldLine;
		}
		else if (token == "continue") {
			parseState = ParseState::ContinueLine;
		}
		else if (token == "break") {
			parseState = ParseState::BreakLine;
		}
		else if (token == "import") {
			parseState = ParseState::ImportModule;
		}
		else if (token == ";") {
			if (currentExpression) {
			} else {
			}
			clearParseStacks();
		}
		else {
			parseState = ParseState::ReadLine;
			parseStrings.push_back(token);
		}

		lastStatementClosedScope = closedExpr;
	}
	break;
	case ParseState::LoopCall:
		if (token == ")") {
			if (--outerNestLayer <= 0) {
				std::vector<std::vector<std::string_view>> exprs = {};
				exprs.push_back({});
				for (auto&& str : parseStrings) {
					if (str == ";") {
						exprs.push_back({});
					}
					else {
						exprs.back().push_back(str);
					}
				}
				auto loop = static_cast<Loop*>(currentExpression);
				bool isForeach = false;
				if (exprs.size() == 1) {
					auto it = std::find(exprs[0].begin(), exprs[0].end(), ":");
					if (it != exprs[0].end()) {
						isForeach = true;
						auto newexpr = interpreter->arena.make<Foreach>(loop->parent);
						
						std::vector<std::string_view> namesTokens(exprs[0].begin(), it);
						std::vector<std::string_view> listTokens(it + 1, exprs[0].end());
						for (auto& n : namesTokens) {
							if (n != ",") {
								newexpr->iterateNames.push_back(std::string(n));
							}
						}
						newexpr->listExpression = getExpression(listTokens, parseScope, nullptr);
						
						if (loop->parent) {
							loop->parent->replaceChild(loop, newexpr);
						} else {
							// if it's the root expression, just replace currentExpression
						}
						currentExpression = newexpr;
					}
				}

				if (!isForeach) {
					switch (exprs.size()) {
					case 1:
						loop->testExpression = getExpression(exprs[0], parseScope, nullptr);
						break;
					case 2:
						loop->testExpression = getExpression(exprs[0], parseScope, nullptr);
						loop->iterateExpression = getExpression(exprs[1], parseScope, nullptr);
						break;
					case 3:
					{
						if (!exprs[0].empty()) {
							auto name = exprs[0].front();
							exprs[0].erase(exprs[0].begin(), exprs[0].begin() + 2);
							loop->initExpression = interpreter->arena.make<DefineVar>(std::string(name), getExpression(exprs[0], parseScope, nullptr));
						}
						if (!exprs[1].empty()) {
							loop->testExpression = getExpression(exprs[1], parseScope, nullptr);
						}
						if (!exprs[2].empty()) {
							loop->iterateExpression = getExpression(exprs[2], parseScope, nullptr);
						}
					}
					break;
					default:
						break;
					}
				}

				clearParseStacks();
				outerNestLayer = 0;
			}
			else {
				parseStrings.push_back(token);
			}
		}
		else if (token == "(") {
			if (++outerNestLayer > 1) {
				parseStrings.push_back(token);
			}
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::ForEach:
		if (token == ")") {
			if (--outerNestLayer <= 0) {
				std::vector<std::vector<std::string_view>> exprs = {};
				exprs.push_back({});
				for (auto&& str : parseStrings) {
					if (str == ":") {
						exprs.push_back({});
					}
					else {
						exprs.back().push_back(move(str));
					}
				}
				if (exprs.size() != 2) {
					clearParseStacks();
					throw Exception("Syntax error, `foreach` requires 2 statements, "s + std::to_string(exprs.size()) + " statements supplied instead");
				}

				auto name = std::string(exprs[0][0]);
				interpreter->resolveVariable(name, parseScope);
				static_cast<Foreach*>(currentExpression)->iterateNames.push_back(move(name));
				static_cast<Foreach*>(currentExpression)->listExpression = getExpression(exprs[1], parseScope, nullptr);

				clearParseStacks();
				outerNestLayer = 0;
			}
			else {
				parseStrings.push_back(token);
			}
		}
		else if (token == "(") {
			if (++outerNestLayer > 1) {
				parseStrings.push_back(token);
			}
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::IfCall:
		if (token == ")") {
			if (--outerNestLayer <= 0) {
				static_cast<IfElse*>(currentExpression)->states.push_back(If());
				static_cast<IfElse*>(currentExpression)->states.back().testExpression = getExpression(move(parseStrings), parseScope, nullptr);
				clearParseStacks();
			}
			else {
				parseStrings.push_back(token);
			}
		}
		else if (token == "(") {
			if (++outerNestLayer > 1) {
				parseStrings.push_back(token);
			}
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::ReadLine:
		if (token == "{") {
			parseLambdaBrakets++;
			parseStrings.push_back(token);
		}
		else if (token == "}") {
			if (parseLambdaBrakets == 0) {
				// Implicit end-of-statement before '}': evaluate pending tokens then close scope
				if (!parseStrings.empty()) {
					auto line = move(parseStrings);
					clearParseStacks();
					if (!currentExpression) {
						interpreter->getValue(line, parseScope, nullptr);
					}
					else {
						currentExpression->push_back(getExpression(line, parseScope, nullptr));
					}
				}
				else {
					clearParseStacks();
				}
				// Now perform BeginExpression '}' handling inline (no recursion)
				parseState = ParseState::BeginExpression;
				bool closedExpr = closeCurrentExpression();
				if (!closedExpr || parseScope->name == "__anon") {
					interpreter->closeScope(parseScope);
				}
				if (previousExpression && previousExpression->type != ExpressionType::IfElse
					&& previousExpression->type != ExpressionType::Match) {
					closedExpr = false;
				}
				lastStatementClosedScope = closedExpr;
			}
			else {
				parseLambdaBrakets--;
				parseStrings.push_back(token);
			}
		}
		else if (token == ";" && parseLambdaBrakets == 0) {
			{//ternary operator
				if (parseStrings.size() > 2 && parseStrings[1] == "=") {
					auto f = std::find(parseStrings.begin(), parseStrings.end(), "if");
					if (f != parseStrings.end()) {
						auto varName = parseStrings[0];
						auto _f = std::find(parseStrings.begin(), parseStrings.end(), "return");
						while (_f != parseStrings.end()) {
							*_f = varName;
							_f++;
							parseStrings.insert(f, "=");
							_f = std::find(parseStrings.begin(), parseStrings.end(), "return");
						}
						parseStrings.erase(parseStrings.begin());//var
						parseStrings.erase(parseStrings.begin());//=

						for (auto& token : parseStrings) {
							parse(token);
						}
						break;
					}
				}
			}


			auto line = move(parseStrings);
			clearParseStacks();
			// we clear before evaluating lines so any exceptions can clear the offending code
			if (!currentExpression) {
				interpreter->getValue(line, parseScope, nullptr);
			}
			else {
				currentExpression->push_back(getExpression(line, parseScope, nullptr));
			}

		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::ReturnLine:
		if (token == ";") {
			if (currentExpression) {
				currentExpression->push_back(interpreter->arena.make<Return>(getExpression(move(parseStrings), parseScope, nullptr)));
			}
			clearParseStacks();
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::YeldLine:
		if (token == ";") {
			if (currentExpression) {
				currentExpression->push_back(interpreter->arena.make<Yeld>(getExpression(move(parseStrings), parseScope, nullptr)));
			}
			clearParseStacks();
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::ContinueLine:
		if (token == ";") {
			if (currentExpression) {
				currentExpression->push_back(interpreter->arena.make<Continue>());
			}
			clearParseStacks();
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::BreakLine:
		if (token == ";") {
			if (currentExpression) {
				currentExpression->push_back(interpreter->arena.make<Break>());
			}
			clearParseStacks();
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::ExpectIfEnd:
		if (token == "if") {
			clearParseStacks();
			parseState = ParseState::IfCall;
		}
		else if (token == "{") {
			parseScope = interpreter->newScope("__anon"s, parseScope);
			static_cast<IfElse*>(currentExpression)->states.push_back(If());
			clearParseStacks();
		}
		else {
			clearParseStacks();
			throw Exception("Malformed Syntax: Incorrect token `" + std::string(token) + "` following `else` keyword");
		}
		break;
	case ParseState::MatchCall:
		if (token == ")") {
			if (--outerNestLayer <= 0) {
				static_cast<MatchExpression*>(currentExpression)->target =
					getExpression(move(parseStrings), parseScope, nullptr);
				clearParseStacks();
			}
			else {
				parseStrings.push_back(token);
			}
		}
		else if (token == "(") {
			if (++outerNestLayer > 1) {
				parseStrings.push_back(token);
			}
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::MatchCasePattern:
		if (token == "=>") {
			if (!currentExpression || currentExpression->type != ExpressionType::Match) {
				throw Exception("MatchCasePattern: currentExpression is not Match");
			}
			auto* matchExpr = static_cast<MatchExpression*>(currentExpression);
			if (matchExpr->arms.empty()) {
				throw Exception("MatchCasePattern: no arms");
			}
			// Wildcard '_' pattern = same as default (null pattern)
			if (parseStrings.size() == 1 && parseStrings[0] == "_") {
				matchExpr->arms.back().pattern = nullptr;
			} else {
				matchExpr->arms.back().pattern = getExpression(move(parseStrings), parseScope, nullptr);
			}
			clearParseStacks();
			// Next token expected: '{' — handled by BeginExpression
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::MatchDefault:
		if (token == "=>") {
			clearParseStacks();
			// Next token expected: '{' — handled by BeginExpression
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::Decorator:
		pendingMetadata.push_back(Metadata{std::string(token)});
		parseState = ParseState::DecoratorArgs;
		break;
	case ParseState::DecoratorArgs:
		if (token == "(") {
			parseState = ParseState::DecoratorArgsList;
			parseStrings.clear();
		} else {
			parseState = ParseState::BeginExpression;
			parse(token);
		}
		break;
	case ParseState::DecoratorArgsList:
		if (token == ")") {
			auto& args = pendingMetadata.back().arguments;
			for (size_t i = 0; i < parseStrings.size(); ++i) {
				if (i + 2 < parseStrings.size() && parseStrings[i+1] == "=") {
					std::string key = std::string(parseStrings[i]);
					std::string valStr = std::string(parseStrings[i+2]);
					ValuePtr val = nullptr;
					if (valStr == "true") val = std::make_shared<Value>(true);
					else if (valStr == "false") val = std::make_shared<Value>(false);
					else if (valStr.size() >= 2 && valStr.front() == '"') val = std::make_shared<Value>(valStr.substr(1, valStr.size() - 2));
					else val = std::make_shared<Value>((Float)parseNumericLiteral(valStr));
					args.push_back({key, val});
					i += 2;
				} else if (parseStrings[i] != ",") {
					std::string valStr = std::string(parseStrings[i]);
					ValuePtr val = nullptr;
					if (valStr == "true") val = std::make_shared<Value>(true);
					else if (valStr == "false") val = std::make_shared<Value>(false);
					else if (valStr.size() >= 2 && valStr.front() == '"') val = std::make_shared<Value>(valStr.substr(1, valStr.size() - 2));
					else val = std::make_shared<Value>((Float)parseNumericLiteral(valStr));
					args.push_back({"", val});
				}
			}
			parseStrings.clear();
			parseState = ParseState::BeginExpression;
		} else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::DefineVar:
	case ParseState::DefineConst:
	case ParseState::DefineDynamic:
		if (token == "fun") {
			parseLambda = 1;
		}
		if (token == "{") {
			parseLambdaBrakets++;
		}
		if (token == "}") {
			parseLambdaBrakets--;
			if (parseLambdaBrakets == 0 && parseLambda == 1) {
				parseLambda = 2;
			}
		}
		if (parseLambdaBrakets == 0 && (parseLambda == 0 || parseLambda == 2) && token == ";") {
			if (parseStrings.size() == 0) {
				throw Exception("Malformed Syntax: `var`/`dynamic` keyword must be followed by user supplied name");
			}

			TypeDescriptor tDescriptor;
			tDescriptor.isConst = parseState == ParseState::DefineConst;
			tDescriptor.isDynamic = parseState == ParseState::DefineDynamic;

			auto name = parseStrings.front();
			ExpressionPtr defineExpr = nullptr;
			if (parseStrings.size() > 2) {
				parseStrings.erase(parseStrings.begin()); //name
				if (parseStrings[0] == ":") {//has type annotation
					parseStrings.erase(parseStrings.begin()); // : before type
					auto typeName = std::string(parseStrings[0].begin(), parseStrings[0].end());
					auto typeDesc = interpreter->checkTypeInScope(typeName, parseScope);
					if (typeDesc.isDynamic) {
						tDescriptor.isDynamic = true;
					} else {
						tDescriptor.type = typeDesc.type;
						tDescriptor.isDynamic = false;
					}
					parseStrings.erase(parseStrings.begin()); // type
					parseContainerSubtype(tDescriptor);
				}
				parseStrings.erase(parseStrings.begin()); // =
				// Only use parseInlineLambda when the ENTIRE RHS is a lambda (starts with "fun").
				// For nested lambdas like map(arr, fun(...){...}), use getExpression which
				// handles inline lambdas in arg position via the inline-lambda detector.
				if (parseLambda == 2 && !parseStrings.empty() && parseStrings[0] == "fun") {
					parseLambda = 0;
					defineExpr = parseInlineLambda(std::vector<std::string_view>(parseStrings.begin(), parseStrings.end()), parseScope);
				}
				else {
					parseLambda = 0;
					defineExpr = getExpression(std::move(parseStrings), parseScope, nullptr);
				}
			}
			if (currentExpression) {
				currentExpression->push_back(interpreter->arena.make<DefineVar>(std::string(name), defineExpr, tDescriptor));
			}
			else {
				auto resValue = interpreter->getValue(interpreter->arena.make<DefineVar>(std::string(name), defineExpr, tDescriptor), parseScope, nullptr);
				// resValue already has typeDescriptor applied by consolidated(); nothing extra needed here
			}
		if (pendingMetadata.size()) {
			parseScope->membersMetadata[std::string(name)] = pendingMetadata;
			pendingMetadata.clear();
		}
		clearParseStacks();
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::DefineClass:
		parseScope = interpreter->newClassScope(std::string(token), parseScope);
		if (pendingMetadata.size()) {
			parseScope->scopeMetadata = pendingMetadata;
			pendingMetadata.clear();
		}
		parseState = ParseState::ClassArgs;
		parseStrings.clear();
		break;
	case ParseState::ClassArgs:
		if (token == ",") {
			if (parseStrings.size()) {
				auto otherscope = interpreter->resolveScope(std::string(parseStrings.back()), parseScope);
				parseScope->variables.insert(otherscope->variables.begin(), otherscope->variables.end());
				parseScope->functions.insert(otherscope->functions.begin(), otherscope->functions.end());
				parseStrings.clear();
			}
		}
		else if (token == "{") {
			if (parseStrings.size()) {
				auto otherscope = interpreter->resolveScope(std::string(parseStrings.back()), parseScope);
				parseScope->variables.insert(otherscope->variables.begin(), otherscope->variables.end());
				parseScope->functions.insert(otherscope->functions.begin(), otherscope->functions.end());
			}
			clearParseStacks();
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::DefineOperator:
		parseStrings.push_back(token);
		parseState = ParseState::OperatorArgs;
		break;
	case ParseState::DefineFunc:
		parseStrings.push_back(token);
		parseState = ParseState::FuncArgs;
		break;
	case ParseState::DefineCoro:
		parseStrings.push_back(token);
		parseState = ParseState::CoroArgs;
		break;
	case ParseState::ImportModule:
		clearParseStacks();
		if (token.size() > 2 && token.front() == '\"' && token.back() == '\"') {
			// import file
			evaluateFile(std::string(token.substr(1, token.size() - 2)));
			clearParseStacks();
		}
		else {
			auto modName = std::string(token);
			auto iter = std::find_if(interpreter->modules.begin(), interpreter->modules.end(), [&modName](auto& mod) {return mod.scope->name == modName; });
			if (iter == interpreter->modules.end()) {
				auto newMod = interpreter->getOptionalModule(modName);
				if (newMod) {
					if (shouldAllow(interpreter->allowedModulePrivileges, newMod->requiredPermissions)) {
						interpreter->modules.emplace_back(newMod->requiredPermissions, newMod->scope);
					}
					else {
						throw Exception("Error: Cannot import restricted module: "s + modName);
					}
				}
			}
		}
		break;
	case ParseState::FuncArgs:
	case ParseState::OperatorArgs:
	case ParseState::CoroArgs:
		if (token == "(") {
			// skip opening paren
		}
		else if (token == ",") {
			parseStrings.push_back(token);
		}
		else if (token == ")") {
			enum class ParseFuncArgs {
				Arg,
				Type,
				Expression
			};

			auto fncName = parseStrings.front();
			parseStrings.erase(parseStrings.begin());

			std::vector<std::string> genericParams;
			if (!parseStrings.empty() && parseStrings.front() == "<") {
				parseStrings.erase(parseStrings.begin()); // remove '<'
				while (!parseStrings.empty() && parseStrings.front() != ">") {
					if (parseStrings.front() != ",") {
						genericParams.push_back(std::string(parseStrings.front()));
						// Register temporary generic scope
						auto genScope = std::make_shared<Scope>(interpreter);
						genScope->name = std::string(parseStrings.front());
						genScope->isGenericScope = true;
						parseScope->scopes[genScope->name] = genScope;
					}
					parseStrings.erase(parseStrings.begin());
				}
				if (!parseStrings.empty() && parseStrings.front() == ">") {
					parseStrings.erase(parseStrings.begin()); // remove '>'
				}
			}

			std::vector<std::string> args;
			std::map<std::string, TypeDescriptor> types;
			std::map<std::string, ExpressionPtr> defValues;
			bool isTyped = false;
			if (parseState == ParseState::OperatorArgs) {
				isTyped = true;
				//check input args count
			}
			bool hasDefValue = false;
			bool varArgsValue = false;
			//if (parseStrings.size() > 2 && parseStrings[1] == ":") { // first argument has type
			//	isTyped = true;
			//}
			ParseFuncArgs state = ParseFuncArgs::Arg;
			for (int i = 0; i < parseStrings.size();) {
			if (state == ParseFuncArgs::Arg) {
					// Skip comma separators
					if (parseStrings[i] == ",") {
						i++;
						continue;
					}
					if (varArgsValue) { //after variable nums of arg can not set enather var
						throw Exception("Malformed function argument syntax");
					}
					args.emplace_back(parseStrings[i]);
					i++;

					if (parseStrings.size() > i+2 && parseStrings[i] == "." && parseStrings[i+1] == "." && parseStrings[+2] == ".") {
						if (parseState == ParseState::OperatorArgs) {
							throw Exception("Expected " + std::string(expect) + " but got " + std::string(vec.front()));
						}
						//variable number of arguments
						i += 3;
						varArgsValue = true;
					}

					if (isTyped && parseStrings.size() > i && parseStrings[i] != ":") {
						throw Exception("Malformed function argument syntax");
					}
					if (!isTyped && parseStrings.size() > i && parseStrings[i] == ":") {
						if (args.size() == 1) { //function is typed
							isTyped = true;
						}
						else {
							throw Exception("Malformed function argument syntax");
						}
					}
					if (isTyped) {
						i++;
						state = ParseFuncArgs::Type;
					}
					else {
						if (hasDefValue && parseStrings.size() > i && parseStrings[i] != "=") {
							throw Exception("Malformed function argument syntax");
						}
						if (parseStrings.size() > i && parseStrings[i] == "=") {
							hasDefValue = true;
							i++;
						}
						if (hasDefValue) {
							state = ParseFuncArgs::Expression;
						}
					}
				}
				else if (state == ParseFuncArgs::Type) {
					auto type = std::string(parseStrings[i]);
					types[args.back()] = interpreter->checkTypeInScope(type, parseScope);
					i++;
					// Skip comma separator
					if (parseStrings.size() > i && parseStrings[i] == ",") {
						if (hasDefValue) {
							throw Exception("Malformed function argument syntax"); // After default values, all remaining args must have defaults
						}
						i++;
						state = ParseFuncArgs::Arg;
					}
					else if (hasDefValue && parseStrings.size() > i && parseStrings[i] != "=") {
						throw Exception("Malformed function argument syntax");
					}
					else if (parseStrings.size() > i && parseStrings[i] == "=") {
						if (parseState == ParseState::OperatorArgs) {
							throw Exception("Malformed function argument syntax");
						}
						hasDefValue = true;
						i++;
						state = ParseFuncArgs::Expression;
					} else {
                        state = ParseFuncArgs::Arg;
                    }
				}
				else if (state == ParseFuncArgs::Expression) {
					std::vector<std::string_view> exprs = {};
					while (i < parseStrings.size() && parseStrings[i] != ",") {
						exprs.push_back(parseStrings[i]);
						++i;
					}
					defValues[args.back()] = getExpression(exprs, parseScope, nullptr);
					// Skip comma separator if present
					if (i < parseStrings.size() && parseStrings[i] == ",") {
						++i;
					}
					state = ParseFuncArgs::Arg;
				}
			}
			auto isConstructor = parseScope->isClassScope && parseScope->name == fncName;
			bool isOperator = parseState == ParseState::OperatorArgs;
			if (isOperator) {
				auto funcType = Function::getPrecedence(std::string(fncName));
				auto argsSize = 0;
				switch (funcType)
				{
				case OperatorPrecedence::boolean:;
				case OperatorPrecedence::compare:;
				case OperatorPrecedence::range:;
				case OperatorPrecedence::addsub:;
				case OperatorPrecedence::muldiv: { argsSize = 2; break; };
				case OperatorPrecedence::assign:;
				case OperatorPrecedence::incdec: { argsSize = 1;  break; }
				case OperatorPrecedence::func: { throw Exception("Malformed function argument syntax"); }
				default: { throw Exception("Malformed function argument syntax"); }
				}
				if (argsSize != args.size()) {
					throw Exception("Malformed function argument syntax");
				}
			}
			auto newfunc = isConstructor ? 
				interpreter->newConstructor(std::string(fncName), parseScope->parent, args, types, defValues) :
				(isOperator ? 
					interpreter->newOperator(std::string(fncName), parseScope, args, types, defValues) : 
					interpreter->newFunction(std::string(fncName), parseScope, args, types, defValues));
			
			newfunc->genericParams = genericParams;

			if (parseState == ParseState::CoroArgs) {
				if (isConstructor || isOperator) {
					throw Exception("Malformed Syntax: coroutine cannot be a constructor or operator");
				}
				newfunc->isCoro = true;
			}
			if (currentExpression) {
				auto newexpr = interpreter->arena.make<FunctionExpression>(newfunc, currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			}
			else {
				currentExpression = interpreter->arena.make<FunctionExpression>(newfunc, nullptr);
			}
			if (varArgsValue) {
				auto funcExpr = static_cast<FunctionExpression*>(currentExpression);
				funcExpr->function->getFunction()->variableArgsParam = varArgsValue;
				if (isTyped) {
					funcExpr->function->getFunction()->variableArgsParamType = types[args.back()];
					types[args.back()] = {Type::List};
				}

			}
			if (pendingMetadata.size()) {
				parseScope->membersMetadata[std::string(fncName)] = pendingMetadata;
				pendingMetadata.clear();
			}
			parseStrings.clear();
			if (isTyped) {
				parseState = ParseState::FuncResult;
			}
		}
		else if (token == "{") {
			clearParseStacks();
		}
		else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::FuncResult:
		{
			if (token == "{") {
				if (parseStrings.empty()) {
					throw Exception("Malformed Syntax: expected return type annotation before `{` in function definition");
				}
				if (parseStrings[0] == ":") {
					auto resType = std::string(parseStrings[1]); // type
					auto desc = interpreter->checkTypeInScope(resType, parseScope);
					static_cast<FunctionExpression*>(currentExpression)->function->getFunction()->returnType = desc;
				}
				else {
					throw Exception("Malformed Syntax: expected `:` before return type, got `" + std::string(parseStrings[0]) + "`");
				}
				
				auto fnc = static_cast<FunctionExpression*>(currentExpression)->function->getFunction();
				if (fnc && !fnc->genericParams.empty()) {
					parseState = ParseState::GenericBody;
					genericBodyNesting = 1;
				} else {
					clearParseStacks();
				}
				break;
			}
			parseStrings.push_back(token);
			break;
		}
	case ParseState::GenericBody:
		{
			auto fnc = static_cast<FunctionExpression*>(currentExpression)->function->getFunction();
			if (token == "{") {
				genericBodyNesting++;
			} else if (token == "}") {
				genericBodyNesting--;
				if (genericBodyNesting == 0) {
					clearParseStacks();
					closeCurrentExpression();
					break;
				}
			}
			fnc->genericBodyRaw += std::string(token) + " ";
			break;
		}
	default:
		break;
	}

	prevState = tempState;
}

bool Parser::readLine(std::string_view text) {
	++IkigaiScriptInterpreter::currentLine;
	auto tokenCount = 0;
	auto tokens = Lexer::Tokenize(text);
	{
		//auto tokens = Tokenizer(text);
		//for (auto& t : tokens) {
		//	std::cout << "TOKEN " << (int)t.type << " " << t.token << std::endl;
		//}
	}
	bool didExcept = false;
	try {
		for (auto& token : tokens) {
			parse(token);
			++tokenCount;
		}
	}
	catch (Exception e) {
		interpreter->__EXEPTION__ = e.type;
#if defined SCRIPT_DO_INTERNAL_PRINT
		interpreter->callFunctionWithArgs(interpreter->resolveFunction("print"), "Error at line "s + std::to_string(IkigaiScriptInterpreter::currentLine) + ", at: " + std::to_string(tokenCount) + string(tokens[tokenCount]) + ": " + e.errStr + "\n");
#else 
		//printf("Error at line %llu at %i: %s : %s\n", IkigaiScriptInterpreter::currentLine, tokenCount, std::string(tokens[tokenCount]).c_str(), e.errStr.c_str());
#endif		

		clearParseStacks();
		parseScope = interpreter->globalScope;
		currentExpression = nullptr;
		didExcept = true;

	}
	catch (std::exception& e) {
#if defined SCRIPT_DO_INTERNAL_PRINT
		interpreter->callFunctionWithArgs(interpreter->resolveFunction("print"), "Error at line "s + std::to_string(IkigaiScriptInterpreter::currentLine) + ", at: " + std::to_string(tokenCount) + string(tokens[tokenCount]) + ": " + e.what() + "\n");
#else
		//printf("Error at line %llu at %i: %s : %s\n", IkigaiScriptInterpreter::currentLine, tokenCount, std::string(tokens[tokenCount]).c_str(), e.what());
#endif		
		clearParseStacks();
		parseScope = interpreter->globalScope;
		currentExpression = nullptr;
		didExcept = true;
	}
	return didExcept;
}


bool Parser::evaluate(std::string_view script) {
	if (readLine(script)) {
		return true;
	}
	// close any dangling if-expressions that may exist
	return readLine(";"s);
}

bool Parser::evaluateFile(const std::string& path) {
	std::string s;
	auto file = std::ifstream(path);
	if (file) {
		file.seekg(0, std::ios::end);
		s.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		// bash scripts have a header line we need to skip
		if (endswith(path, ".sh")) {
			getline(file, s);
		}
		s.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return evaluate(s);
	}
	else {
		printf("file: %s not found\n", path.c_str());
		return 1;
	}
}

bool Parser::readLine(std::string_view text, ScopePtr scope) {
	auto temp = parseScope;
	parseScope = scope;
	auto result = readLine(text);
	parseScope = temp;
	return result;
}

bool Parser::evaluate(std::string_view script, ScopePtr scope) {
	auto temp = parseScope;
	parseScope = scope;
	auto result = evaluate(script);
	parseScope = temp;
	return result;
}

bool Parser::evaluateFile(const std::string& path, ScopePtr scope) {
	auto temp = parseScope;
	parseScope = scope;
	auto result = evaluateFile(path);
	parseScope = temp;
	return result;
}

void ClearScope(ScopePtr s) {
	s->parent = nullptr;
	for (auto&& c : s->scopes) {
		ClearScope(c.second);
	}
	s->scopes.clear();
}

void Parser::clearState() {
	clearParseStacks();
	ClearScope(interpreter->globalScope);
	interpreter->globalScope = std::make_shared<Scope>(interpreter);
	parseScope = interpreter->globalScope;
	currentExpression = nullptr;
	if (interpreter->modules.size() > 1) {
		interpreter->modules.erase(interpreter->modules.begin() + 1, interpreter->modules.end());
	}
}

// general purpose clear to reset state machine for next statement
void Parser::clearParseStacks() {
	parseState = ParseState::BeginExpression;
	parseStrings.clear();
	outerNestLayer = 0;
	parseLambdaBrakets = 0;
	parseLambda = 0;
}

bool Parser::closeCurrentExpression() {
	if (currentExpression) {
		previousExpression = currentExpression;
		if (currentExpression->parent) {
			currentExpression = currentExpression->parent;
		}
		else {
			if (currentExpression->type != ExpressionType::FunctionDef
				&& currentExpression->type != ExpressionType::IfElse
				&& currentExpression->type != ExpressionType::Match
				) {
				interpreter->getValue(currentExpression, parseScope, nullptr);
			}
			currentExpression = nullptr;
			return true;
		}
	}
	return false;
}

}
