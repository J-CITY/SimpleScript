#pragma once
// Type-marshaling traits for native C++ ↔ SimpleScript Value conversion.
// Specialize NativeTraits<T> to teach the builder how to convert your types.

#include "../types.hpp"
#include "../value.hpp"

namespace IkigaiScript {
namespace Native {

// Primary template — triggers a static_assert if an unsupported type is used.
template<typename T, typename = void>
struct Traits {
    static_assert(sizeof(T) == 0,
        "NativeTraits not specialized for this type. "
        "Add a specialization in native_traits.hpp or before including native_class_builder.hpp.");
};

// --- bool ---
template<>
struct Traits<bool> {
    static ValuePtr toValue(bool v) { return std::make_shared<Value>(Int(v ? 1 : 0)); }
    static bool fromValue(const ValuePtr& v) { return v->getBool(); }
    static TypeDescriptor typeDesc() {
        TypeDescriptor td; td.type = Type::Bool; td.isInit = true; return td;
    }
};

// --- int (32-bit) ---
template<>
struct Traits<int> {
    static ValuePtr toValue(int v) { return std::make_shared<Value>(Int(v)); }
    static int fromValue(const ValuePtr& v) { return static_cast<int>(v->getInt()); }
    static TypeDescriptor typeDesc() {
        TypeDescriptor td; td.type = Type::Int; td.isInit = true; return td;
    }
};

// --- long long (Int = int64_t on 64-bit) ---
template<>
struct Traits<long long> {
    static ValuePtr toValue(long long v) { return std::make_shared<Value>(Int(v)); }
    static long long fromValue(const ValuePtr& v) { return v->getInt(); }
    static TypeDescriptor typeDesc() {
        TypeDescriptor td; td.type = Type::Int; td.isInit = true; return td;
    }
};

// --- float ---
template<>
struct Traits<float> {
    static ValuePtr toValue(float v) { return std::make_shared<Value>(Float(v)); }
    static float fromValue(const ValuePtr& v) { return static_cast<float>(v->getFloat()); }
    static TypeDescriptor typeDesc() {
        TypeDescriptor td; td.type = Type::Float; td.isInit = true; return td;
    }
};

// --- double (Float = double on 64-bit) ---
template<>
struct Traits<double> {
    static ValuePtr toValue(double v) { return std::make_shared<Value>(Float(v)); }
    static double fromValue(const ValuePtr& v) { return v->getFloat(); }
    static TypeDescriptor typeDesc() {
        TypeDescriptor td; td.type = Type::Float; td.isInit = true; return td;
    }
};

// --- std::string ---
template<>
struct Traits<std::string> {
    static ValuePtr toValue(const std::string& v) { return std::make_shared<Value>(v); }
    static std::string fromValue(const ValuePtr& v) { return v->getString(); }
    static TypeDescriptor typeDesc() {
        TypeDescriptor td; td.type = Type::String; td.isInit = true; return td;
    }
};

// Strip const/reference so callers can use Traits<const float&> etc.
template<typename U>
struct Traits<const U&> : Traits<U> {};

template<typename U>
struct Traits<U&> : Traits<U> {};

template<typename U>
struct Traits<U&&> : Traits<U> {};

template<typename U>
struct Traits<const U> : Traits<U> {};

} // namespace Native
} // namespace IkigaiScript
