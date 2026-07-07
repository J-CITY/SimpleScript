#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include "cancellation.hpp"

namespace IkigaiScript {

struct Function;
using FunctionRef = std::shared_ptr<Function>;

struct Value;
using ValuePtr = std::shared_ptr<Value>;

struct Scope;
using ScopePtr = std::shared_ptr<Scope>;

struct Class;

enum class TaskState : uint8_t {
    Created,    // constructed, not yet started
    Running,    // currently inside callTask
    Suspended,  // waiting for a child / yield issued
    Completed,  // return executed (or body exhausted)
    Cancelled,  // cancelled via token
};

inline const char* taskStateName(TaskState s) {
    switch (s) {
    case TaskState::Created:    return "Created";
    case TaskState::Running:    return "Running";
    case TaskState::Suspended:  return "Suspended";
    case TaskState::Completed:  return "Completed";
    case TaskState::Cancelled:  return "Cancelled";
    }
    return "Unknown";
}

struct Task : std::enable_shared_from_this<Task> {
    // Identity
    std::string id;             // unique name used for the scope frame

    // Execution context (set at creation time, stable)
    FunctionRef func;
    ScopePtr    scope;
    Class*      classs  = nullptr;

    // Program counter (expressionId in coro terms)
    int pc = 0;

    // State machine
    TaskState state = TaskState::Created;

    // Yield / result payload
    ValuePtr suspendPayload;    // value produced by the last yield
    ValuePtr result;            // final return value when Completed

    // Structured concurrency links
    std::weak_ptr<Task> awaitingTask;   // child task we're currently waiting for
    std::weak_ptr<Task> parent;         // our parent task (if any)
    std::vector<std::shared_ptr<Task>> children;  // owned child tasks

    // Cancellation
    CancellationToken::Ptr token;
    bool isNativeJob = false;

    bool isActive() const noexcept {
        return state == TaskState::Created ||
               state == TaskState::Running ||
               state == TaskState::Suspended;
    }

    // Deprecation shim for old Coro::expressionId
    int& expressionId() noexcept { return pc; }
};

using TaskRef = std::shared_ptr<Task>;

using Coro = Task;
using CoroRef = TaskRef;

}  // namespace IkigaiScript
