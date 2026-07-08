#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// NOTE: Logical operators && and || return Int (0/1), not Bool.
// They ARE short-circuit: && stops on false, || stops on true.
// The ! operator is logical NOT: !x → 0 if truthy, 1 if falsy.
// For factorial use fact(n).

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
// Short-circuit evaluation (&&/|| are now short-circuit)
// =============================================================================

TEST_CASE("&& short-circuits: false && side-effect — rhs not evaluated", "[operators.logical]") {
	auto interp = makeInterp();
	// false && anything: rhs must not be evaluated
	run(interp, "var x = false && (print(\"side\") == \"\");");
	REQUIRE(interp.__DEBUG_OUT__ == "");
}

TEST_CASE("|| short-circuits: true || side-effect — rhs not evaluated", "[operators.logical]") {
	auto interp = makeInterp();
	run(interp, "var x = true || (print(\"side\") == \"\");");
	REQUIRE(interp.__DEBUG_OUT__ == "");
}

TEST_CASE("&& evaluates rhs when lhs is true", "[operators.logical]") {
	auto interp = makeInterp();
	run(interp, "var x = true && (print(\"side\") == \"\");");
	REQUIRE(interp.__DEBUG_OUT__ == "side");
}

TEST_CASE("|| evaluates rhs when lhs is false", "[operators.logical]") {
	auto interp = makeInterp();
	run(interp, "var x = false || (print(\"side\") == \"\");");
	REQUIRE(interp.__DEBUG_OUT__ == "side");
}

// =============================================================================
// Logical NOT (!)
// =============================================================================

TEST_CASE("!false gives 1", "[operators.logical]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var r = !false; print(r);") == "1");
}

TEST_CASE("!true gives 0", "[operators.logical]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var r = !true; print(r);") == "0");
}

TEST_CASE("!0 gives 1", "[operators.logical]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var r = !0; print(r);") == "1");
}

TEST_CASE("!1 gives 0", "[operators.logical]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var r = !1; print(r);") == "0");
}

// =============================================================================
// Negation patterns in conditions
// =============================================================================

TEST_CASE("negation: ! in if condition", "[operators.logical]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "if (!false) { print(1); } else { print(0); }") == "1");
}

TEST_CASE("negation pattern: compare int to 0", "[operators.logical]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var f = 0; if (f == 0) { print(1); } else { print(0); }") == "1");
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
