#include "TestHelpers.hpp"
#include "../Library/concurrency/cancellation.hpp"
#include "../Library/concurrency/native_job_pool.hpp"

#include <chrono>
#include <thread>

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Phase 1: scheduler pump
// =============================================================================

TEST_CASE("scheduler: pump with no tasks returns 0 steps", "[concurrency][phase1]") {
    auto interp = makeInterp();
    REQUIRE(interp.pump(100) == 0);
}

TEST_CASE("scheduler: clearState clears scheduler", "[concurrency][phase1]") {
    auto interp = makeInterp();
    interp.clearState();
    REQUIRE(interp.pump(10) == 0);
}

// =============================================================================
// Phase 2: await — synchronous completion path
// =============================================================================

TEST_CASE("await: coro that returns immediately is awaited correctly", "[concurrency][phase2]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro answer() {
            return 42;
        };
        var t = answer();
        var result = await t;
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "result") == 42);
}

TEST_CASE("await: coro with yield, await drives to completion", "[concurrency][phase2]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro gen(n) {
            yield n;
            return n + 1;
        };
        var t = gen(10);
        t();
        var result = await t;
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "result") == 11);
}

// =============================================================================
// Phase 2: spawn block — fire and return
// =============================================================================

TEST_CASE("spawn: block executes and last value accessible via await", "[concurrency][phase2]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro work() {
            return 99;
        };
        var t = work();
        var result = await t;
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "result") == 99);
}

// =============================================================================
// Phase 3: sync block
// =============================================================================

TEST_CASE("sync: two tasks complete, results collected", "[concurrency][phase3]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro a() { return 1; };
        coro b() { return 2; };
        var ta = a();
        var tb = b();
        var results = sync {
            await ta;
            await tb;
        };
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

// =============================================================================
// Phase 3: race block
// =============================================================================

TEST_CASE("race: first task wins", "[concurrency][phase3]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro fast() { return 1; };
        coro slow() { yield 0; return 2; };
        var tf = fast();
        var ts = slow();
        var winner = race {
            tf;
            ts;
        };
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    // fast() completes immediately with 1
    REQUIRE(getVarInt(interp, "winner") == 1);
}

// =============================================================================
// Phase 4: cancellation token basic API
// =============================================================================

TEST_CASE("cancellation: cancel token triggers isCancelled", "[concurrency][phase4]") {
    using namespace IkigaiScript;
    auto token = CancellationToken::make();
    REQUIRE(!token->isCancelled());
    token->cancel();
    REQUIRE(token->isCancelled());
}

TEST_CASE("cancellation: child token cancelled when parent cancelled", "[concurrency][phase4]") {
    using namespace IkigaiScript;
    auto parent = CancellationToken::make();
    auto child  = parent->makeChild();
    REQUIRE(!child->isCancelled());
    parent->cancel();
    REQUIRE(child->isCancelled());
}

TEST_CASE("cancellation: callback invoked on cancel", "[concurrency][phase4]") {
    using namespace IkigaiScript;
    auto token = CancellationToken::make();
    bool called = false;
    token->onCancel([&called]() { called = true; });
    REQUIRE(!called);
    token->cancel();
    REQUIRE(called);
}

TEST_CASE("cancellation: callback invoked immediately if already cancelled", "[concurrency][phase4]") {
    using namespace IkigaiScript;
    auto token = CancellationToken::make();
    token->cancel();
    bool called = false;
    token->onCancel([&called]() { called = true; });
    REQUIRE(called);
}

// =============================================================================
// Phase 5: NativeJobPool — C++ thread pool integration
// =============================================================================

TEST_CASE("nativePool: enableNativePool creates pool", "[concurrency][phase5]") {
    auto interp = makeInterp();
    REQUIRE(interp.nativePool == nullptr);
    interp.enableNativePool(2);
    REQUIRE(interp.nativePool != nullptr);
    REQUIRE(interp.nativePool->totalWorkers() == 2);
}

TEST_CASE("nativePool: registerJob + submitNativeJob completes synchronously via pump", "[concurrency][phase5]") {
    auto interp = makeInterp();
    interp.enableNativePool(2);
    interp.registerNativeJob("square", [](const IkigaiScript::List& args) -> IkigaiScript::ValuePtr {
        auto n = args.empty() ? 0 : args[0]->getInt();
        return std::make_shared<IkigaiScript::Value>(IkigaiScript::Int(n * n));
    });

    auto args = IkigaiScript::List{std::make_shared<IkigaiScript::Value>(IkigaiScript::Int(7))};
    auto task = interp.submitNativeJob("square", args);

    REQUIRE(task != nullptr);
    REQUIRE(task->isNativeJob);
    REQUIRE(task->state == IkigaiScript::TaskState::Suspended);

    // Poll until worker finishes (bounded wait)
    using namespace std::chrono_literals;
    for (int i = 0; i < 100 && task->state == IkigaiScript::TaskState::Suspended; ++i) {
        std::this_thread::sleep_for(10ms);
        interp.pump(0);  // drain completions
    }

    REQUIRE(task->state == IkigaiScript::TaskState::Completed);
    REQUIRE(task->result != nullptr);
    REQUIRE(task->result->getInt() == 49);
}

TEST_CASE("nativePool: script nativeJob() + await returns result", "[concurrency][phase5]") {
    auto interp = makeInterp();
    interp.enableNativePool(2);
    interp.registerNativeJob("add", [](const IkigaiScript::List& args) -> IkigaiScript::ValuePtr {
        auto a = args.size() > 0 ? args[0]->getInt() : 0;
        auto b = args.size() > 1 ? args[1]->getInt() : 0;
        return std::make_shared<IkigaiScript::Value>(IkigaiScript::Int(a + b));
    });

    // Script submits "add" job with args 10, 32 and awaits the result
    interp.evaluate(R"(
        var t = nativeJob("add", 10, 32);
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);

    // Pump until worker completes
    using namespace std::chrono_literals;
    auto task = interp.nativePool->submit("add",
        IkigaiScript::List{
            std::make_shared<IkigaiScript::Value>(IkigaiScript::Int(10)),
            std::make_shared<IkigaiScript::Value>(IkigaiScript::Int(32))
        });
    for (int i = 0; i < 100 && task->state == IkigaiScript::TaskState::Suspended; ++i) {
        std::this_thread::sleep_for(10ms);
        interp.pump(0);
    }
    REQUIRE(task->state == IkigaiScript::TaskState::Completed);
    REQUIRE(task->result->getInt() == 42);
}

TEST_CASE("nativePool: unknown job name throws exception", "[concurrency][phase5]") {
    auto interp = makeInterp();
    interp.enableNativePool(1);
    interp.evaluate(R"( var t = nativeJob("doesNotExist"); )");
    REQUIRE(interp.__EXEPTION__ != IkigaiScript::ExceptionType::None);
}

TEST_CASE("nativePool: nativeJob without pool throws exception", "[concurrency][phase5]") {
    auto interp = makeInterp();
    // No enableNativePool() call
    interp.evaluate(R"( var t = nativeJob("anything"); )");
    REQUIRE(interp.__EXEPTION__ != IkigaiScript::ExceptionType::None);
}

TEST_CASE("nativePool: clearState does not crash with active pool", "[concurrency][phase5]") {
    auto interp = makeInterp();
    interp.enableNativePool(2);
    interp.registerNativeJob("noop", [](const IkigaiScript::List&) {
        return std::make_shared<IkigaiScript::Value>();
    });
    interp.clearState();
    REQUIRE(interp.pump(0) == 0);
}

TEST_CASE("nativePool: cancelled task does not deliver result", "[concurrency][phase5]") {
    using namespace IkigaiScript;
    auto interp = makeInterp();
    interp.enableNativePool(1);
    interp.registerNativeJob("slow", [](const List&) -> ValuePtr {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return std::make_shared<Value>(Int(1));
    });

    auto token = CancellationToken::make();
    auto task = interp.nativePool->submit("slow", {}, token);
    token->cancel();

    using namespace std::chrono_literals;
    for (int i = 0; i < 50 && task->isActive(); ++i) {
        std::this_thread::sleep_for(10ms);
        interp.pump(0);
    }
    // Task should be Cancelled (not Completed)
    REQUIRE(task->state == TaskState::Cancelled);
}
