#pragma once

#include <algorithm>
#include <array>
#include <type_traits>
#include <memory>
#include <ranges>
#include <variant>
#include <optional>
#include <string_view>

namespace IkigaiScript {

#define DEFINE_ENUM_CLASS_BITWISE_OPERATORS(Enum)				   \
	inline constexpr Enum operator|(Enum Lhs, Enum Rhs) {		   \
		return static_cast<Enum>(								   \
			static_cast<std::underlying_type_t<Enum>>(Lhs) |		\
			static_cast<std::underlying_type_t<Enum>>(Rhs));		\
	}															   \
	inline constexpr Enum operator&(Enum Lhs, Enum Rhs) {		   \
		return static_cast<Enum>(								   \
			static_cast<std::underlying_type_t<Enum>>(Lhs) &		\
			static_cast<std::underlying_type_t<Enum>>(Rhs));		\
	}															   \
	inline constexpr Enum operator^(Enum Lhs, Enum Rhs) {		   \
		return static_cast<Enum>(								   \
			static_cast<std::underlying_type_t<Enum>>(Lhs) ^		\
			static_cast<std::underlying_type_t<Enum>>(Rhs));		\
	}															   \
	inline constexpr Enum operator~(Enum E) {					   \
		return static_cast<Enum>(								   \
			~static_cast<std::underlying_type_t<Enum>>(E));		 \
	}															   \
	inline Enum& operator|=(Enum& Lhs, Enum Rhs) {				  \
		return Lhs = static_cast<Enum>(							 \
				   static_cast<std::underlying_type_t<Enum>>(Lhs) | \
				   static_cast<std::underlying_type_t<Enum>>(Lhs)); \
	}															   \
	inline Enum& operator&=(Enum& Lhs, Enum Rhs) {				  \
		return Lhs = static_cast<Enum>(							 \
				   static_cast<std::underlying_type_t<Enum>>(Lhs) & \
				   static_cast<std::underlying_type_t<Enum>>(Lhs)); \
	}															   \
	inline Enum& operator^=(Enum& Lhs, Enum Rhs) {				  \
		return Lhs = static_cast<Enum>(							 \
				   static_cast<std::underlying_type_t<Enum>>(Lhs) ^ \
				   static_cast<std::underlying_type_t<Enum>>(Lhs)); \
	}

	
	
}


namespace IkigaiScript
{

	enum class Type : uint8_t {
		Null = 0,
		Bool,
		Char,
		Int,
		Float,
		Function,
		Coro,
		Pointer,
		String,
		Array,
		List,
		Set,
		Map,
		Class,
		Generic,
		Range,
		Tuple
	};

	enum class LoopInterupt {
		None, Break, Continue
	};

	enum class ExpressionType : uint8_t {
		Value,
		ResolveVar,
		DefineVar,
		FunctionDef,
		FunctionCall,
		CoroCall,
		MemberFunctionCall,
		MemberVariable,
		Return,
		Yeld,
		Loop,
		ForEach,
		IfElse,
		Continue,
		Break,
		Block,
		NamedArgument,
		Match,
		TupleLiteral
	};

	enum class ParseState : uint8_t {
		BeginExpression,
		ReadLine,
		DefineVar,
		DefineConst,
		DefineFunc,
		DefineCoro,
		DefineOperator,
		DefineClass,
		ClassArgs,
		FuncArgs,
		OperatorArgs,
		CoroArgs,
		FuncResult,
		ReturnLine,
		YeldLine,
		ContinueLine,
		BreakLine,
		IfCall,
		ExpectIfEnd,
		LoopCall,
		ForEach,
		ImportModule,
		Decorator,
		DecoratorArgs,
		DecoratorArgsList,
		GenericBody,
		DefineDynamic,
		MatchCall,
		MatchCasePattern,
		MatchDefault
	};

	

	enum class TokenType {
		KeyWord = 0,
		Identifire = 1,

		NumInt = 2,
		NumFloat = 4,
		NumHex = 8,
		NumBin = 16,
		Num = NumInt | NumFloat | NumHex | NumBin,

		Operator = 32,
		SpecSymbol = 64,

		String = 128,
		Char = 256,
	};

	DEFINE_ENUM_CLASS_BITWISE_OPERATORS(TokenType)

	struct TypeDescriptor {
		Type type = Type::Null;
		bool isInit = false;
		bool isNullable = true;
		bool isConst = false;
		bool isDynamic = false;

		std::optional<std::variant<std::string, std::shared_ptr<TypeDescriptor>>> subtype;
		std::optional<std::shared_ptr<TypeDescriptor>> subtype2; // for map
	};
}
