#pragma once

#include <vector>
#include <memory>
#include <variant>
#include <string>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <map>
#include <mutex>
#include <optional>
#include <unordered_set>

#include "exception.h"
#include "stringUtils.hpp"
#include "concurrency/task.hpp"

namespace IkigaiScript {

    using namespace std::string_literals;
#ifdef IS_32_BIT
    using Int = int;
    using Float = float;
#else
    using Int = long long;
    using Float = double;
#endif
    using Bool = bool;
	
	inline std::string getTypeName(Type t) {
		switch (t) {
		case Type::Null:
			return "null";
        case Type::Bool:
            return "Bool";
		case Type::Char:
			return "char";
		case Type::Int:
			return "int";
		case Type::Float:
			return "float";
		case Type::Function:
			return "function";
        case Type::Coro:  // same value as Type::Task
            return "task";
		case Type::Pointer:
			return "pointer";
		case Type::String:
			return "string";
		case Type::Array:
			return "array";
		case Type::List:
			return "list";
        case Type::Set:
            return "set";
		case Type::Map:
			return "map";
		case Type::Class:
			return "class";
		case Type::Range:
			return "range";
		case Type::Tuple:
			return "tuple";
		case Type::Optional:
			return "Optional";
		case Type::Result:
			return "Result";
		default:
			return "unknown";
		}
	}

	inline size_t typeHashBits(Type type) {
		return (static_cast<size_t>(INT32_MAX) << static_cast<size_t>(type));
	}

	struct Value;
	using ValuePtr = std::shared_ptr<Value>;

	struct ResultData {
		bool isOk = true;
		ValuePtr value;
	};
	using ResultDataPtr = std::shared_ptr<ResultData>;

	using List = std::vector<ValuePtr>;
	
	struct Metadata {
		std::string name;
		std::vector<std::pair<std::string, ValuePtr>> arguments;
	};

	struct Function;
	using FunctionRef = std::shared_ptr<Function>;

    // Coro/CoroRef are now defined in concurrency/task.hpp (Task/TaskRef aliases)
	
	struct Array {
        Type type;
        union Payload {
            std::vector<Bool> asBool;
            std::vector<Int> asInt;
            std::vector<Float> asFloat;
            std::vector<FunctionRef> asFunction;
            std::vector<void*> asPointer;
            std::vector<std::string> asString;
            Payload() {}
            ~Payload() {}
        } value;

        void destroy() {
            switch(type) {
            case Type::Bool: value.asBool.~vector(); break;
            case Type::Int: value.asInt.~vector(); break;
            case Type::Float: value.asFloat.~vector(); break;
            case Type::Function: value.asFunction.~vector(); break;
            case Type::Pointer: value.asPointer.~vector(); break;
            case Type::String: value.asString.~vector(); break;
            default: break;
            }
        }

        void copyFrom(const Array& o) {
            type = o.type;
            switch(type) {
            case Type::Bool: new (&value.asBool) std::vector<Bool>(o.value.asBool); break;
            case Type::Int: new (&value.asInt) std::vector<Int>(o.value.asInt); break;
            case Type::Float: new (&value.asFloat) std::vector<Float>(o.value.asFloat); break;
            case Type::Function: new (&value.asFunction) std::vector<FunctionRef>(o.value.asFunction); break;
            case Type::Pointer: new (&value.asPointer) std::vector<void*>(o.value.asPointer); break;
            case Type::String: new (&value.asString) std::vector<std::string>(o.value.asString); break;
            default: break;
            }
        }

        void moveFrom(Array&& o) {
            type = o.type;
            switch(type) {
            case Type::Bool: new (&value.asBool) std::vector<Bool>(std::move(o.value.asBool)); break;
            case Type::Int: new (&value.asInt) std::vector<Int>(std::move(o.value.asInt)); break;
            case Type::Float: new (&value.asFloat) std::vector<Float>(std::move(o.value.asFloat)); break;
            case Type::Function: new (&value.asFunction) std::vector<FunctionRef>(std::move(o.value.asFunction)); break;
            case Type::Pointer: new (&value.asPointer) std::vector<void*>(std::move(o.value.asPointer)); break;
            case Type::String: new (&value.asString) std::vector<std::string>(std::move(o.value.asString)); break;
            default: break;
            }
        }

        Array() { type = Type::Int; new (&value.asInt) std::vector<Int>(); }
        Array(std::vector<Bool> a) { type = Type::Bool; new (&value.asBool) std::vector<Bool>(a); }
        Array(std::vector<Int> a) { type = Type::Int; new (&value.asInt) std::vector<Int>(a); }
        Array(std::vector<Float> a) { type = Type::Float; new (&value.asFloat) std::vector<Float>(a); }
        Array(std::vector<FunctionRef> a) { type = Type::Function; new (&value.asFunction) std::vector<FunctionRef>(a); }
        Array(std::vector<void*> a) { type = Type::Pointer; new (&value.asPointer) std::vector<void*>(a); }
        Array(std::vector<std::string> a) { type = Type::String; new (&value.asString) std::vector<std::string>(a); }
        
        Array(const Array& o) { copyFrom(o); }
        Array(Array&& o) noexcept { moveFrom(std::move(o)); }
        ~Array() { destroy(); }

        Array& operator=(const Array& o) {
            if (this != &o) {
                destroy();
                copyFrom(o);
            }
            return *this;
        }

        Array& operator=(Array&& o) noexcept {
            if (this != &o) {
                destroy();
                moveFrom(std::move(o));
            }
            return *this;
        }

        Type getType() const { return type; }

        template <typename T>
        std::vector<T>& getStdVector();

        bool operator==(const Array& o) const {
            if (size() != o.size() || getType() != o.getType()) return false;
            switch (getType()) {
            case Type::Bool: return value.asBool == o.value.asBool;
            case Type::Int: return value.asInt == o.value.asInt;
            case Type::Float: return value.asFloat == o.value.asFloat;
            case Type::String: return value.asString == o.value.asString;
            default: break;
            }
            return true;
        }

        bool operator!=(const Array& o) const { return !(*this == o); }

        size_t size() const {
            switch (getType()) {
            case Type::Bool: return value.asBool.size();
            case Type::Int: return value.asInt.size();
            case Type::Float: return value.asFloat.size();
            case Type::Function: return value.asFunction.size();
            case Type::Pointer: return value.asPointer.size();
            case Type::String: return value.asString.size();
            default: return 0;
            }
        }

        template <typename T>
        void insert(const std::vector<T>& bb) {
            auto& aa = getStdVector<T>();
            aa.insert(aa.end(), bb.begin(), bb.end());
        }

        template<typename T> void push_back(const T& t) { throw Exception("Unsupported type"); }

        void push_back(const Int& t) {
            if (getType() == Type::Null || getType() == Type::Int) value.asInt.push_back(t);
            else throw Exception("Unsupported type");
        }
        void push_back(const Float& t) {
            if (getType() == Type::Float) value.asFloat.push_back(t);
            else throw Exception("Unsupported type");
        }
        void push_back(const std::string& t) {
            if (getType() == Type::String) value.asString.push_back(t);
            else throw Exception("Unsupported type");
        }
        void push_back(const FunctionRef& t) {
            if (getType() == Type::Function) value.asFunction.push_back(t);
            else throw Exception("Unsupported type");
        }
        void push_back(void* t) {
            if (getType() == Type::Pointer) value.asPointer.push_back(t);
            else throw Exception("Unsupported type");
        }

        void push_back(const Array& barr) {
            if (getType() == barr.getType()) {
                switch (getType()) {
                case Type::Bool: insert(barr.value.asBool); break;
                case Type::Int: insert(barr.value.asInt); break;
                case Type::Float: insert(barr.value.asFloat); break;
                case Type::Function: insert(barr.value.asFunction); break;
                case Type::Pointer: insert(barr.value.asPointer); break;
                case Type::String: insert(barr.value.asString); break;
                default: break;
                }
            }
        }

        void pop_back() {
            switch (getType()) {
            case Type::Bool: value.asBool.pop_back(); break;
            case Type::Null:
            case Type::Int: value.asInt.pop_back(); break;
            case Type::Float: value.asFloat.pop_back(); break;
            case Type::Function: value.asFunction.pop_back(); break;
            case Type::Pointer: value.asPointer.pop_back(); break;
            case Type::String: value.asString.pop_back(); break;
            default: break;
            }
        }
    };

    template <> inline std::vector<Bool>& Array::getStdVector<Bool>() { return value.asBool; }
    template <> inline std::vector<Int>& Array::getStdVector<Int>() { return value.asInt; }
    template <> inline std::vector<Float>& Array::getStdVector<Float>() { return value.asFloat; }
    template <> inline std::vector<FunctionRef>& Array::getStdVector<FunctionRef>() { return value.asFunction; }
    template <> inline std::vector<void*>& Array::getStdVector<void*>() { return value.asPointer; }
    template <> inline std::vector<std::string>& Array::getStdVector<std::string>() { return value.asString; }

    using Dictionary = std::unordered_map<size_t, ValuePtr>;
    using DictionaryRef = std::shared_ptr<Dictionary>;

    using Set = std::unordered_set<ValuePtr>;
    using SetRef = std::shared_ptr<Set>;

    struct Scope;
    using ScopePtr = std::shared_ptr<Scope>;

    struct Class {
        std::string name;
        std::unordered_map<std::string, ValuePtr> variables;
        ScopePtr functionScope;
        std::mutex varInsert;
        std::vector<std::string> baseClasses;

        Class(const std::string& name_) : name(name_) {}
        Class(const std::string& name_, const std::unordered_map<std::string, ValuePtr>& variables_) : name(name_), variables(variables_)
        {
            int a = 0;
        }
        Class(const Class& o);
        Class(const ScopePtr& o);
        ~Class();

        ValuePtr& insertVar(const std::string& n, ValuePtr val) {
            auto l = std::unique_lock(varInsert);
            auto& ref = variables[n];
            ref = val;
            return ref;
        }
    };

    using ClassRef = std::shared_ptr<Class>;
	
	enum class OperatorPrecedence : int {
		assign = 0,
        boolean,
		compare,
		range,
		addsub,
		muldiv,
		incdec,
		func
	};
	
    using Lambda = std::function<ValuePtr(const List&)>;
    using ScopedLambda = std::function<ValuePtr(ScopePtr, const List&)>;
    using ClassLambda = std::function<ValuePtr(Class*, ScopePtr, const List&)>;


    enum class FunctionType : uint8_t {
        Common,
        Constructor,
        Member,
        Operator
    };

    struct Expression;
    using ExpressionPtr = Expression*;

    using FunctionBodyVariant =
        std::variant<
        std::vector<ExpressionPtr>,
        Lambda,
        ScopedLambda,
        ClassLambda
        >;

    enum class FunctionBodyType : uint8_t {
        Subexpressions,
        Lambda,
        ScopedLambda,
        ClassLambda
    };

    // Range value type: start..end (exclusive) or start..=end (inclusive)
    struct RangeValue {
        Int start = 0;
        Int end_ = 0;   // end_ to avoid conflict with std::end
        bool inclusive = false;  // false = .., true = ..=
    };
	
	struct Function {
        OperatorPrecedence opPrecedence;
        FunctionType type = FunctionType::Common;
        std::string name;
        std::vector<std::string> argNames;
        std::vector<TypeDescriptor> argTypes;
        std::map<std::string, TypeDescriptor> types;
		std::map<std::string, ExpressionPtr> defValues;
        std::optional<TypeDescriptor> returnType;
        bool variableArgsParam = false;
        bool isCoro = false;
        bool isSuspending = false;  // true for coro/async functions (isCoro implies isSuspending)
        std::optional<TypeDescriptor> variableArgsParamType;
        std::vector<std::string> genericParams;
        std::string genericBodyRaw;

        struct CaptureEntry {
            enum class Kind { Ref, Copy } kind;
            ValuePtr value;  // Ref: shared ptr aliasing outer; Copy: owned snapshot
        };
        std::map<std::string, CaptureEntry> captures;  // key = name visible in body

        FunctionBodyVariant body;

        FunctionBodyType getBodyType() {
            return static_cast<FunctionBodyType>(body.index());
        }

		static OperatorPrecedence getPrecedence(const std::string& n) {
			if (n == ".." || n == "..=") {
				return OperatorPrecedence::range;
			}
			if (n.size() > 2) {
				return OperatorPrecedence::func;
			}
            if (n == "||" || n == "&&") {
                return OperatorPrecedence::boolean;
            }
            if (n == "++" || n == "--") {
                return OperatorPrecedence::incdec;
            }
			if (contains("!<>|&"s, n[0]) || n == "==") {
				return OperatorPrecedence::compare;
			}
			if (contains(n, '=')) {
				return OperatorPrecedence::assign;
			}
			if (contains("/*%"s, n[0])) {
				return OperatorPrecedence::muldiv;
			}
			if (contains("-+"s, n[0])) {
				return OperatorPrecedence::addsub;
			}
			return OperatorPrecedence::func;
		}

		Function(const std::string& name_, const Lambda& l)
            : name(name_), opPrecedence(getPrecedence(name_)), body(l) {}
        Function(const std::string& name_, const ScopedLambda& l)
            : name(name_), opPrecedence(getPrecedence(name_)), body(l) {}
        Function(const std::string& name_, const ClassLambda& l)
            : name(name_), opPrecedence(getPrecedence(name_)), body(l) {}
		Function(const std::string& name_, 
            const std::vector<std::string>& argNames_, 
            const std::map<std::string, TypeDescriptor>& types_,
            const std::map<std::string, ExpressionPtr>& defValues_,
            const std::vector<ExpressionPtr>& body_, FunctionType type = FunctionType::Common)
			: name(name_), body(body_),
			argNames(argNames_),
            types(types_),
            defValues(defValues_),
			opPrecedence(OperatorPrecedence::func) {}
        Function(const std::string& name_, 
            const std::vector<std::string>& argNames_,
            const std::map<std::string, TypeDescriptor>& types_,
            const std::map<std::string, ExpressionPtr>& defValues_, FunctionType type = FunctionType::Common):
			Function(name_, argNames_, types_, defValues_, {}, type) {}
        Function(const std::string& name);
		Function() : name("__anon") {}
        Function(const Function& o) = default;
	};


    // Task (was Coro) is now in concurrency/task.hpp
    // Backward-compat: Coro = Task, CoroRef = TaskRef (defined in task.hpp)
}
