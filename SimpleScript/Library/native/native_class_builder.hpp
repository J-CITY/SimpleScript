#pragma once
// NativeClassBuilder<T> — fluent API for registering a C++ struct/class as a
// SimpleScript class.
//
// Usage:
//   #include "Library/native/native_class_builder.hpp"
//
//   IkigaiScript::Native::NativeClassBuilder<Vec2>(interp, "Vec2")
//       .member("x", &Vec2::x)
//       .member("y", &Vec2::y)
//       .constructor<float, float>()      // Vec2(float, float)
//       .constructor<>()                  // Vec2()
//       .method("sum",   &Vec2::sum)
//       .operator_("+",  &Vec2::operator+)
//       .operator_("*",  [](const Vec2& a, float s){ return a * s; })
//       .finalize();
//
// The finalize() call registers everything with the interpreter and seals the
// builder. Do not call finalize() more than once.
//
// Storage model
// -------------
// Public fields (member()) are stored as Value entries in Class::variables and
// kept in sync with the C++ struct. Before each method call the struct is
// reconstructed from variables; after a mutating call the variables are updated
// from the struct. This approach is ideal for small value types. For large
// objects with opaque internal state, use NativeBox (native_box.hpp) instead.

#include "../ikigaiScript.h"
#include "native_traits.hpp"
#include <functional>
#include <vector>
#include <string>
#include <memory>
#include <type_traits>

namespace IkigaiScript {
namespace Native {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Extract one argument from a ValuePtr. For the native class type T, reconstruct
// from its Class::variables using the provided makeT function.
template<typename T, typename Arg>
auto extractArg(const ValuePtr& v,
                const std::shared_ptr<std::function<T(Class*)>>& makeT)
    -> std::remove_cvref_t<Arg>
{
    using R = std::remove_cvref_t<Arg>;
    if constexpr (std::is_same_v<R, T>) {
        return (*makeT)(v->getClass().get());
    } else {
        return Traits<R>::fromValue(v);
    }
}

// Wrap a return value as a ValuePtr.
template<typename T, typename Ret>
ValuePtr wrapReturn(Ret&& val,
                    const std::shared_ptr<std::function<ValuePtr(const T&)>>& wrapT)
{
    using R = std::remove_cvref_t<Ret>;
    if constexpr (std::is_void_v<R>) {
        return std::make_shared<Value>();
    } else if constexpr (std::is_same_v<R, T>) {
        return (*wrapT)(std::forward<Ret>(val));
    } else {
        return Traits<R>::toValue(std::forward<Ret>(val));
    }
}

// Build a std::function that constructs T from a List by forwarding args.
template<typename T, typename... Args, size_t... I>
std::function<T(const List&)> makeCtorFn(std::index_sequence<I...>) {
    return [](const List& args) -> T {
        return T(Traits<std::remove_cvref_t<Args>>::fromValue(args[I])...);
    };
}

// Build a std::function that invokes a const member pointer and wraps the result.
template<typename T, typename Ret, typename... Args, size_t... I>
std::function<ValuePtr(Class*, const List&,
                       const std::shared_ptr<std::function<T(Class*)>>&,
                       const std::shared_ptr<std::function<ValuePtr(const T&)>>&)>
makeConstMethodFn(Ret (T::*fn)(Args...) const, std::index_sequence<I...>) {
    return [fn](Class* inst, const List& args,
                const std::shared_ptr<std::function<T(Class*)>>& mkT,
                const std::shared_ptr<std::function<ValuePtr(const T&)>>& wT) -> ValuePtr {
        T self = (*mkT)(inst);
        if constexpr (std::is_void_v<Ret>) {
            (self.*fn)(extractArg<T, Args>(args[I], mkT)...);
            return std::make_shared<Value>();
        } else {
            auto result = (self.*fn)(extractArg<T, Args>(args[I], mkT)...);
            return wrapReturn<T>(std::move(result), wT);
        }
    };
}

// Build a std::function that invokes a non-const member pointer, then writes
// back the (possibly mutated) self to the instance variables.
template<typename T, typename Ret, typename... Args, size_t... I>
std::function<ValuePtr(Class*, const List&,
                       const std::shared_ptr<std::function<T(Class*)>>&,
                       const std::shared_ptr<std::function<ValuePtr(const T&)>>&,
                       const std::shared_ptr<std::vector<std::function<void(const T&, Class*)>>>&)>
makeMutMethodFn(Ret (T::*fn)(Args...), std::index_sequence<I...>) {
    return [fn](Class* inst, const List& args,
                const std::shared_ptr<std::function<T(Class*)>>& mkT,
                const std::shared_ptr<std::function<ValuePtr(const T&)>>& wT,
                const std::shared_ptr<std::vector<std::function<void(const T&, Class*)>>>& storeFields)
        -> ValuePtr
    {
        T self = (*mkT)(inst);
        ValuePtr result;
        if constexpr (std::is_void_v<Ret>) {
            (self.*fn)(extractArg<T, Args>(args[I], mkT)...);
            result = std::make_shared<Value>();
        } else {
            auto r = (self.*fn)(extractArg<T, Args>(args[I], mkT)...);
            result = wrapReturn<T>(std::move(r), wT);
        }
        // Write modified self back to instance variables
        if (storeFields && !storeFields->empty()) {
            for (auto& sf : *storeFields) sf(self, inst);
        }
        return result;
    };
}

// Tag type: carries a type list without ever being constructed (avoids issues
// with reference-typed elements in std::tuple that can't be default-constructed).
template<typename... Args>
struct TypeList {};

// Convert std::tuple<Args...> → TypeList<Args...>
template<typename Tuple>
struct TupleToTypeList;
template<typename... Args>
struct TupleToTypeList<std::tuple<Args...>> { using type = TypeList<Args...>; };
template<typename Tuple>
using TupleToTypeList_t = typename TupleToTypeList<Tuple>::type;

// Invoke a free callable with all args sourced from the List.
// Used by operator_ (free function) and staticMethod.
template<typename T, typename Fn2, typename... Args, size_t... I>
ValuePtr invokeFreeFn(
    Fn2&& fn,
    const List& args,
    const std::shared_ptr<std::function<T(Class*)>>& mkT,
    const std::shared_ptr<std::function<ValuePtr(const T&)>>& wT,
    std::index_sequence<I...>,
    TypeList<Args...> /*tag*/)
{
    using R = std::invoke_result_t<Fn2, std::remove_cvref_t<Args>...>;
    if constexpr (std::is_void_v<R>) {
        std::forward<Fn2>(fn)(extractArg<T, Args>(args[I], mkT)...);
        return std::make_shared<Value>();
    } else {
        auto result = std::forward<Fn2>(fn)(extractArg<T, Args>(args[I], mkT)...);
        return wrapReturn<T>(std::move(result), wT);
    }
}

// Deduce argument types from a callable's signature via partial specialization.
template<typename F>
struct FnTraits;

template<typename Ret, typename... Args>
struct FnTraits<Ret(*)(Args...)> {
    using RetType = Ret;
    using ArgsTuple = std::tuple<Args...>;  // may contain refs — use TypeList for tagging
    static constexpr size_t arity = sizeof...(Args);
};
template<typename Ret, typename Cls, typename... Args>
struct FnTraits<Ret(Cls::*)(Args...) const> {
    using RetType = Ret;
    using ClassType = Cls;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
};
template<typename Ret, typename Cls, typename... Args>
struct FnTraits<Ret(Cls::*)(Args...)> {
    using RetType = Ret;
    using ClassType = Cls;
    using ArgsTuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);
};
// Lambda / std::function: strip the call operator
template<typename F>
struct FnTraits : FnTraits<decltype(&F::operator())> {};
template<typename Ret, typename Cls, typename... Args>
struct FnTraits<Ret(Cls::*)(Args...) const noexcept> : FnTraits<Ret(Cls::*)(Args...) const> {};


// ---------------------------------------------------------------------------
// NativeClassBuilder<T>
// ---------------------------------------------------------------------------

template<typename T>
class NativeClassBuilder {
public:
    NativeClassBuilder(IkigaiScriptInterpreter& interp, const std::string& name, ScopePtr scope = nullptr)
        : interp_(interp)
        , className_(name)
        , parentScope_(scope ? scope : interp.getGlobalScope())
        , makeTPtr_(std::make_shared<std::function<T(Class*)>>())
        , wrapTPtr_(std::make_shared<std::function<ValuePtr(const T&)>>())
        , storeFieldsPtr_(std::make_shared<std::vector<std::function<void(const T&, Class*)>>>())
    {}

    // ------------------------------------------------------------------
    // member(name, &T::field)
    // ------------------------------------------------------------------
    template<typename Field>
    NativeClassBuilder& member(const std::string& name, Field T::* ptr) {
        using F = std::remove_cvref_t<Field>;
        FieldEntry fe;
        fe.name = name;
        fe.typeDesc = Traits<F>::typeDesc();
        fe.defaultValue = Traits<F>::toValue(F{});
        fe.applyToInst = [ptr, name](const T& obj, Class* inst) {
            inst->variables[name] = Traits<F>::toValue(obj.*ptr);
        };
        fe.applyFromInst = [ptr, name](T& obj, Class* inst) {
            auto it = inst->variables.find(name);
            if (it != inst->variables.end() && it->second)
                obj.*ptr = Traits<F>::fromValue(it->second);
        };
        fields_.push_back(std::move(fe));
        return *this;
    }

    // ------------------------------------------------------------------
    // constructor<Args...>()  — constructs T(Args...) from script args
    // constructor(factory)    — custom factory: List → T
    // ------------------------------------------------------------------
    template<typename... Args>
    NativeClassBuilder& constructor() {
        auto ctorFn = makeCtorFn<T, Args...>(std::index_sequence_for<Args...>{});
        std::vector<TypeDescriptor> argTypes = { Traits<std::remove_cvref_t<Args>>::typeDesc()... };
        std::vector<std::string> argNames;
        for (size_t i = 0; i < sizeof...(Args); i++)
            argNames.push_back("_a" + std::to_string(i));
        auto storeHook = storeFieldsPtr_;
        ClassLambda body = [ctorFn, storeHook](Class* inst, ScopePtr, const List& args) -> ValuePtr {
            T obj = ctorFn(args);
            for (auto& sf : *storeHook) sf(obj, inst);
            return nullptr;
        };
        ctors_.push_back({argNames, std::move(body), std::move(argTypes)});
        return *this;
    }

    template<typename Fn>
    NativeClassBuilder& constructor(Fn factory) {
        auto storeHook = storeFieldsPtr_;
        ClassLambda body = [factory, storeHook](Class* inst, ScopePtr, const List& args) -> ValuePtr {
            T obj = factory(args);
            for (auto& sf : *storeHook) sf(obj, inst);
            return nullptr;
        };
        ctors_.push_back({{}, std::move(body), {}});
        return *this;
    }

    // ------------------------------------------------------------------
    // method(name, &T::fn)   — const member function
    // ------------------------------------------------------------------
    template<typename Ret, typename... Args>
    NativeClassBuilder& method(const std::string& name, Ret (T::*fn)(Args...) const) {
        std::vector<TypeDescriptor> argTypes = buildMethodArgTypes<Args...>();
        auto invoker = makeConstMethodFn<T>(fn, std::index_sequence_for<Args...>{});
        auto mkT = makeTPtr_;
        auto wT  = wrapTPtr_;
        ClassLambda body = [invoker, mkT, wT](Class* inst, ScopePtr, const List& args) -> ValuePtr {
            return invoker(inst, args, mkT, wT);
        };
        methods_.push_back({name, std::move(body), std::move(argTypes)});
        return *this;
    }

    // method(name, &T::fn)   — non-const member function (writes back)
    template<typename Ret, typename... Args>
    NativeClassBuilder& method(const std::string& name, Ret (T::*fn)(Args...)) {
        std::vector<TypeDescriptor> argTypes = buildMethodArgTypes<Args...>();
        auto invoker = makeMutMethodFn<T>(fn, std::index_sequence_for<Args...>{});
        auto mkT    = makeTPtr_;
        auto wT     = wrapTPtr_;
        auto store  = storeFieldsPtr_;
        ClassLambda body = [invoker, mkT, wT, store](Class* inst, ScopePtr, const List& args) -> ValuePtr {
            return invoker(inst, args, mkT, wT, store);
        };
        methods_.push_back({name, std::move(body), std::move(argTypes)});
        return *this;
    }

    // staticMethod — a free callable registered as a class method
    template<typename Fn>
    NativeClassBuilder& staticMethod(const std::string& name, Fn fn) {
        using FnClean = std::remove_reference_t<Fn>;
        using Tr = FnTraits<FnClean>;
        auto mkT = makeTPtr_;
        auto wT  = wrapTPtr_;
        ClassLambda body = [fn, mkT, wT](Class*, ScopePtr, const List& args) -> ValuePtr {
            using FnT = std::remove_reference_t<decltype(fn)>;
            using Tr2 = FnTraits<FnT>;
            return invokeFreeFn<T>(fn, args, mkT, wT,
                std::make_index_sequence<Tr2::arity>{},
                TupleToTypeList_t<typename Tr2::ArgsTuple>{});
        };
        methods_.push_back({name, std::move(body), {}});
        return *this;
    }

    // ------------------------------------------------------------------
    // operator_(op, &T::fn)   — member function binary operator
    // ------------------------------------------------------------------
    template<typename Ret, typename... Args>
    NativeClassBuilder& operator_(const std::string& op, Ret (T::*fn)(Args...) const) {
        // Binary operator: args[0] = lhs (T), args[1..] = method params
        std::vector<TypeDescriptor> argTypes = buildOpArgTypes<Args...>();
        auto mkT = makeTPtr_;
        auto wT  = wrapTPtr_;
        auto invoker = makeConstMethodFn<T>(fn, std::index_sequence_for<Args...>{});
        ClassLambda body = [invoker, mkT, wT](Class* inst, ScopePtr, const List& args) -> ValuePtr {
            // For operators called as infix: args[0] = lhs, args[1..] = rhs
            auto lhsInst = args[0]->getClass().get();
            List methodArgs(args.begin() + 1, args.end());
            return invoker(lhsInst, methodArgs, mkT, wT);
        };
        ops_.push_back({op, std::move(body), std::move(argTypes)});
        return *this;
    }

    // operator_(op, lambda)   — free function operator (all args from script)
    template<typename Fn,
             typename = std::enable_if_t<!std::is_member_function_pointer_v<std::decay_t<Fn>>>>
    NativeClassBuilder& operator_(const std::string& op, Fn fn) {
        auto argTypes = buildFreeOpArgTypes<std::remove_reference_t<Fn>>();
        auto mkT = makeTPtr_;
        auto wT  = wrapTPtr_;
        ClassLambda body = [fn, mkT, wT](Class*, ScopePtr, const List& args) -> ValuePtr {
            using FnT = std::remove_reference_t<decltype(fn)>;
            using Tr2 = FnTraits<FnT>;
            return invokeFreeFn<T>(fn, args, mkT, wT,
                std::make_index_sequence<Tr2::arity>{},
                TupleToTypeList_t<typename Tr2::ArgsTuple>{});
        };
        ops_.push_back({op, std::move(body), std::move(argTypes)});
        return *this;
    }

    // ------------------------------------------------------------------
    // finalize() — register everything with the interpreter
    // ------------------------------------------------------------------
    FunctionRef finalize() {
        // Populate the shared field-store/reconstruct functions now that all
        // fields are known.
        auto fieldsCopy = fields_;
        *storeFieldsPtr_ = buildStoreFields(fieldsCopy);

        *makeTPtr_ = [fieldsCopy](Class* inst) -> T {
            T obj{};
            for (auto& fe : fieldsCopy) fe.applyFromInst(obj, inst);
            return obj;
        };

        // Build the class scope
        auto classScope = interp_.newClassScope(className_, parentScope_);

        // Register field default values
        for (auto& fe : fields_) {
            classScope->variables[fe.name] = fe.defaultValue;
        }

        // Populate wrapT now that we have the class scope
        *wrapTPtr_ = [classScope, fieldsCopy, className = className_](const T& obj) -> ValuePtr {
            auto inst = std::make_shared<Class>(classScope);
            // Override default values with actual values from obj
            for (auto& fe : fieldsCopy) fe.applyToInst(obj, inst.get());
            auto val = std::make_shared<Value>(inst);
            val->typeDescriptor.type = Type::Class;
            val->typeDescriptor.subtype = className;
            val->typeDescriptor.isInit = true;
            return val;
        };

        // Register methods
        for (auto& m : methods_) {
            interp_.newFunction(m.name, classScope, m.body);
        }

        // Register operators on the class scope
        for (auto& op : ops_) {
            auto fn = std::make_shared<Function>(op.name, op.body);
            fn->type = FunctionType::Operator;
            fn->argNames.reserve(op.argTypes.size());
            for (size_t i = 0; i < op.argTypes.size(); i++)
                fn->argNames.push_back("_op" + std::to_string(i));
            fn->argTypes = op.argTypes;
            classScope->operators[op.name].push_back(fn);
        }

        interp_.closeScope(classScope, false);

        // Register constructors
        FunctionRef primaryCtor;
        if (ctors_.empty()) {
            // Implicit default constructor
            auto storeHook = storeFieldsPtr_;
            ClassLambda defaultBody = [storeHook](Class* inst, ScopePtr, const List&) -> ValuePtr {
                T obj{};
                for (auto& sf : *storeHook) sf(obj, inst);
                return nullptr;
            };
            auto fn = std::make_shared<Function>(className_, defaultBody);
            primaryCtor = interp_.newConstructor(className_, parentScope_, fn);
        } else {
            bool first = true;
            for (auto& c : ctors_) {
                auto fn = std::make_shared<Function>(className_, c.body);
                fn->argNames = c.argNames;
                fn->argTypes = c.argTypes;
                if (first) {
                    primaryCtor = interp_.newConstructor(className_, parentScope_, fn);
                    first = false;
                } else {
                    interp_.newConstructorOverload(className_, parentScope_, fn);
                }
            }
        }

        // Register destructor (~ClassName) to clean up any NativeBox
        auto dtorName = "~" + className_;
        ClassLambda dtorBody = [](Class*, ScopePtr, const List&) -> ValuePtr {
            return std::make_shared<Value>();
        };
        interp_.newFunction(dtorName,
            interp_.resolveScope(className_, parentScope_), dtorBody);

        return primaryCtor;
    }

private:
    // ------------------------------------------------------------------
    // Internal data
    // ------------------------------------------------------------------
    struct FieldEntry {
        std::string name;
        TypeDescriptor typeDesc;
        ValuePtr defaultValue;
        std::function<void(const T&, Class*)> applyToInst;    // T  → variables
        std::function<void(T&, Class*)>       applyFromInst;  // variables → T
    };
    struct MethodEntry {
        std::string name;
        ClassLambda body;
        std::vector<TypeDescriptor> argTypes;
    };
    struct CtorEntry {
        std::vector<std::string>  argNames;
        ClassLambda               body;
        std::vector<TypeDescriptor> argTypes;
    };
    struct OpEntry {
        std::string name;
        ClassLambda body;
        std::vector<TypeDescriptor> argTypes;
    };

    IkigaiScriptInterpreter& interp_;
    std::string               className_;
    ScopePtr                  parentScope_;

    std::shared_ptr<std::function<T(Class*)>>            makeTPtr_;
    std::shared_ptr<std::function<ValuePtr(const T&)>>   wrapTPtr_;
    std::shared_ptr<std::vector<std::function<void(const T&, Class*)>>> storeFieldsPtr_;

    std::vector<FieldEntry>  fields_;
    std::vector<MethodEntry> methods_;
    std::vector<CtorEntry>   ctors_;
    std::vector<OpEntry>     ops_;

    // ------------------------------------------------------------------
    // Helpers to build TypeDescriptor vectors
    // ------------------------------------------------------------------
    template<typename Arg>
    TypeDescriptor argTypeDesc() {
        using R = std::remove_cvref_t<Arg>;
        if constexpr (std::is_same_v<R, T>) {
            TypeDescriptor td;
            td.type = Type::Class;
            td.subtype = className_;
            td.isInit = true;
            return td;
        } else {
            return Traits<R>::typeDesc();
        }
    }

    template<typename... Args>
    std::vector<TypeDescriptor> buildMethodArgTypes() {
        return { argTypeDesc<Args>()... };
    }

    // Operator arg types: lhs is T, then the method's explicit params
    template<typename... Args>
    std::vector<TypeDescriptor> buildOpArgTypes() {
        TypeDescriptor lhsTd;
        lhsTd.type = Type::Class; lhsTd.subtype = className_; lhsTd.isInit = true;
        std::vector<TypeDescriptor> v = { lhsTd };
        auto rhsTypes = buildMethodArgTypes<Args...>();
        v.insert(v.end(), rhsTypes.begin(), rhsTypes.end());
        return v;
    }

    // Operator arg types from a free callable (all args visible, uses TypeList tag)
    template<typename FnClean>
    std::vector<TypeDescriptor> buildFreeOpArgTypes() {
        using Tr = FnTraits<FnClean>;
        return buildFreeOpArgTypesFromTypeList(TupleToTypeList_t<typename Tr::ArgsTuple>{});
    }

    template<typename... Args>
    std::vector<TypeDescriptor> buildFreeOpArgTypesFromTypeList(TypeList<Args...>) {
        return { argTypeDesc<Args>()... };
    }

    // Build the per-field store functions after all fields are known
    static std::vector<std::function<void(const T&, Class*)>>
    buildStoreFields(const std::vector<FieldEntry>& fields) {
        std::vector<std::function<void(const T&, Class*)>> out;
        out.reserve(fields.size());
        for (auto& fe : fields) out.push_back(fe.applyToInst);
        return out;
    }
};

} // namespace Native
} // namespace IkigaiScript
