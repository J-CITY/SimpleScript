#pragma once
// NativeBox<T>: stores an owned C++ object inside a script Class instance.
//
// Use when T has opaque internal state that cannot be reconstructed from
// public fields alone. For simple value-type structs (Vec2, Color, etc.)
// prefer the field-mirroring approach in NativeClassBuilder instead.
//
// The box is stored in Class::variables["__native"] as a Type::Pointer.
// Lifetime: constructed in a ctor ClassLambda, deleted in a ~ClassName
// ClassLambda which is automatically registered by NativeClassBuilder::finalize().

#include "../types.hpp"
#include "../value.hpp"
#include "../scope.hpp"

namespace IkigaiScript {
namespace Native {

template<typename T>
struct NativeBox {
    T obj;
    explicit NativeBox(T o) : obj(std::move(o)) {}
    // Non-copyable; each script instance owns exactly one box.
    NativeBox(const NativeBox&) = delete;
    NativeBox& operator=(const NativeBox&) = delete;
};

// Retrieve a pointer to the stored T from a Class instance.
// Returns nullptr if the instance has no "__native" field.
template<typename T>
T* getNativePtr(Class* inst) {
    auto it = inst->variables.find("__native");
    if (it == inst->variables.end() || it->second->getType() != Type::Pointer)
        return nullptr;
    return &static_cast<NativeBox<T>*>(it->second->getPointer())->obj;
}

// Store a new T in the Class instance, replacing any existing box.
template<typename T>
void setNativePtr(Class* inst, T obj) {
    auto* prev = getNativePtr<T>(inst);
    if (prev) {
        *prev = std::move(obj);
        return;
    }
    auto* box = new NativeBox<T>(std::move(obj));
    inst->variables["__native"] = std::make_shared<Value>(static_cast<void*>(box));
}

// Delete the NativeBox; called from the destructor ClassLambda.
template<typename T>
void deleteNativePtr(Class* inst) {
    auto it = inst->variables.find("__native");
    if (it != inst->variables.end() && it->second->getType() == Type::Pointer) {
        delete static_cast<NativeBox<T>*>(it->second->getPointer());
        it->second = std::make_shared<Value>();  // zero the pointer
    }
}

} // namespace Native
} // namespace IkigaiScript
