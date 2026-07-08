#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Block expressions (migrated)
// =============================================================================

TEST_CASE("block expr: last expression is the value", "[expressions.block]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var a = {
            var b = 40;
            b + 2;
        };
    )");
	REQUIRE(getVarInt(interp, "a") == 42);
}

TEST_CASE("block expr: nested blocks", "[expressions.block]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var a = {
            var b = { 10 + 20; };
            b + 12;
        };
    )");
	REQUIRE(getVarInt(interp, "a") == 42);
}

TEST_CASE("block expr: return inside block propagates", "[expressions.block]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        fun test() {
            var a = {
                return 42;
                100;
            };
            return a;
        }
        var b = test();
    )");
	REQUIRE(getVarInt(interp, "b") == 42);
}

// =============================================================================
// If as expression (migrated)
// =============================================================================

TEST_CASE("if-expr: selects true branch", "[expressions.if]") {
	auto interp = makeInterp();
	interp.evaluate("var a = if (true) { 42; } else { 10; };");
	REQUIRE(getVarInt(interp, "a") == 42);
}

TEST_CASE("if-expr: selects false branch", "[expressions.if]") {
	auto interp = makeInterp();
	interp.evaluate("var b = if (false) { 42; } else { 10; };");
	REQUIRE(getVarInt(interp, "b") == 10);
}

// =============================================================================
// For-comprehension — produces Array (migrated)
// =============================================================================

TEST_CASE("for-comprehension: single binding maps array", "[expressions.comprehension]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var arr = [1, 2, 3];
        var a = for (x : arr) { x * 2; };
    )");
	REQUIRE(getVar(interp, "a")->getType() == IkigaiScript::Type::Array);
	auto& vec = getVar(interp, "a")->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 3);
	REQUIRE(vec[0] == 2);
	REQUIRE(vec[1] == 4);
	REQUIRE(vec[2] == 6);
}

TEST_CASE("for-comprehension: idx+value binding on array", "[expressions.comprehension]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var arr = [10, 20, 30];
        var a = for (idx, x : arr) { idx + x; };
    )");
	auto& vec = getVar(interp, "a")->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 3);
	REQUIRE(vec[0] == 10);
	REQUIRE(vec[1] == 21);
	REQUIRE(vec[2] == 32);
}

TEST_CASE("for-comprehension: key+value over map", "[expressions.comprehension]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var map = dictionary();
        map["a"] = 100;
        map["b"] = 200;
        var a = for (key, val : map) { val * 2; };
    )");
	auto& vec = getVar(interp, "a")->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 2);
	REQUIRE((vec[0] + vec[1]) == 600);
}

TEST_CASE("for-comprehension: idx+key+value over map", "[expressions.comprehension]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var map = dictionary();
        map["a"] = 100;
        map["b"] = 200;
        var a = for (idx, key, val : map) { idx + val; };
    )");
	auto& vec = getVar(interp, "a")->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 2);
	REQUIRE((vec[0] + vec[1]) == 301);
}

TEST_CASE("for-comprehension: filter-like with conditional", "[expressions.comprehension]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var nums = [1, 2, 3, 4, 5, 6];
        var evens = for (x : nums) { x * 2; };
    )");
	auto& vec = getVar(interp, "evens")->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 6);
	REQUIRE(vec[0] == 2);
	REQUIRE(vec[5] == 12);
}
