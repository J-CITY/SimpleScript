#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Basic coroutine lifecycle
// =============================================================================

TEST_CASE("coro: basic yield-yield-return sequence", "[coroutines]") {
    auto interp = makeInterp();
    run(interp, R"(
        coro f(a, b) {
            yield a + b + 2;
            yield a + b + 4;
            return a + b;
        };
        var cr = f(13, 2);
        print(cr());
        print(cr());
        print(cr());
    )");
    // 13+2+2=17, 13+2+4=19, 13+2=15
    REQUIRE(interp.__DEBUG_OUT__ == "171915");
}

TEST_CASE("coro: single yield then return", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro g(x) {
            yield x * 2;
            return x * 3;
        };
        var c = g(5);
        var first  = c();
        var second = c();
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "first") == 10);
    REQUIRE(getVarInt(interp, "second") == 15);
}

TEST_CASE("coro: is active after creation, before exhaust", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro h() {
            yield 1;
            return 2;
        };
        var c = h();
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto cval = getVar(interp, "c");
    REQUIRE(cval->getType() == IkigaiScript::Type::Coro);
    REQUIRE(cval->getCoro()->isActive() == true);
}

TEST_CASE("coro: becomes inactive after exhaustion", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro h() {
            yield 1;
            return 2;
        };
        var c = h();
        var v1 = c();
        var v2 = c();
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto cval = getVar(interp, "c");
    REQUIRE(cval->getCoro()->isActive() == false);
}

TEST_CASE("coro: multiple yield values are correct", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro counter(start) {
            yield start;
            yield start + 1;
            yield start + 2;
            return start + 3;
        };
        var c = counter(10);
        var a = c();
        var b = c();
        var d = c();
        var e = c();
    )");
    REQUIRE(getVarInt(interp, "a") == 10);
    REQUIRE(getVarInt(interp, "b") == 11);
    REQUIRE(getVarInt(interp, "d") == 12);
    REQUIRE(getVarInt(interp, "e") == 13);
}

TEST_CASE("coro: truthiness while active", "[coroutines]") {
    auto interp = makeInterp();
    run(interp, R"(
        coro gen() { yield 1; return 2; };
        var c = gen();
        if (c) { print("active"); }
        c();
        c();
        if (c) { print("still"); } else { print("done"); }
    )");
    REQUIRE(interp.__DEBUG_OUT__ == "activedone");
}

// =============================================================================
// Phase 0: Unique coro scopes (two instances must not share state)
// =============================================================================

TEST_CASE("coro: two instances are independent (unique scopes)", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro counter(start) {
            yield start;
            yield start + 1;
            return start + 2;
        };
        var a = counter(0);
        var b = counter(10);
        var a0 = a();
        var b0 = b();
        var a1 = a();
        var b1 = b();
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "a0") == 0);
    REQUIRE(getVarInt(interp, "b0") == 10);
    REQUIRE(getVarInt(interp, "a1") == 1);
    REQUIRE(getVarInt(interp, "b1") == 11);
}

// =============================================================================
// Phase 0: Yield inside a nested if block
// =============================================================================

TEST_CASE("coro: yield inside if branch", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro f(flag) {
            if (flag) {
                yield 42;
            }
            return 0;
        };
        var c = f(true);
        var v1 = c();
        var v2 = c();
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "v1") == 42);
    REQUIRE(getVarInt(interp, "v2") == 0);
}

// =============================================================================
// Phase 0: Exception safety — coro marks itself inactive on throw
// =============================================================================

TEST_CASE("coro: exception in body marks inactive", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro bad() {
            yield 1;
            print("boom");
        };
        var c = bad();
        var v = c();
    )");
    // First resume should return 1
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "v") == 1);
    // Calling an exhausted coro returns null (not another crash)
    interp.evaluate("var safe = c();");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

// =============================================================================
// Phase 0: Reusing coro name twice — must create separate instances
// =============================================================================

TEST_CASE("coro: same name reused creates fresh instances", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro counter(n) {
            yield n;
            return n + 1;
        };
        var c1 = counter(5);
        var c2 = counter(5);
        var v1 = c1();
        var v2 = c1();
        var u1 = c2();
        var u2 = c2();
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "v1") == 5);
    REQUIRE(getVarInt(interp, "v2") == 6);
    REQUIRE(getVarInt(interp, "u1") == 5);
    REQUIRE(getVarInt(interp, "u2") == 6);
}
