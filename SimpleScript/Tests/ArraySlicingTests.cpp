#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("slice: list exclusive range length", "[slice.list.exclusive]") {
	auto interp = makeInterp();
	interp.evaluate("var lst = [10, 20, 30, 40, 50];");
	interp.evaluate("var s = lst[1..3];");
	// [1,3) = indices 1,2 → 2 elements
	REQUIRE(run(interp, "print(length(s));") == "2");
	REQUIRE(run(interp, "print(s[0]);") == "20");
}

TEST_CASE("slice: list inclusive range length", "[slice.list.inclusive]") {
	auto interp = makeInterp();
	interp.evaluate("var lst = [10, 20, 30, 40, 50];");
	interp.evaluate("var s = lst[1..=3];");
	// [1,3] = indices 1,2,3 → 3 elements
	REQUIRE(run(interp, "print(length(s));") == "3");
}

TEST_CASE("slice: string exclusive range", "[slice.string.exclusive]") {
	auto interp = makeInterp();
	interp.evaluate("var str = \"hello\";");
	interp.evaluate("var s = str[1..3];");
	// code points 1,2 = "el"
	REQUIRE(run(interp, "print(s);") == "el");
}

TEST_CASE("slice: string inclusive range", "[slice.string.inclusive]") {
	auto interp = makeInterp();
	interp.evaluate("var str = \"hello\";");
	interp.evaluate("var s = str[1..=3];");
	// code points 1,2,3 = "ell"
	REQUIRE(run(interp, "print(s);") == "ell");
}

TEST_CASE("slice: array with inclusive range", "[slice.array]") {
	auto interp = makeInterp();
	interp.evaluate("var arr = [1, 2, 3, 4, 5];");
	interp.evaluate("var s = arr[0..=2];");
	REQUIRE(run(interp, "print(length(s));") == "3");
}

TEST_CASE("slice: empty slice 2..2", "[slice.empty]") {
	auto interp = makeInterp();
	interp.evaluate("var lst = [10, 20, 30];");
	interp.evaluate("var s = lst[2..2];");
	REQUIRE(run(interp, "print(length(s));") == "0");
}

TEST_CASE("slice: range(container, a, b) existing builtin", "[slice.range.builtin]") {
	auto interp = makeInterp();
	interp.evaluate("var str = \"hello\";");
	interp.evaluate("var s = range(str, 0, 2);");
	// range(str, start=0, count=2) → first 2 code points "he"
	REQUIRE(run(interp, "print(s);") == "he");
}
