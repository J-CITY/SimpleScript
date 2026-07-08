#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Basic if / else
// =============================================================================

TEST_CASE("if true branch executes", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "if (1 < 2) { print(1); }") == "1");
}

TEST_CASE("if false, else branch executes", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "if (1 > 2) { print(1); } else { print(2); }") == "2");
}

TEST_CASE("if with variable comparison: a < a*a", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2; if (a < a * a) { print(1); } else { print(2); }") == "1");
}

TEST_CASE("if with &&: a>1 && a%2==0", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2; if (a > 1 && a % 2 == 0) { print(1); } else { print(2); }") == "1");
}

TEST_CASE("if with && false result", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2; if (a > 1 && a % 2 == 1) { print(1); } else { print(2); }") == "2");
}

TEST_CASE("if with ||: a>1 || a%2==0", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2; if (a > 1 || a % 2 == 0) { print(1); } else { print(2); }") == "1");
}

TEST_CASE("if with || partial true: a>2 || a%2==0", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2; if (a > 2 || a % 2 == 0) { print(1); } else { print(2); }") == "1");
}

TEST_CASE("two consecutive ifs", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        if (1 < 2) { print(1); }
        if (1 > 2) { print(2); } else { print(3); }
    )") == "13");
}

// =============================================================================
// else-if chains
// =============================================================================

TEST_CASE("else-if chain: first branch taken", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        var x = 1;
        if (x == 1) { print("a"); }
        else if (x == 2) { print("b"); }
        else { print("c"); }
    )") == "a");
}

TEST_CASE("else-if chain: middle branch taken", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        var x = 2;
        if (x == 1) { print("a"); }
        else if (x == 2) { print("b"); }
        else { print("c"); }
    )") == "b");
}

TEST_CASE("else-if chain: else branch taken", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        var x = 3;
        if (x == 1) { print("a"); }
        else if (x == 2) { print("b"); }
        else { print("c"); }
    )") == "c");
}

TEST_CASE("else-if multiple levels", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        var score = 75;
        if (score >= 90) { print("A"); }
        else if (score >= 80) { print("B"); }
        else if (score >= 70) { print("C"); }
        else { print("F"); }
    )") == "C");
}

// =============================================================================
// If with truthy/falsy non-bool values
// =============================================================================

TEST_CASE("if truthy: non-zero int condition", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "if (5) { print(1); } else { print(0); }") == "1");
}

TEST_CASE("if falsy: zero int condition", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "if (0) { print(1); } else { print(0); }") == "0");
}

TEST_CASE("if truthy: non-empty string condition", "[control.if]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(if ("hello") { print(1); } else { print(0); })") == "1");
}

// =============================================================================
// If as expression
// =============================================================================

TEST_CASE("if-as-expression: true branch value", "[control.if]") {
	auto interp = makeInterp();
	interp.evaluate("var a = if (true) { 42; } else { 10; };");
	REQUIRE(getVarInt(interp, "a") == 42);
}

TEST_CASE("if-as-expression: false branch value", "[control.if]") {
	auto interp = makeInterp();
	interp.evaluate("var b = if (false) { 42; } else { 10; };");
	REQUIRE(getVarInt(interp, "b") == 10);
}

TEST_CASE("if-as-expression used in arithmetic", "[control.if]") {
	auto interp = makeInterp();
	interp.evaluate("var x = 5; var result = if (x > 3) { 100; } else { 0; };");
	REQUIRE(getVarInt(interp, "result") == 100);
}
