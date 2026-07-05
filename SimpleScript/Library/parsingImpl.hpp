#pragma once

#include <fstream>

#include "ikigaiScript.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "exception.h"
#include "expressions.hpp"

using namespace IkigaiScript;

std::set<char> operator+(const std::set<char>& lhs, const std::set<char>& rhs) {
	std::set<char> res;
	res.insert(lhs.begin(), lhs.end());
	res.insert(rhs.begin(), rhs.end());
	return res;
}

inline size_t findFirstOf(std::string_view input, size_t pos, const std::set<char>& pattern) {
	while (pos < input.size()) {
		if (pattern.contains(input[pos])) {
			return pos;
		}
		pos++;
	}
	return std::string::npos;
}

const std::set<char> WhitespaceChars = {' ', '\t', '\n'};
const std::set<char> SpecialChars = { ',', '.', '(', ')', '{', '}', '[', ']', ';', '+', '-', '/', '*', '%', '<', '>', '=', '!', '&', '|', '"' };
const std::set<char> GrammarChars = WhitespaceChars + SpecialChars;
const std::set<char> MultiCharTokenStartChars = { '+', '-', '/', '*', '<', '>', '=', '!', '&', '|' };
const std::set<char> NumChars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'e', 'E', 'x', 'X', 'b', 'B',
	'A', 'C', 'D', 'F', 'a', 'c', 'd', 'f', '+', '-'};
const std::set<char> NumStartChars = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
const std::set<char> DisallowedIdentifierStartChars = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '\t', '\n', ',', '.',
	'(', ')', '{', '}', '[', ']', ';', '+', '-', '/', '*', '%', '<', '>', '=', '!', '&', '|', '"'};


std::vector<std::string_view> Tokenize(std::string_view input) {
	std::vector<std::string_view> ret;
	if (input.empty()) {
		return ret;
	}
	bool exitFromComment = false;

	size_t pos = 0;
	size_t lpos = 0;
	while ((pos = findFirstOf(input, lpos, GrammarChars)) != std::string::npos) {
		size_t len = pos - lpos;
		// Handle range operators '..' and '..=' before decimal check
		if (input[pos] == '.' && pos + 1 < input.size() && input[pos + 1] == '.') {
			if (len) {
				ret.push_back(input.substr(lpos, len));
			}
			int stride = 2;
			if (pos + 2 < input.size() && input[pos + 2] == '=') stride = 3;
			ret.push_back(input.substr(pos, stride));
			lpos = pos + stride;
			continue;
		}
		// differentiate between decimals and dot syntax for member access / function calls
		// Only treat '.<digit>' as decimal continuation if the char before '.' is also a digit (not an identifier)
		if (input[pos] == '.' && pos + 1 < input.size() && contains(NumChars, input[pos + 1])
			&& (pos == 0 || isNumChar(input[pos - 1]))) {
			pos = findFirstOf(input, pos +  1, GrammarChars);
			ret.push_back(input.substr(lpos, pos - lpos));
			lpos = pos;
			continue;
		}
		if (len) {
			ret.push_back(input.substr(lpos, pos - lpos));
			lpos = pos;
		} else {
			// handle strings and escaped strings
			if (input[pos] == '\"' && pos > 0 && input[pos - 1] != '\\') {
				auto originalPos = pos;
				auto testpos = lpos + 1;
				bool loop = true;
				while (loop) {
					pos = input.find('\"', testpos);
					if (pos == std::string::npos) {
						throw Exception("Quote mismatch at "s + std::string(input.substr(lpos, input.size() - lpos)));
					}
					loop = (input[pos - 1] == '\\');
					testpos = ++pos;
				}

				ret.push_back(input.substr(originalPos, pos - originalPos));
				lpos = pos;
				continue;
			}
		}
		// special case for negative numbers
		if ((input[pos] == '-' || input[pos] == '+') && contains(NumChars, input[pos + 1])
			&& (ret.size() == 0 || contains(MultiCharTokenStartChars, ret.back().back()))) {
			pos = findFirstOf(input, pos + 1, GrammarChars);
			if ((input[pos] == '.' && pos + 1 < input.size() && contains(NumChars, input[pos + 1])) ||
				((input[pos] == '+' || input[pos] == '-') && pos + 1 < input.size() && contains(NumChars, input[pos + 1]))) {
				pos = findFirstOf(input, pos + 1, GrammarChars);
			}
			ret.push_back(input.substr(lpos, pos - lpos));
			lpos = pos;
		} else if (!contains(WhitespaceChars, input[pos])) {
			// process multicharacter special tokens like ++, //, -=, etc
			auto stride = 1;
			if (contains(MultiCharTokenStartChars, input[pos]) && pos + 1 < input.size() && contains(MultiCharTokenStartChars, input[pos + 1])) {
				if (input[pos] == '/' && input[pos + 1] == '/') {
					exitFromComment = true;
					break;
				}
				++stride;
			}
			ret.push_back(input.substr(lpos, stride));
			lpos += stride;
		} else {
			++lpos;
		}
	}
	if (!exitFromComment && lpos < input.length()) {
		ret.push_back(input.substr(lpos, input.length()));
	}
	return ret;
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
		return contains(MultiCharTokenStartChars, test[0]) && contains("=+-&|"s, test[1]);
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
		return '.' == test[0];
	}
	return false;
}

bool isOpeningBracketOrParen(std::string_view test) {
	return (test.size() == 1 && (test[0] == '[' || test[0] == '('));
}

bool isClosingBracketOrParen(std::string_view test) {
	return (test.size() == 1 && (test[0] == ']' || test[0] == ')'));
}

bool needsUnaryPlacementFix(const std::vector<std::string_view>& strings, size_t i) {
	return (isUnaryMathOperator(strings[i]) && (i == 0 || !(isClosingBracketOrParen(strings[i - 1]) || isVarOrFuncToken(strings[i - 1]) || isNumeric(strings[i - 1]))));
}

bool checkPrecedence(OperatorPrecedence curr, OperatorPrecedence neww) {
	return (int)curr < (int)neww || (neww == curr && neww == OperatorPrecedence::incdec);
}

ExpressionPtr IkigaiScriptInterpreter::getResolveVarExpression(const std::string& name, bool classScope) {
	if (classScope) {
		return arena.make<MemberVariable>(nullptr, name);
	}
	return arena.make<ResolveVar>(name);
}

ExpressionPtr IkigaiScriptInterpreter::parseInterpolatedString(std::string val, ScopePtr scope, Class* classs) {
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
		return arena.make<ValueNode>(std::make_shared<Value>(val), ExpressionType::Value);
	}

	std::string prefix = val.substr(0, pos);
	ExpressionPtr finalExpr = nullptr;
	if (!prefix.empty()) {
		finalExpr = arena.make<ValueNode>(std::make_shared<Value>(prefix), ExpressionType::Value);
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
		auto addExpr = arena.make<FunctionExpression>(resolveVariable("+"));
		addExpr->subexpressions.push_back(finalExpr);
		addExpr->subexpressions.push_back(innerExpr);
		finalExpr = addExpr;
	} else {
		finalExpr = innerExpr;
	}

	if (!suffix.empty()) {
		auto suffixExpr = parseInterpolatedString(suffix, scope, classs);
		auto addExpr = arena.make<FunctionExpression>(resolveVariable("+"));
		addExpr->subexpressions.push_back(finalExpr);
		addExpr->subexpressions.push_back(suffixExpr);
		finalExpr = addExpr;
	}

	return finalExpr;
}

// recursively build an expression tree from a list of tokens
ExpressionPtr IkigaiScriptInterpreter::getExpression(const std::vector<std::string_view>& strings, ScopePtr scope, Class* classs) {
	ExpressionPtr root = nullptr;
	size_t i = 0;
	while (i < strings.size()) {
		if (isMathOperator(strings[i])) {
			auto prev = root;
			root = arena.make<FunctionExpression>(resolveVariable(std::string(strings[i]), modules[0].scope));
			auto curr = prev;
			if (curr) {
				// find operations of lesser precedence
				if (curr->type == ExpressionType::FunctionCall) {
					auto rootExpression = static_cast<FunctionExpression*>(root);
					auto funcExpr = static_cast<FunctionExpression*>(curr)->function;
					if (funcExpr->getType() != Type::Function) {
						rootExpression->subexpressions.push_back(curr);
					} else {
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
								} else {
									break;
								}
							}
							auto currExpression = static_cast<FunctionExpression*>(curr);
							// swap values around to correct the otherwise incorect order of operations (except unary)
							if (needsUnaryPlacementFix(strings, i)) {
								rootExpression->subexpressions.insert(rootExpression->subexpressions.begin(),
									arena.make<ValueNode>(std::make_shared<Value>(), root));
							} else {
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
											} else if (strings[i][0] == checkstr) {
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
								rootExpression->subexpressions.push_back(getExpression(move(minisub), scope, classs));
							}
							currExpression->subexpressions.push_back(root);
							root = prev;
						} else {
							rootExpression->subexpressions.push_back(curr);
						}
					}
				} else {
					static_cast<FunctionExpression*>(root)->subexpressions.push_back(curr);
				}
			} else {
				if (needsUnaryPlacementFix(strings, i)) {
					auto rootExpression = static_cast<FunctionExpression*>(root);
					rootExpression->subexpressions.insert(rootExpression->subexpressions.begin(),
						arena.make<ValueNode>(std::make_shared<Value>(), root));
				}
			}
		} else if (isStringLiteral(strings[i])) {
			// trim quotation marks
			bool multiline = (strings[i].size() >= 6 && strings[i].substr(0, 3) == "\"\"\"");
			auto val = multiline ? std::string(strings[i].substr(3, strings[i].size() - 6)) 
								 : std::string(strings[i].substr(1, strings[i].size() - 2));
			replaceWhitespaceLiterals(val);
			auto newExpr = parseInterpolatedString(val, scope, classs);
			if (root) {
				static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
			} else {
				root = newExpr;
			}
		} else if (isCharLiteral(strings[i])) {
			auto val = std::string(strings[i].substr(1, strings[i].size() - 2));
			replaceWhitespaceLiterals(val);
			char32_t c = val.empty() ? 0 : val[0]; // simple fallback
			auto newExpr = arena.make<ValueNode>(std::make_shared<Value>(c), ExpressionType::Value);
			if (root) {
				static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
			} else {
				root = newExpr;
			}
		} else if (strings[i] == "(" || strings[i] == "[" || isVarOrFuncToken(strings[i])) {
			if (strings[i] == "(" || i + 2 < strings.size() && strings[i + 1] == "(") {
				// function
				ExpressionPtr cur = nullptr;
				if (strings[i] == "(") {
					if (root) {
						if (root->type == ExpressionType::FunctionCall) {
							if (static_cast<FunctionExpression*>(root)->function->getFunction()->opPrecedence == OperatorPrecedence::func) {
								cur = arena.make<FunctionExpression>(std::make_shared<Value>());
								static_cast<FunctionExpression*>(cur)->subexpressions.push_back(root);
								root = cur;
							} else {
								static_cast<FunctionExpression*>(root)->subexpressions.push_back(arena.make<FunctionExpression>(identityFunctionVarLocation));
								cur = static_cast<FunctionExpression*>(root)->subexpressions.back();
							}
						} else {
							static_cast<MemberFunctionCall*>(root)->subexpressions.push_back(arena.make<FunctionExpression>(identityFunctionVarLocation));
							cur = static_cast<MemberFunctionCall*>(root)->subexpressions.back();
						}
					} else {
						root = arena.make<FunctionExpression>(identityFunctionVarLocation);
						cur = root;
					}
				} else {
					auto funccall = arena.make<FunctionExpression>(std::make_shared<Value>(std::string(strings[i])));
					if (root) {
						auto t = root->type;
						static_cast<FunctionExpression*>(root)->subexpressions.push_back(funccall);
						cur = static_cast<FunctionExpression*>(root)->subexpressions.back();
					} else {
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
							static_cast<FunctionExpression*>(cur)->subexpressions.push_back(getExpression(move(minisub), scope, classs));
							minisub.clear();
						}
					} else if (isClosingBracketOrParen(strings[i])) {
						if (--nestLayers > 0) {
							minisub.push_back(strings[i]);
						} else {
							if (minisub.size()) {
								static_cast<FunctionExpression*>(cur)->subexpressions.push_back(getExpression(move(minisub), scope, classs));
								minisub.clear();
							}
						}
					} else if (isOpeningBracketOrParen(strings[i]) || !(strings[i].size() == 1 && contains("+-*%/"s, strings[i][0])) && i + 2 < strings.size() && strings[i + 1] == "(") {
						++nestLayers;
						if (strings[i] == "(") {
							minisub.push_back(strings[i]);
						} else {
							minisub.push_back(strings[i]);
							++i;
							if (isClosingBracketOrParen(strings[i])) {
								--nestLayers;
							}
							if (nestLayers > 0) {
								minisub.push_back(strings[i]);
							}
						}
					} else {
						minisub.push_back(strings[i]);
					}
				}

			} else if (strings[i] == "[" || i + 2 < strings.size() && strings[i + 1] == "[") {
				// list
				bool indexOfIndex = i > 0 && (isClosingBracketOrParen(strings[i-1]) || strings[i - 1].back() == '\"') || (i > 1 && strings[i - 2] == ".");
				ExpressionPtr cur = nullptr;
				if (!indexOfIndex && strings[i] == "[") {
					// list literal / collection literal
					if (root) {
						static_cast<FunctionExpression*>(root)->subexpressions.push_back(
							arena.make<ValueNode>(std::make_shared<Value>(List()), ExpressionType::Value));
						cur = static_cast<FunctionExpression*>(root)->subexpressions.back();
					} else {
						root = arena.make<ValueNode>(std::make_shared<Value>(List()), ExpressionType::Value);
						cur = root;
					}
					std::vector<std::string_view> minisub;
					int nestLayers = 1;
					while (nestLayers > 0 && ++i < strings.size()) {
						if (nestLayers == 1 && strings[i] == ",") {
							if (minisub.size()) {
								auto val = *getValue(std::move(minisub), scope, classs);
								static_cast<ValueNode*>(cur)->value->getList().push_back(std::make_shared<Value>(val));
								minisub.clear();
							}
						} else if (isClosingBracketOrParen(strings[i])) {
							if (--nestLayers > 0) {
								minisub.push_back(strings[i]);
							} else {
								if (minisub.size()) {
									auto val = *getValue(std::move(minisub), scope, classs);
									static_cast<ValueNode*>(cur)->value->getList().push_back(std::make_shared<Value>(val));
									minisub.clear();
								}
							}
						} else if (isOpeningBracketOrParen(strings[i])) {
							++nestLayers;
							minisub.push_back(strings[i]);
						} else {
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
				} else {
					// list access
					auto indexexpr = arena.make<FunctionExpression>(listIndexFunctionVarLocation);
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
						} else {
							static_cast<FunctionExpression*>(parent)->subexpressions.pop_back();
							static_cast<FunctionExpression*>(parent)->subexpressions.push_back(indexexpr);
							cur = static_cast<FunctionExpression*>(parent)->subexpressions.back();
						}
					} else {
						if (root) {
							static_cast<FunctionExpression*>(root)->subexpressions.push_back(indexexpr);
							cur = static_cast<FunctionExpression*>(root)->subexpressions.back();
						} else {
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
							} else {
								if (minisub.size()) {
									static_cast<FunctionExpression*>(cur)->subexpressions.push_back(getExpression(move(minisub), scope, classs));
									minisub.clear();
								}
							}
						} else if (strings[i] == "[") {
							++nestLayers;
							minisub.push_back(strings[i]);
						} else {
							minisub.push_back(strings[i]);
						}
					}
				}
			} else {
				// variable
				ExpressionPtr newExpr;
				if (strings[i] == "true") {
					newExpr = arena.make<ValueNode>(std::make_shared<Value>(Int(1)), ExpressionType::Value);
				} else if (strings[i] == "false") {
					newExpr = arena.make<ValueNode>(std::make_shared<Value>(Int(0)), ExpressionType::Value);
				} else if (strings[i] == "null") {
					newExpr = arena.make<ValueNode>(std::make_shared<Value>(), ExpressionType::Value);
				} else {
					newExpr = getResolveVarExpression(std::string(strings[i]), parseScope->isClassScope);
				}

				if (root) {
					if (root->type == ExpressionType::ResolveVar || root->type == ExpressionType::MemberVariable) {
						throw Exception("Syntax Error: unexpected series of values at "s + std::string(strings[i]) +", possible missing `,`");
					}
					static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
				} else {
					root = newExpr;
				}
			}
		} else if (isMemberCall(strings[i])) {
			// member var
			bool isfunc = strings.size() > i + 2 && strings[i + 2] == "("s;
			if (isfunc) {
				ExpressionPtr expr;
				{
					if (root->type == ExpressionType::FunctionCall && static_cast<FunctionExpression*>(root)->subexpressions.size()) {
						expr = arena.make<MemberFunctionCall>(static_cast<FunctionExpression*>(root)->subexpressions.back(), std::string(strings[++i]), std::vector<ExpressionPtr>());
						static_cast<FunctionExpression*>(root)->subexpressions.pop_back();
						static_cast<FunctionExpression*>(root)->subexpressions.push_back(expr);
					} else {
						expr = arena.make<MemberFunctionCall>(root, std::string(strings[++i]), std::vector<ExpressionPtr>());
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
							static_cast<MemberFunctionCall*>(expr)->subexpressions.push_back(getExpression(move(minisub), scope, classs));
							minisub.clear();
							addedArgs = true;
						}
					} else if (strings[i] == ")") {
						if (--nestLayers > 0) {
							minisub.push_back(strings[i]);
						} else {
							if (minisub.size()) {
								static_cast<MemberFunctionCall*>(expr)->subexpressions.push_back(getExpression(move(minisub), scope, classs));
								minisub.clear();
								addedArgs = true;
							}
						}
					} else if (strings[i] == "(") {
						++nestLayers;
						minisub.push_back(strings[i]);
					} else {
						minisub.push_back(strings[i]);
					}
				}
				if (!addedArgs) {
					i = previ;
				}
			} else {
				if (root->type == ExpressionType::FunctionCall && static_cast<FunctionExpression*>(root)->subexpressions.size()) {
					auto expr = arena.make<MemberVariable>(static_cast<FunctionExpression*>(root)->subexpressions.back(), std::string(strings[++i]));
					static_cast<FunctionExpression*>(root)->subexpressions.pop_back();
					static_cast<FunctionExpression*>(root)->subexpressions.push_back(expr);
				} else {
					root = arena.make<MemberVariable>(root, std::string(strings[++i]));
				}
			}
		} else {
			// number (supports decimal, 0x hex, 0b binary)
			auto val = parseNumericLiteral(strings[i]);
			bool isFloat = contains(strings[i], '.') && !isHexLiteral(strings[i]) && !isBinaryLiteral(strings[i]);
			auto newExpr = arena.make<ValueNode>(ValuePtr(isFloat ? new Value((Float)val) : new Value((Int)val)), ExpressionType::Value);
			if (root) {
				static_cast<FunctionExpression*>(root)->subexpressions.push_back(newExpr);
			} else {
				root = newExpr;
			}
		}
		++i;
	}

	return root;
}

// parse one token at a time, uses the state machine
void Parser::parse(std::string_view token) {
	auto tempState = parseState;
	switch (parseState) {
	case ParseState::BeginExpression:
	{
		bool wasElse = false;
		bool closedScope = false;
		bool closedExpr = false;
		bool isEndCurlBracket = false;
		if (token == "@") {
			parseState = ParseState::Decorator;
		} else if (token == "fun" || token == "coro") {
			parseState = ParseState::DefineFunc;
		} else if (token == "var") {
			parseState = ParseState::DefineVar;
		} else if (token == "dynamic") {
			parseState = ParseState::DefineDynamic;
		} else if (token == "for" || token == "while") {
			parseState = ParseState::LoopCall;
			if (currentExpression) {
				auto newexpr = arena.make<Loop>(currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			} else {
				currentExpression = arena.make<Loop>();
			}
		} else if (token == "foreach") {
			parseState = ParseState::ForEach;
			if (currentExpression) {
				auto newexpr = arena.make<Foreach>(currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			} else {
				currentExpression = arena.make<Foreach>();
			}
		} else if (token == "if") {
			parseState = ParseState::IfCall;
			if (currentExpression) {
				auto dummy = std::vector<If>{};
				auto newexpr = arena.make<IfElse>(dummy, currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			} else {
				currentExpression = arena.make<IfElse>();
			}
		} else if (token == "else") {
			parseState = ParseState::ExpectIfEnd;
			wasElse = true;
		} else if (token == "match") {
			parseState = ParseState::MatchCall;
			if (currentExpression) {
				auto newexpr = arena.make<MatchExpression>(currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			} else {
				currentExpression = arena.make<MatchExpression>();
			}
		} else if (token == "case") {
			if (!currentExpression && previousExpression && previousExpression->type == ExpressionType::Match) {
				currentExpression = previousExpression;
			}
			if (currentExpression && currentExpression->type == ExpressionType::Match) {
				static_cast<MatchExpression*>(currentExpression)->arms.push_back(MatchArm{});
				parseState = ParseState::MatchCasePattern;
			}
		} else if (token == "default") {
			if (!currentExpression && previousExpression && previousExpression->type == ExpressionType::Match) {
				currentExpression = previousExpression;
			}
			if (currentExpression && currentExpression->type == ExpressionType::Match) {
				static_cast<MatchExpression*>(currentExpression)->arms.push_back(MatchArm{});
				parseState = ParseState::MatchDefault;
			}
		} else if (token == "class") {
			parseState = ParseState::DefineClass;
		} else if (token == "{") {
			parseScope = newScope("__anon"s, parseScope);
			clearParseStacks();
		} else if (token == "}") {
			wasElse = !currentExpression || (currentExpression->type != ExpressionType::IfElse
				&& currentExpression->type != ExpressionType::Match);
			bool wasFreefunc = !currentExpression || (currentExpression->type == ExpressionType::FunctionDef
				&& static_cast<FunctionExpression*>(currentExpression)->function->getFunction()->type == FunctionType::free);
			closedExpr = closeCurrentExpression();
			if (!closedExpr && wasFreefunc || parseScope->name == "__anon") {
				closeScope(parseScope);
			}
			closedScope = true;
			isEndCurlBracket = true;
		} else if (token == "return") {
			parseState = ParseState::ReturnLine;
		} else if (token == "import") {
			parseState = ParseState::ImportModule;
		} else if (token == ";") {
			clearParseStacks();
		} else {
			parseState = ParseState::ReadLine;
			parseStrings.push_back(token);
		}
		if (!closedExpr && (closedScope && lastStatementClosedScope || (!lastStatementWasElse && !wasElse && lastTokenEndCurlBraket))) {
			bool wasIfExpr = currentExpression && currentExpression->type == ExpressionType::IfElse;
			closeDanglingIfExpression();
			if (closedScope && wasIfExpr && currentExpression->type != ExpressionType::IfElse) {
				closeCurrentExpression();
				closedScope = false;
			}
		}
		lastStatementClosedScope = closedScope;
		lastTokenEndCurlBraket = isEndCurlBracket;
		lastStatementWasElse = wasElse;
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
					} else {
						exprs.back().push_back(str);
					}
				}
				auto loop = static_cast<Loop*>(currentExpression);
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
					auto name = exprs[0].front();
					exprs[0].erase(exprs[0].begin(), exprs[0].begin() + 2);
					loop->initExpression = arena.make<DefineVar>(std::string(name), getExpression(exprs[0], parseScope, nullptr));
					loop->testExpression = getExpression(exprs[1], parseScope, nullptr);
					loop->iterateExpression = getExpression(exprs[2], parseScope, nullptr);
				}
				break;
				default:
					break;
				}

				clearParseStacks();
				outerNestLayer = 0;
			} else {
				parseStrings.push_back(token);
			}
		} else if (token == "(") {
			if (++outerNestLayer > 1) {
				parseStrings.push_back(token);
			}
		} else {
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
					} else {
						exprs.back().push_back(move(str));
					}
				}
				if (exprs.size() != 2) {
					clearParseStacks();
					throw Exception("Syntax error, `foreach` requires 2 statements, "s + std::to_string(exprs.size()) + " statements supplied instead");
				}

				auto name = std::string(exprs[0][0]);
				resolveVariable(name, parseScope);
				static_cast<Foreach*>(currentExpression)->iterateName = move(name);
				static_cast<Foreach*>(currentExpression)->listExpression = getExpression(exprs[1], parseScope, nullptr);

				clearParseStacks();
				outerNestLayer = 0;
			} else {
				parseStrings.push_back(token);
			}
		} else if (token == "(") {
			if (++outerNestLayer > 1) {
				parseStrings.push_back(token);
			}
		} else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::IfCall:
		if (token == ")") {
			if (--outerNestLayer <= 0) {
				static_cast<IfElse*>(currentExpression)->states.push_back(If());
				static_cast<IfElse*>(currentExpression)->states.back().testExpression = getExpression(move(parseStrings), parseScope, nullptr);
				clearParseStacks();
			} else {
				parseStrings.push_back(token);
			}
		} else if (token == "(") {
			if (++outerNestLayer > 1) {
				parseStrings.push_back(token);
			}
		} else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::ReadLine:
		if (token == "{") {
			parseLambdaBrakets++;
			parseStrings.push_back(token);
		} else if (token == "}") {
			if (parseLambdaBrakets == 0) {
				if (!parseStrings.empty()) {
					auto line = move(parseStrings);
					clearParseStacks();
					if (!currentExpression) {
						getValue(line, parseScope, nullptr);
					} else {
						currentExpression->push_back(getExpression(line, parseScope, nullptr));
					}
				} else {
					clearParseStacks();
				}
				// Inline BeginExpression '}' handling (no recursion)
				parseState = ParseState::BeginExpression;
				bool closedExpr = closeCurrentExpression();
				if (!closedExpr || parseScope->name == "__anon") {
					closeScope(parseScope);
				}
				if (previousExpression && previousExpression->type != ExpressionType::IfElse
					&& previousExpression->type != ExpressionType::Match) {
					closedExpr = false;
				}
				lastStatementClosedScope = closedExpr;
			} else {
				parseLambdaBrakets--;
				parseStrings.push_back(token);
			}
		} else if (token == ";") {
			auto line = move(parseStrings);
			clearParseStacks();
			// we clear before evaluating lines so any exceptions can clear the offending code
			if (!currentExpression) {
				getValue(line, parseScope, nullptr);
			} else {
				currentExpression->push_back(getExpression(line, parseScope, nullptr));
			}

		} else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::ReturnLine:
		if (token == ";") {
			if (currentExpression) {
				currentExpression->push_back(arena.make<Return>(getExpression(move(parseStrings), parseScope, nullptr)));
			}
			clearParseStacks();
		} else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::ExpectIfEnd:
		if (token == "if") {
			clearParseStacks();
			parseState = ParseState::IfCall;
		} else if (token == "{") {
			parseScope = newScope("__anon"s, parseScope);
			static_cast<IfElse*>(currentExpression)->states.push_back(If());
			clearParseStacks();
		} else {
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
			} else { parseStrings.push_back(token); }
		} else if (token == "(") {
			if (++outerNestLayer > 1) parseStrings.push_back(token);
		} else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::MatchCasePattern:
		if (token == "=>") {
			auto* matchExpr = static_cast<MatchExpression*>(currentExpression);
			if (parseStrings.size() == 1 && parseStrings[0] == "_") {
				matchExpr->arms.back().pattern = nullptr;
			} else {
				matchExpr->arms.back().pattern = getExpression(move(parseStrings), parseScope, nullptr);
			}
			clearParseStacks();
		} else { parseStrings.push_back(token); }
		break;
	case ParseState::MatchDefault:
		if (token == "=>") { clearParseStacks(); }
		else { parseStrings.push_back(token); }
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
	case ParseState::DefineDynamic:
		if (token == ";") {
			if (parseStrings.size() == 0) {
				throw Exception("Malformed Syntax: `var`/`dynamic` keyword must be followed by user supplied name");
			}
			TypeDescriptor tDescriptor;
			tDescriptor.isDynamic = parseState == ParseState::DefineDynamic;
			auto name = parseStrings.front();
			ExpressionPtr defineExpr = nullptr;
			if (parseStrings.size() > 2) {
				parseStrings.erase(parseStrings.begin());
				if (parseStrings[0] == ":") {
					parseStrings.erase(parseStrings.begin());
					auto typeName = std::string(parseStrings[0].begin(), parseStrings[0].end());
					auto typeDesc = checkTypeInScope(typeName, parseScope);
					if (typeDesc.isDynamic) {
						tDescriptor.isDynamic = true;
					} else {
						tDescriptor.type = typeDesc.type;
						tDescriptor.isDynamic = false;
					}
					parseStrings.erase(parseStrings.begin());
				}
				parseStrings.erase(parseStrings.begin());
				defineExpr = getExpression(move(parseStrings), parseScope, nullptr);
			}
			if (currentExpression) {
				currentExpression->push_back(arena.make<DefineVar>(std::string(name), defineExpr, tDescriptor));
			} else {
				getValue(arena.make<DefineVar>(std::string(name), defineExpr, tDescriptor), parseScope, nullptr);
			}
			if (pendingMetadata.size()) {
				parseScope->membersMetadata[std::string(name)] = pendingMetadata;
				pendingMetadata.clear();
			}
			clearParseStacks();
		} else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::DefineClass:
		parseScope = newClassScope(std::string(token), parseScope);
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
				auto otherscope = resolveScope(std::string(parseStrings.back()), parseScope);
				parseScope->variables.insert(otherscope->variables.begin(), otherscope->variables.end());
				parseScope->functions.insert(otherscope->functions.begin(), otherscope->functions.end());
				parseStrings.clear();
			}
		} else if (token == "{") {
			if (parseStrings.size()) {
				auto otherscope = resolveScope(std::string(parseStrings.back()), parseScope);
				parseScope->variables.insert(otherscope->variables.begin(), otherscope->variables.end());
				parseScope->functions.insert(otherscope->functions.begin(), otherscope->functions.end());
			}
			clearParseStacks();
		} else {
			parseStrings.push_back(token);
		}
		break;
	case ParseState::DefineFunc:
		parseStrings.push_back(token);
		parseState = ParseState::FuncArgs;
		break;
	case ParseState::ImportModule:
		clearParseStacks();
		if (token.size() > 2 && token.front() == '\"' && token.back() == '\"') {
			// import file	   
			evaluateFile(std::string(token.substr(1, token.size() - 2)));
			clearParseStacks();
		} else {
			auto modName = std::string(token);
			auto iter = std::find_if(modules.begin(), modules.end(), [&modName](auto& mod) {return mod.scope->name == modName; });
			if (iter == modules.end()) {
				auto newMod = getOptionalModule(modName);
				if (newMod) {
					if (shouldAllow(allowedModulePrivileges, newMod->requiredPermissions)) {
						modules.emplace_back(newMod->requiredPermissions, newMod->scope);
					} else {
						throw Exception("Error: Cannot import restricted module: "s + modName);
					}
				}
			}
		}
		break;
	case ParseState::FuncArgs:
		if (token == "(" || token == ",") {
			// eat these tokens
		} else if (token == ")") {
			auto fncName = move(parseStrings.front());
			parseStrings.erase(parseStrings.begin());
			std::vector<std::string> args;
			args.reserve(parseStrings.size());
			for (auto parseString : parseStrings) {
				args.emplace_back(parseString);
			}
			auto isConstructor = parseScope->isClassScope && parseScope->name == fncName;
			auto newfunc = isConstructor ? newConstructor(std::string(fncName), parseScope->parent, args) : newFunction(std::string(fncName), parseScope, args);
			if (pendingMetadata.size()) {
				parseScope->membersMetadata[std::string(fncName)] = pendingMetadata;
				pendingMetadata.clear();
			}
			if (currentExpression) {
				auto newexpr = arena.make<FunctionExpression>(newfunc, currentExpression);
				currentExpression->push_back(newexpr);
				currentExpression = newexpr;
			} else {
				currentExpression = arena.make<FunctionExpression>(newfunc, nullptr);
			}
		} else if (token == "{") {
			clearParseStacks();
		} else {
			parseStrings.push_back(token);
		}
		break;
	default:
		break;
	}

	prevState = tempState;
}

bool IkigaiScriptInterpreter::readLine(std::string_view text) {
	++currentLine;
	auto tokenCount = 0;
	auto tokens = Tokenize(text);
	bool didExcept = false;
	try {			
		for (auto& token : tokens) {
			parse(token);
			++tokenCount;
		}
	} catch (Exception e) {
#if defined SCRIPT_DO_INTERNAL_PRINT
		callFunctionWithArgs(resolveFunction("print"), "Error at line "s + std::to_string(currentLine) + ", at: " + std::to_string(tokenCount) + string(tokens[tokenCount]) + ": " + e.wh + "\n");
#else 
		printf("Error at line %llu at %i: %s : %s\n", currentLine, tokenCount, std::string(tokens[tokenCount]).c_str(), e.errStr.c_str());
#endif		
		__EXEPTION__ = e.type;
		clearParseStacks();
		parseScope = globalScope;
		currentExpression = nullptr;
		didExcept = true;
	} catch (std::exception& e) {
#if defined SCRIPT_DO_INTERNAL_PRINT
		callFunctionWithArgs(resolveFunction("print"), "Error at line "s + std::to_string(currentLine)  + ", at: " + std::to_string(tokenCount) + string(tokens[tokenCount]) +  ": " + e.what()  + "\n");
#else
		printf("Error at line %llu at %i: %s : %s\n", currentLine, tokenCount, std::string(tokens[tokenCount]).c_str(), e.what());
#endif		
		clearParseStacks();
		parseScope = globalScope;
		currentExpression = nullptr;
		didExcept = true;
	}
	return didExcept;
}

bool IkigaiScriptInterpreter::evaluate(std::string_view script) {
	for (auto& line : split(script, '\n')) {
		if (readLine(line)) {
			return true;
		}
	}
	// close any dangling if-expressions that may exist
	return readLine(";"s);
}

bool IkigaiScriptInterpreter::evaluateFile(const std::string& path) {
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
	} else {
		printf("file: %s not found\n", path.c_str());
		return 1;
	}
}

bool IkigaiScriptInterpreter::readLine(std::string_view text, ScopePtr scope) {
	auto temp = parseScope;
	parseScope = scope;
	auto result = readLine(text);
	parseScope = temp;
	return result;
}

bool IkigaiScriptInterpreter::evaluate(std::string_view script, ScopePtr scope) {
	auto temp = parseScope;
	parseScope = scope;
	auto result = evaluate(script);
	parseScope = temp;
	return result;
}

bool IkigaiScriptInterpreter::evaluateFile(const std::string& path, ScopePtr scope) {
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

void IkigaiScriptInterpreter::clearState() {
	clearParseStacks();
	ClearScope(globalScope);
	globalScope = std::make_shared<Scope>(this);
	parseScope = globalScope;
	currentExpression = nullptr;
	if (modules.size() > 1) {
		modules.erase(modules.begin() + 1, modules.end());
	}
}

// general purpose clear to reset state machine for next statement
void IkigaiScriptInterpreter::clearParseStacks() {
	parseState = ParseState::BeginExpression;
	parseStrings.clear();
	outerNestLayer = 0;
}
