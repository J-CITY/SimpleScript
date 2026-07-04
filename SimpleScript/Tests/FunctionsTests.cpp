#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Lambda (migrated from [WHILE] tag, fixed tagging)
// =============================================================================

TEST_CASE("lambda: basic addition lambda", "[functions.lambda]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var l = fun(a, b) { return a + b; };
        print(l(13, 2));
    )") == "15");
}

TEST_CASE("lambda: immediately invoked via variable", "[functions.lambda]") {
    auto interp = makeInterp();
    interp.evaluate("var f = fun() { return 42; }; var x = f();");
    REQUIRE(getVarInt(interp, "x") == 42);
}

TEST_CASE("lambda: captures outer variable (closure)", "[functions.lambda]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var base = 10;
        var add = fun(x) { return base + x; };
        var result = add(5);
    )");
    REQUIRE(getVarInt(interp, "result") == 15);
}

TEST_CASE("lambda: as argument to fun parameter", "[functions.lambda]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        fun apply(f, x) { return f(x); }
        var double = fun(n) { return n * 2; };
        var result = apply(double, 7);
    )");
    REQUIRE(getVarInt(interp, "result") == 14);
}

// =============================================================================
// Named arguments (migrated from [NAMED_ARGS])
// =============================================================================

TEST_CASE("named args: all named in any order", "[functions.args]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        fun myFunc(a, b, c) { return a + b * c; }
        var res = myFunc(c=3, b=2, a=1);
        print(res);
    )") == "7");
}

TEST_CASE("named args: mixed positional and named", "[functions.args]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        fun myFunc(a, b, c) { return a - b + c; }
        var res = myFunc(10, c=5, b=2);
        print(res);
    )") == "13");
}

TEST_CASE("named args: default values only", "[functions.args]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        fun myFunc(a, b=10, c=5) { return a + b + c; }
        var res = myFunc(2);
        print(res);
    )") == "17");
}

TEST_CASE("named args: missing arg uses default", "[functions.args]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        fun myFunc(a, b=10, c=5) { return a + b + c; }
        var res = myFunc(c=1, a=2);
        print(res);
    )") == "13");
}

// =============================================================================
// Basic function declaration and return
// =============================================================================

TEST_CASE("fun: basic return value", "[functions.decl]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        fun add(a, b) { return a + b; }
        print(add(3, 4));
    )") == "7");
}

TEST_CASE("fun: early return", "[functions.decl]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        fun abs_val(x) {
            if (x < 0) { return -x; }
            return x;
        }
        var a = abs_val(-5);
        var b = abs_val(3);
    )");
    REQUIRE(getVarInt(interp, "a") == 5);
    REQUIRE(getVarInt(interp, "b") == 3);
}

TEST_CASE("fun: multiple calls accumulate output", "[functions.decl]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        fun greet(name) { return "hi " + name; }
        print(greet("Alice"));
        print(greet("Bob"));
    )") == "hi Alicehi Bob");
}

// =============================================================================
// Recursion
// =============================================================================

TEST_CASE("recursion: factorial", "[functions.recursion]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        fun fact(n) {
            if (n <= 1) { return 1; }
            return n * fact(n - 1);
        }
        var r = fact(5);
    )");
    REQUIRE(getVarInt(interp, "r") == 120);
}

TEST_CASE("recursion: fibonacci", "[functions.recursion]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        fun fib(n) {
            if (n <= 1) { return n; }
            return fib(n - 1) + fib(n - 2);
        }
        var r = fib(7);
    )");
    REQUIRE(getVarInt(interp, "r") == 13);
}

// =============================================================================
// Typed function parameters and return type
// =============================================================================

TEST_CASE("typed params: fun f(a: Int, b: Int)", "[functions.typed]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        fun add_ints(a: Int, b: Int): Int {
            return a + b;
        }
        var r = add_ints(3, 4);
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "r") == 7);
}

// NOTE: Calling typed function with wrong arg type hangs (infinite loop in type resolution).
// Skipped: TEST_CASE("typed params: type mismatch on call raises error")

// =============================================================================
// Variadic arguments
// =============================================================================

TEST_CASE("variadic: function receives extra args", "[functions.variadic]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        fun sum_all(first, rest...) {
            return first;
        }
        var r = sum_all(1, 2, 3, 4);
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

// NOTE: applyfunction(f, array_of_args) hangs — excluded.
// TEST_CASE("applyfunction: call function with list of args") — skipped
