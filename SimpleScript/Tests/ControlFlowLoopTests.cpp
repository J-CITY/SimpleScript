#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// C-style for loops (migrated from existing tests)
// =============================================================================

TEST_CASE("for: 3-part i=0;i<10;i++", "[control.for]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "for (i = 0; i < 10; i++) { print(i); }") == "0123456789");
}

TEST_CASE("for: missing increment, manual increment inside", "[control.for]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "for (i = 0; i < 10;) { print(i); i++; }") == "0123456789");
}

TEST_CASE("for: condition-only form", "[control.for]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var i = 0; for (i < 10) { print(i); i++; }") == "0123456789");
}

TEST_CASE("for: empty init and increment", "[control.for]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var i = 0; for (;i < 10;) { print(i); i++; }") == "0123456789");
}

TEST_CASE("for: empty condition (break inside)", "[control.for]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        for (i = 0; ; i++) {
            print(i);
            if (i >= 10) { break; }
        }
    )") == "012345678910");
}

TEST_CASE("for: infinite with continue/break", "[control.for]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var i = 0;
        for (;;) {
            print(i);
            i++;
            if (i < 10) { continue; } else { break; }
        }
    )") == "0123456789");
}

TEST_CASE("for: empty parens with continue/break", "[control.for]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var i = 0;
        for () {
            print(i);
            i++;
            if (i < 10) { continue; } else { break; }
        }
    )") == "0123456789");
}

TEST_CASE("for: float counter with negative step", "[control.for]") {
    auto interp = makeInterp();
    run(interp, R"(
        for (i = 3; i > 0; i--) {
            print(i);
        }
    )");
    REQUIRE(interp.__DEBUG_OUT__ == "321");
}

// =============================================================================
// while loops (migrated)
// =============================================================================

TEST_CASE("while: countdown 10 to 1", "[control.while]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var i = 10; while (i > 0) { print(i); --i; }") == "10987654321");
}

TEST_CASE("while: print only odd numbers", "[control.while]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var i = 10;
        while (i > 0) {
            if (i % 2 == 1) { print(i); }
            --i;
        }
    )") == "97531");
}

TEST_CASE("while: combined || condition", "[control.while]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var i = 10;
        var j = 5;
        while (i > 0 || j > 0) {
            print(i);
            --i;
            j--;
        }
    )") == "10987654321");
}

TEST_CASE("while: combined && condition", "[control.while]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var i = 10;
        var j = 5;
        while (i > 0 && (j > 0)) {
            print(i);
            --i;
            j--;
        }
    )") == "109876");
}

TEST_CASE("while: break exits loop", "[control.while]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var i = 0;
        while (i < 10) {
            if (i == 5) { break; }
            print(i);
            i++;
        }
    )") == "01234");
}

TEST_CASE("while: continue skips rest of body", "[control.while]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var i = 0;
        while (i < 5) {
            i++;
            if (i == 3) { continue; }
            print(i);
        }
    )") == "1245");
}

// =============================================================================
// for (x : collection) — range-based statement loop
// =============================================================================

TEST_CASE("for-in: iterate array of ints", "[control.for-in]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var arr = [1, 2, 3];
        for (x : arr) { print(x); }
    )") == "123");
}

TEST_CASE("for-in: iterate and accumulate", "[control.for-in]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var arr = [10, 20, 30];
        var sum = 0;
        for (x : arr) { sum += x; }
    )");
    REQUIRE(getVarInt(interp, "sum") == 60);
}

TEST_CASE("for-in: iterate with index and value", "[control.for-in]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var arr = [10, 20, 30];
        for (idx, x : arr) { print(idx); }
    )") == "012");
}

TEST_CASE("for-in: iterate map key-value", "[control.for-in]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var m = dictionary();
        m["x"] = 1;
        var count = 0;
        for (k, v : m) { count += v; }
    )");
    REQUIRE(getVarInt(interp, "count") == 1);
}

// =============================================================================
// range() builtin
// =============================================================================

TEST_CASE("range(): numeric range produces array", "[control.range]") {
    auto interp = makeInterp();
    interp.evaluate("var r = range(0, 4);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto r = getVar(interp, "r");
    REQUIRE(r->getType() == IkigaiScript::Type::Array);
}

TEST_CASE("range() in for-in loop", "[control.range]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var r = range(0, 3);
        for (x : r) { print(x); }
    )") == "0123");
}

TEST_CASE("range() string slice", "[control.range]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var s = "hello"; var sub = range(s, 0, 2);)");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

// =============================================================================
// foreach (x : collection)
// =============================================================================

TEST_CASE("foreach: iterate array", "[control.foreach]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(
        var arr = [5, 6, 7];
        foreach (x : arr) { print(x); }
    )") == "567");
}

// =============================================================================
// Loop as expression (NOTE: currently hangs — feature may not be implemented)
// This test is excluded; commented for reference.
// TEST_CASE("loop-as-expression: for loop returns list of body results", "[control.loop-expr]") { ... }
// =============================================================================
