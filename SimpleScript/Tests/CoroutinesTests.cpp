#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Basic coroutine lifecycle
// =============================================================================

TEST_CASE("coro: basic yeld-yeld-return sequence", "[coroutines]") {
    auto interp = makeInterp();
    run(interp, R"(
        coro f(a, b) {
            yeld a + b + 2;
            yeld a + b + 4;
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

TEST_CASE("coro: single yeld then return", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro g(x) {
            yeld x * 2;
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
            yeld 1;
            return 2;
        };
        var c = h();
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto cval = getVar(interp, "c");
    REQUIRE(cval->getType() == IkigaiScript::Type::Coro);
    REQUIRE(cval->getCoro()->isActive == true);
}

TEST_CASE("coro: becomes inactive after exhaustion", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro h() {
            yeld 1;
            return 2;
        };
        var c = h();
        var v1 = c();
        var v2 = c();
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto cval = getVar(interp, "c");
    REQUIRE(cval->getCoro()->isActive == false);
}

TEST_CASE("coro: multiple yeld values are correct", "[coroutines]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        coro counter(start) {
            yeld start;
            yeld start + 1;
            yeld start + 2;
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
        coro gen() { yeld 1; return 2; };
        var c = gen();
        if (c) { print("active"); }
        c();
        c();
        if (c) { print("still"); } else { print("done"); }
    )");
    REQUIRE(interp.__DEBUG_OUT__ == "activedone");
}
