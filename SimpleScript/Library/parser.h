#pragma once

#include <vector>
#include <string>
#include <string_view>
#include "types.hpp"
#include "scope.hpp"

namespace IkigaiScript {
	class IkigaiScriptInterpreter;

	class Parser {
	public:
		Parser(IkigaiScriptInterpreter* interpreter);

		bool readLine(std::string_view text);
		bool evaluate(std::string_view script);
		bool evaluateFile(const std::string& path);
		bool readLine(std::string_view text, ScopePtr scope);
		bool evaluate(std::string_view script, ScopePtr scope);
		bool evaluateFile(const std::string& path, ScopePtr scope);
		void clearState();
		bool closeCurrentExpression();

		void parse(std::string_view token);
		void clearParseStacks();

		ExpressionPtr getExpression(std::vector<std::string_view> strings, ScopePtr scope, Class* classs);
		ExpressionPtr getResolveVarExpression(const std::string& name, bool classScope);
		ExpressionPtr parseInterpolatedString(std::string val, ScopePtr scope, Class* classs);
		ExpressionPtr parseInlineLambda(std::vector<std::string_view> tokens, ScopePtr scope);
		void parseType(TypeDescriptor& td);
		void parseContainerSubtype(TypeDescriptor& td);

		IkigaiScriptInterpreter* interpreter = nullptr;
		ScopePtr parseScope;
		ExpressionPtr currentExpression = nullptr;
		ExpressionPtr previousExpression = nullptr;
		
		ParseState parseState = ParseState::BeginExpression;
		std::vector<std::string_view> parseStrings;
		int outerNestLayer = 0;
		bool lastStatementClosedScope = false;
		
		ParseState prevState = ParseState::BeginExpression;
		
		int parseLambda = 0;
		int parseLambdaBrakets = 0;
		int genericBodyNesting = 0;
		
		std::vector<Metadata> pendingMetadata;
	};
}
