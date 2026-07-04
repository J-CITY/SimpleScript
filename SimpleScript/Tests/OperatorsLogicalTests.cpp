#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// NOTE: Logical operators && and || return Int (0/1), not Bool.
// They are NOT short-circuit: all arguments are evaluated eagerly.
// The ! operator (1-arg) computes factorial; it is NOT logical NOT.
// Logical NOT is best tested via negated comparisons or doubled conditions.

// =============================================================================
// Boolean AND (&&) — returns Int 0/1
// =============================================================================

TEST_CASE("&& both true: result is 1", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = (true && true); print(a);") == "1");
}

TEST_CASE("&& true and false: result is 0", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = (true && false); print(a);") == "0");
}

TEST_CASE("&& both false: result is 0", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = (false && false); print(a);") == "0");
}

TEST_CASE("&& int truthy: 1 && 1 is 1", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = 1 && 1; print(a);") == "1");
}

TEST_CASE("&& int falsy: 0 && 1 is 0", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = 0 && 1; print(a);") == "0");
}

// =============================================================================
// Boolean OR (||) — returns Int 0/1
// =============================================================================

TEST_CASE("|| both false: result is 0", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = (false || false); print(a);") == "0");
}

TEST_CASE("|| true and false: result is 1", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = (true || false); print(a);") == "1");
}

TEST_CASE("|| both true: result is 1", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = (true || true); print(a);") == "1");
}

TEST_CASE("|| int: 0||1 is 1", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = 0 || 1; print(a);") == "1");
}

TEST_CASE("|| int: 0||0 is 0", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = 0 || 0; print(a);") == "0");
}

// =============================================================================
// Eager evaluation (no short-circuit in stdlib dispatch)
// =============================================================================

TEST_CASE("&& evaluates both sides eagerly: side effects happen", "[operators.logical]") {
    auto interp = makeInterp();
    // Both sides are evaluated regardless of left result
    run(interp, "var x = false && (print(\"side\") == \"\");");
    REQUIRE(interp.__DEBUG_OUT__ == "side");
}

TEST_CASE("|| evaluates both sides eagerly: side effects happen", "[operators.logical]") {
    auto interp = makeInterp();
    run(interp, "var x = true || (print(\"side\") == \"\");");
    REQUIRE(interp.__DEBUG_OUT__ == "side");
}

// =============================================================================
// Logical negation via (x == 0) since ! is factorial
// =============================================================================

TEST_CASE("negation pattern: compare int to 0 (bool == int may not upconvert correctly)", "[operators.logical]") {
    auto interp = makeInterp();
    // var f = 0 (int falsy), check f == 0
    REQUIRE(run(interp, "var f = 0; if (f == 0) { print(1); } else { print(0); }") == "1");
}

TEST_CASE("negation pattern: truthy int != 0", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var t = 1; if (t == 0) { print(1); } else { print(0); }") == "0");
}

// =============================================================================
// Truthiness of various types
// =============================================================================

TEST_CASE("truthy: non-zero int in if condition", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "if (42) { print(1); } else { print(0); }") == "1");
}

TEST_CASE("falsy: zero int in if condition", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "if (0) { print(1); } else { print(0); }") == "0");
}

TEST_CASE("truthy: non-empty string in if condition", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(if ("hello") { print(1); } else { print(0); })") == "1");
}

TEST_CASE("falsy: empty string in if condition", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, R"(if ("") { print(1); } else { print(0); })") == "0");
}

TEST_CASE("truthy: non-empty array in if condition", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = [1, 2]; if (a) { print(1); } else { print(0); }") == "1");
}

TEST_CASE("falsy: null in if condition", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = null; if (a) { print(1); } else { print(0); }") == "0");
}

// =============================================================================
// Complex logical in conditions (the typical use-case)
// =============================================================================

TEST_CASE("combined && and || in condition", "[operators.logical]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "if (1 && 0 || 1) { print(1); } else { print(0); }") == "1");
}
