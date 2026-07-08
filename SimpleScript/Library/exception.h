#pragma once
#include <exception>
#include <string>

#include "enums.h"

namespace IkigaiScript {

	enum class ExceptionType {
		None,
		TypeConvert,
		Unknown,
	};

	struct Exception: public std::exception {
		ExceptionType type = ExceptionType::Unknown;
		std::string errStr;
		Exception(std::string errStr): errStr(std::move(errStr)) {
		}
	};

	struct ExceptionExpectExpression: public Exception {
		ExpressionType type;
		ExceptionExpectExpression(std::string errStr, ExpressionType type): Exception(std::move(errStr)), type(type) {
		}
	};

	struct TypeConvertError: public Exception {
		TypeConvertError(TypeDescriptor typeFrom, TypeDescriptor typeTo):
			Exception("Cannot convert types from " +
			std::to_string((int)typeFrom.type) + " to " + std::to_string((int)typeTo.type)) {
			type = ExceptionType::TypeConvert;
		}
	};

	struct Value;

	// Thrown by the bytecode VM when a coro executes OP_YELD.
	// Caught by VM::resumeCoro / IkigaiScriptInterpreter::callCoro to suspend the task.
	struct YieldSignal {
		std::shared_ptr<Value> payload;
		int savedIp = 0;
	};
}
