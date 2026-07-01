#include "lexer.h"
#include "exception.h"
#include "stringUtils.hpp"

namespace IkigaiScript {
using namespace std::string_literals;

inline size_t findFirstOf(std::string_view input, size_t pos, const std::set<char>& pattern) {
	while (pos < input.size()) {
		if (pattern.contains(input[pos])) {
			return pos;
		}
		pos++;
	}
	return std::string::npos;
}

bool isStartIdentChar(char ch) {
	return ch == '_' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

bool isIdentChar(char ch) {
	return ch == '_' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9');
}

bool isNumChar(char ch) {
	return (ch >= '0' && ch <= '9');
}

bool isBinNumChar(char ch) {
	return (ch == '0' || ch == '1');
}

bool isHexNumChar(char ch) {
	return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

std::vector<std::string_view> Lexer::Tokenize(std::string_view input) {
	std::vector<std::string_view> ret;
	if (input.empty()) {
		return ret;
	}
	bool exitFromComment = false;

	size_t pos = 0;
	size_t lpos = 0;
	while ((pos = findFirstOf(input, lpos, GrammarChars)) != std::string::npos) {
		size_t len = pos - lpos;
		// differentiate between decimals and dot syntax for function calls
		if (input[pos] == '.' && pos + 1 < input.size() && contains(NumChars, input[pos + 1])) {
			pos = findFirstOf(input, pos + 1, GrammarChars);
			ret.push_back(input.substr(lpos, pos - lpos));
			lpos = pos;
			continue;
		}
		if (len) {
			ret.push_back(input.substr(lpos, pos - lpos));
			lpos = pos;
		}
		else {
			// handle strings and escaped strings
			if (input[pos] == '\"' && (pos == 0 || input[pos - 1] != '\\')) {
				auto originalPos = pos;
				bool isMultiline = (pos + 2 < input.size() && input[pos+1] == '"' && input[pos+2] == '"');
				if (isMultiline) {
					auto testpos = pos + 3;
					bool loop = true;
					while (loop) {
						pos = input.find("\"\"\"", testpos);
						if (pos == std::string::npos) {
							throw Exception("Multiline quote mismatch at "s + std::string(input.substr(lpos, input.size() - lpos)));
						}
						loop = (input[pos - 1] == '\\');
						testpos = pos + 3;
					}
					pos += 3;
				} else {
					auto testpos = pos + 1;
					bool loop = true;
					while (loop) {
						pos = input.find('\"', testpos);
						if (pos == std::string::npos) {
							throw Exception("Quote mismatch at "s + std::string(input.substr(lpos, input.size() - lpos)));
						}
						loop = (input[pos - 1] == '\\');
						testpos = pos + 1;
					}
					pos += 1;
				}

				ret.push_back(input.substr(originalPos, pos - originalPos));
				lpos = pos;
				continue;
			}
			else if (input[pos] == '\'' && (pos == 0 || input[pos - 1] != '\\')) {
				auto originalPos = pos;
				auto testpos = pos + 1;
				bool loop = true;
				while (loop) {
					pos = input.find('\'', testpos);
					if (pos == std::string::npos) {
						throw Exception("Char literal mismatch at "s + std::string(input.substr(lpos, input.size() - lpos)));
					}
					loop = (input[pos - 1] == '\\');
					testpos = pos + 1;
				}
				pos += 1;

				ret.push_back(input.substr(originalPos, pos - originalPos));
				lpos = pos;
				continue;
			}
		}
		// special case for negative numbers
		if ((input[pos] == '-' || input[pos] == '+') && contains(NumChars2, input[pos + 1])
			&& (ret.size() == 0 || contains(MultiCharTokenStartChars, ret.back().back()))) {
			pos = findFirstOf(input, pos + 1, GrammarChars);
			if ((input[pos] == '.' && pos + 1 < input.size() && contains(NumChars, input[pos + 1])) ||
				((input[pos] == '+' || input[pos] == '-') && pos + 1 < input.size() && contains(NumChars, input[pos + 1]))) {
				pos = findFirstOf(input, pos + 1, GrammarChars);
			}
			ret.push_back(input.substr(lpos, pos - lpos));
			lpos = pos;
		}
		else if (!contains(WhitespaceChars, input[pos])) {
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
		}
		else {
			++lpos;
		}
	}
	if (!exitFromComment && lpos < input.length()) {
		ret.push_back(input.substr(lpos, input.length()));
	}
	return ret;
}


enum class ParseIntState
{
	None,
	StartUnary,
	Float,
	Bin,
	Hex,
	Num,
	ExpSign,
	Exp
};

std::vector<Token> Lexer::Tokenizer(std::string_view input) {
	std::vector<Token> ret;
	if (input.empty()) {
		return ret;
	}
	bool isComment = false;

	size_t pos = 0;
	while (pos < input.size()) {
		if (contains(WhitespaceChars, input[pos])) {
			pos++;
			continue;
		}

		if (isStartIdentChar(input[pos])) { //ident
			auto startPos = pos;
			while (pos < input.size() && isIdentChar(input[pos])) {
				pos++;
			}
			auto token = input.substr(startPos, pos - startPos);
			ret.push_back({ token, keyWords.contains(token) ? TokenType::KeyWord : TokenType::Identifire });
			continue;
		}

		if (contains(NumStartChars, input[pos]) || (
			(input[pos] == '-' || input[pos] == '+') && contains(NumStartChars, input[pos + 1])
			&& ((ret.size() == 0) || contains(MultiCharTokenStartChars, ret.back().token.back())))
		) {//num
			ParseIntState state = ParseIntState::None;
			auto startPos = pos;
			auto type = TokenType::NumInt;
			while (pos < input.size() && contains(NumChars, input[pos])) {
				switch (state)
				{
				case ParseIntState::None:
					{
						if (input[pos] == '+' || input[pos] == '-') {
							state = ParseIntState::StartUnary;
							break;
						}
						else if (isNumChar(input[pos])) {
							state = ParseIntState::Num;
							break;
						}
						throw;
					}
					break;
				case ParseIntState::StartUnary: 
					{
						if (isNumChar(input[pos])) {
							state = ParseIntState::Num;
							break;
						}
						throw;
					}
					break;
				case ParseIntState::Num:
					{
						if (isNumChar(input[pos])) {
							break;
						}
						if (input[pos] == 'e' || input[pos] == 'E') {
							state = ParseIntState::ExpSign;
							type = TokenType::NumFloat;
							break;
						}
						else if (input[pos] == 'x' || input[pos] == 'X') {
							if (startPos == pos-1 && input[pos-1] == '0') {
								state = ParseIntState::Hex;
								type = TokenType::NumHex;
								break;
							}
							throw;
						}
						else if (input[pos] == 'b' || input[pos] == 'B') {
							if (startPos == pos - 1 && input[pos - 1] == '0') {
								state = ParseIntState::Bin;
								type = TokenType::NumBin;
								break;
							}
							throw;
						}
						else if (input[pos] == '.') {
							state = ParseIntState::Float;
							type = TokenType::NumFloat;
							break;
						}
						throw;
					}
					break;
				case ParseIntState::ExpSign:
					{
						if (input[pos] == '+' || input[pos] == '-') {
							break;
						}
						if (isNumChar(input[pos])) {
							state = ParseIntState::Exp;
							break;
						}
						throw;
					}
					break;
				case ParseIntState::Exp:
				{
					if (isNumChar(input[pos])) {
						break;
					}
					throw;
				}
				break;
				case ParseIntState::Float:
					{
						if (isNumChar(input[pos])) {
							break;
						}
						if (input[pos] == 'e' || input[pos] == 'E') {
							state = ParseIntState::ExpSign;
							break;
						}
						throw;
					}
					break;
				case ParseIntState::Bin:
					{
						if (isBinNumChar(input[pos])) {
							break;
						}
						throw;
					}
					break;
				case ParseIntState::Hex: 
					{
						if (isHexNumChar(input[pos])) {
							break;
						}
						throw;
					}
					break;
				}

				pos++;
			}


			//if (input.size() > startPos + 1 && input[startPos] == '0' && input[startPos + 1] == '0') {
			//	throw;
			//}
			//
			//if (input.size() > startPos + 2 && 
			//	(input[startPos] == '+' || input[startPos] == '1') && input[startPos + 1] == '0' && input[startPos + 2] == '0') {
			//	throw;
			//}

			ret.push_back({ input.substr(startPos, pos - startPos), type });
			continue;
		}

		if (contains(MultiCharTokenStartChars, input[pos])) {
			auto startPos = pos;
			pos++;
			if (pos < input.size() && contains(MultiCharTokenStartChars, input[pos])) {
				pos++;
				if (input[startPos] == '/' && input[startPos + 1] == '/') {
					isComment = true;
					while (pos < input.size()) {
						if (input[pos] == '\n') {
							break;
						}
						pos++;
					}
					continue;
				}
			}
			ret.push_back({ input.substr(startPos, pos - startPos), TokenType::Operator });
			continue;
		}

		if (contains(SpecialChars, input[pos])) {
			auto startPos = pos;
			if (input[startPos] == '"') {
				bool isMultiline = false;
				if (pos + 2 < input.size() && input[pos+1] == '"' && input[pos+2] == '"') {
					isMultiline = true;
					pos += 3;
				} else {
					pos++;
				}
				
				bool endStr = false;
				while (pos < input.size()) {
					if (isMultiline) {
						if (input[pos] == '"' && pos + 2 < input.size() && input[pos+1] == '"' && input[pos+2] == '"') {
							if (input[pos-1] != '\\') {
								pos += 3;
								endStr = true;
								break;
							}
						}
					} else {
						if (input[pos] == '"') {
							if (input[pos-1] != '\\') {
								pos++;
								endStr = true;
								break;
							}
						}
					}
					pos++;
				}
				if (!endStr) {
					throw Exception("Unterminated string");
				}
				ret.push_back({ input.substr(startPos, pos - startPos), TokenType::String });
				continue;
			}
			else if (input[startPos] == '\'') {
				pos++;
				bool endStr = false;
				while (pos < input.size()) {
					if (input[pos] == '\'') {
						if (input[pos-1] != '\\') {
							pos++;
							endStr = true;
							break;
						}
					}
					pos++;
				}
				if (!endStr) {
					throw Exception("Unterminated char literal");
				}
				ret.push_back({ input.substr(startPos, pos - startPos), TokenType::Char });
				continue;
			}
			pos++;
			ret.push_back({ input.substr(startPos, pos - startPos), TokenType::SpecSymbol });
			continue;
		}
	}

	return ret;
}

}
