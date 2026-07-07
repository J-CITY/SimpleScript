#pragma once
#include <vector>
#include <memory>
#include "chunk.hpp"
#include "../types.hpp"
#include "../scope.hpp"

namespace IkigaiScript {

class IkigaiScriptInterpreter;

// One activation record on the call stack.
struct CallFrame {
    BytecodeFunctionRef fn;
    size_t ip         = 0;   // instruction pointer (byte offset into fn->code)
    size_t stackBase  = 0;   // first stack slot belonging to this frame
    ScopePtr scope;
    Class* classs     = nullptr;
};

// Stack-based virtual machine for executing compiled BytecodeFunction objects.
//
// The VM borrows all runtime helpers from IkigaiScriptInterpreter:
//   resolveVariable / resolveVariableForWrite
//   callFunction (for C++ builtins and AST-body fallback)
//   commitTransaction / rollbackTransaction
//   registerDefer / runDefers
//   dependencyManager
//
// This keeps a single source of truth for runtime semantics and allows
// seamless mixing of interpreted and compiled functions.
class VM {
public:
    explicit VM(IkigaiScriptInterpreter* interp);

    // Execute a top-level Chunk in the given scope.
    ValuePtr runChunk(ChunkRef chunk, ScopePtr scope);

    // Execute a single BytecodeFunction (used by callFunction Bytecode path).
    ValuePtr runFunction(BytecodeFunctionRef fn, ScopePtr scope,
                         const List& args, Class* classs = nullptr);

    // Resume a suspended coroutine (Task) that has a bytecode body.
    // Returns the yield value if suspended again, or the return value if done.
    ValuePtr resumeCoro(TaskRef task);

private:
    IkigaiScriptInterpreter* interp_;
    std::vector<ValuePtr> stack_;
    std::vector<CallFrame> frames_;

    // Stack helpers
    void push(ValuePtr v);
    ValuePtr pop();
    ValuePtr& peek(size_t depth = 0);

    // Main dispatch loop — runs until frames_.size() drops to stopDepth.
    // Passing stopDepth = frames_.size() - 1 makes it execute exactly one
    // call frame (the top one), so nested calls via runFunction don't consume
    // the caller's frames.
    ValuePtr execute(size_t stopDepth = 0);

    // Read helpers (advances ip inside frame)
    uint8_t  readByte(CallFrame& f);
    uint16_t readU16(CallFrame& f);
    int16_t  readI16(CallFrame& f);

    // Runtime helpers
    ValuePtr callValue(ValuePtr callee, size_t argc, ScopePtr scope, Class* classs);
    void defineVar(const std::string& name, ValuePtr val, bool isConst, ScopePtr scope);
    bool isTruthy(const ValuePtr& v) const;
};

} // namespace IkigaiScript
