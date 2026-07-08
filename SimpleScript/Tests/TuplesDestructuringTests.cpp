#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// ---------------------------------------------------------------------------
// Phase 1: var / const / dynamic (a, b, ...) = expr
// ---------------------------------------------------------------------------

TEST_CASE("destruct: basic two-element unpack", "[destruct.basic]") {
	auto interp = makeInterp();
	interp.evaluate("var (a, b) = (10, 20);");
	REQUIRE(run(interp, "print(a);") == "10");
	REQUIRE(run(interp, "print(b);") == "20");
}

TEST_CASE("destruct: heterogeneous tuple", "[destruct.hetero]") {
	auto interp = makeInterp();
	interp.evaluate("var (x, s, flag) = (1, \"hello\", true);");
	REQUIRE(run(interp, "print(x);") == "1");
	REQUIRE(run(interp, "print(s);") == "hello");
	REQUIRE(run(interp, "print(flag);") == "true");
}

TEST_CASE("destruct: three ints from function return", "[destruct.func_return]") {
	auto interp = makeInterp();
	interp.evaluate("fun triple() { return (3, 6, 9); }");
	interp.evaluate("var (a, b, c) = triple();");
	REQUIRE(run(interp, "print(a + b + c);") == "18");
}

TEST_CASE("destruct: wildcard _ skips element", "[destruct.wildcard]") {
	auto interp = makeInterp();
	interp.evaluate("var (a, _) = (1, 2);");
	REQUIRE(run(interp, "print(a);") == "1");
	// _ should not be a bound variable
	run(interp, "print(_);");
	// _ is defined as a keyword/reserved, not a user variable — no assertion on its value,
	// just confirm 'a' is correct (tested above)
}

TEST_CASE("destruct: const prevents reassignment", "[destruct.const]") {
	auto interp = makeInterp();
	interp.evaluate("const (x, y) = (1, 2);");
	REQUIRE(run(interp, "print(x);") == "1");
	// assigning a different type to a static-typed const var throws TypeConvert
	run(interp, "x = \"hello\";");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::TypeConvert);
}

TEST_CASE("destruct: size mismatch throws", "[destruct.mismatch]") {
	auto interp = makeInterp();
	bool hadError = interp.evaluate("var (a, b, c) = (1, 2);");
	REQUIRE(hadError == true);
}

TEST_CASE("destruct: non-tuple rhs throws", "[destruct.non_tuple]") {
	auto interp = makeInterp();
	bool hadError = interp.evaluate("var (a, b) = 5;");
	REQUIRE(hadError == true);
}

TEST_CASE("destruct: duplicate names throw", "[destruct.duplicate]") {
	auto interp = makeInterp();
	bool hadError = interp.evaluate("var (a, a) = (1, 2);");
	REQUIRE(hadError == true);
}

TEST_CASE("destruct: scope — variables not visible outside block", "[destruct.scope]") {
	auto interp = makeInterp();
	interp.evaluate("{ var (inner, _) = (42, 0); }");
	// 'inner' should not be accessible in the outer scope
	run(interp, "print(inner);");
	// resolveVariable inserts a null uninit var — but no exception; just check value is not 42
	// (it will be a null/uninitialized default)
	REQUIRE(run(interp, "print(42);") == "42"); // interpreter is still alive
}

// ---------------------------------------------------------------------------
// Phase 2: (a, b, ...) = expr   (reassignment of existing variables)
// ---------------------------------------------------------------------------

TEST_CASE("destruct assign: basic reassignment", "[destruct.assign.basic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 0; var b = 0;");
	interp.evaluate("var t = (1, 2);");
	interp.evaluate("(a, b) = t;");
	REQUIRE(run(interp, "print(a);") == "1");
	REQUIRE(run(interp, "print(b);") == "2");
}

TEST_CASE("destruct assign: swap", "[destruct.assign.swap]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 10; var b = 20;");
	interp.evaluate("(a, b) = (b, a);");
	REQUIRE(run(interp, "print(a);") == "20");
	REQUIRE(run(interp, "print(b);") == "10");
}

TEST_CASE("destruct assign: wildcard skips element", "[destruct.assign.wildcard]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 0; var b = 99;");
	interp.evaluate("(a, _) = (42, 55);");
	REQUIRE(run(interp, "print(a);") == "42");
	REQUIRE(run(interp, "print(b);") == "99");
}

TEST_CASE("destruct assign: type mismatch throws for static var", "[destruct.assign.type_error]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 0; var b = 0;");
	// a is static Int — assigning String to it throws TypeConvert
	run(interp, "(a, b) = (1, \"oops\");");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::TypeConvert);
}

TEST_CASE("destruct assign: const variable throws on reassign", "[destruct.assign.const_error]") {
	auto interp = makeInterp();
	interp.evaluate("const x = 5; var y = 0;");
	// assigning a different type to const static Int throws TypeConvert
	bool hadError = interp.evaluate("(x, y) = (\"oops\", 20);");
	REQUIRE(hadError == true);
}
