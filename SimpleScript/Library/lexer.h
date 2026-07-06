#pragma once

#include <set>
#include <string>
#include <string_view>
#include <vector>
#include "enums.h"

namespace IkigaiScript {

	inline std::set<char> operator+(const std::set<char>& lhs, const std::set<char>& rhs) {
		std::set<char> res;
		res.insert(lhs.begin(), lhs.end());
		res.insert(rhs.begin(), rhs.end());
		return res;
	}

	inline const std::set<char> WhitespaceChars = { ' ', '\t', '\n' };
	inline const std::set<char> SpecialChars = { ',', '.', '(', ')', '{', '}', '[', ']', ';', '+', '-', '/', '*', '%', '<', '>', '=', '!', '&', '|', '"', '\'', '?', ':', '@'};
	inline const std::set<char> GrammarChars = WhitespaceChars + SpecialChars;
	inline const std::set<char> MultiCharTokenStartChars = { '+', '-', '/', '*', '<', '>', '=', '!', '&', '|', '#'};
	inline const std::set<char> NumChars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'e', 'E', 'x', 'X', 'b', 'B',
		'A', 'C', 'D', 'F', 'a', 'c', 'd', 'f', '+', '-' };
	inline const std::set<char> NumChars2 = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', 'e', 'E', 'x', 'X', 'b', 'B',
		'A', 'C', 'D', 'F', 'a', 'c', 'd', 'f' };
	inline const std::set<char> NumStartChars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	inline const std::set<char> DisallowedIdentifierStartChars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', '\t', '\n', ',', '.',
		'(', ')', '{', '}', '[', ']', ';', '+', '-', '/', '*', '%', '<', '>', '=', '!', '&', '|', '"' };
	inline const std::set<std::string, std::less<>> keyWords = { "if", "else", "for", "while", "foreach", "break", "continue",
		"var", "const",
		"class", "fun", "operator", "coroutine", "coro", "return", "yeld",
		"true", "false", "import",
		"match", "case", "default", "dynamic",
		"module", "export", "using", "from", "as", "defer"
	};

	struct Token {
		std::string_view token;
		TokenType type;
	};

	class Lexer {
	public:
		static std::vector<std::string_view> Tokenize(std::string_view input);
		static std::vector<Token> Tokenizer(std::string_view input);
	};

	// Token character checks
	bool isStartIdentChar(char ch);
	bool isIdentChar(char ch);
	bool isNumChar(char ch);
	bool isBinNumChar(char ch);
	bool isHexNumChar(char ch);
}
