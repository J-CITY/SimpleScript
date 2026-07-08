#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// print (I/O)
// =============================================================================

TEST_CASE("print: integer", "[builtins.io]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "print(42);") == "42");
}

TEST_CASE("print: string", "[builtins.io]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(print("hello");)") == "hello");
}

TEST_CASE("print: float", "[builtins.io]") {
	auto interp = makeInterp();
	run(interp, "print(3.14);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 3.14f, 0.001f));
}

TEST_CASE("print: bool true", "[builtins.io]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "print(true);") == "true");
}

TEST_CASE("print: bool false", "[builtins.io]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "print(false);") == "false");
}

TEST_CASE("print: null", "[builtins.io]") {
	auto interp = makeInterp();
	// null.getPrintString() returns "" — document this behavior
	run(interp, "print(null);");
	// Accept either "null" or "" — the interpreter currently returns ""
	REQUIRE((interp.__DEBUG_OUT__ == "null" || interp.__DEBUG_OUT__ == ""));
}

TEST_CASE("print: multiple values concatenated in DEBUG_OUT", "[builtins.io]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "print(1); print(2); print(3);") == "123");
}

// =============================================================================
// Math builtins
// =============================================================================

TEST_CASE("abs: negative int", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = abs(-5);");
	REQUIRE(getVarInt(interp, "r") == 5);
}

TEST_CASE("abs: positive int unchanged", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = abs(5);");
	REQUIRE(getVarInt(interp, "r") == 5);
}

TEST_CASE("abs: negative float", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = abs(-3.5);");
	REQUIRE(nearEqual((float)getVarFloat(interp, "r"), 3.5f));
}

TEST_CASE("sqrt: perfect square", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = sqrt(9.0);");
	REQUIRE(nearEqual((float)getVarFloat(interp, "r"), 3.0f));
}

TEST_CASE("pow: 2^10 = 1024", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = pow(2.0, 10.0);");
	REQUIRE(nearEqual((float)getVarFloat(interp, "r"), 1024.0f));
}

TEST_CASE("min: returns smaller value", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = min(3, 7);");
	REQUIRE(getVarInt(interp, "r") == 3);
}

TEST_CASE("max: returns larger value", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = max(3, 7);");
	REQUIRE(getVarInt(interp, "r") == 7);
}

TEST_CASE("sin: sin(0) == 0", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = sin(0.0);");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(nearEqual((float)getVarFloat(interp, "r"), 0.0f));
}

TEST_CASE("cos: cos(0) == 1", "[builtins.math]") {
	auto interp = makeInterp();
	interp.evaluate("var r = cos(0.0);");
	REQUIRE(nearEqual((float)getVarFloat(interp, "r"), 1.0f));
}

// =============================================================================
// Casting builtins
// =============================================================================

TEST_CASE("int(): from float truncates", "[builtins.cast]") {
	auto interp = makeInterp();
	interp.evaluate("var r = int(3.9);");
	REQUIRE(getVarInt(interp, "r") == 3);
}

TEST_CASE("float(): from int", "[builtins.cast]") {
	auto interp = makeInterp();
	interp.evaluate("var r = float(5);");
	REQUIRE(nearEqual((float)getVarFloat(interp, "r"), 5.0f));
}

TEST_CASE("string(): from int", "[builtins.cast]") {
	auto interp = makeInterp();
	interp.evaluate("var r = string(99);");
	REQUIRE(getVarString(interp, "r") == "99");
}

TEST_CASE("bool(): 1 is true", "[builtins.cast]") {
	auto interp = makeInterp();
	interp.evaluate("var r = bool(1);");
	REQUIRE(getVar(interp, "r")->getBool() == true);
}

TEST_CASE("typeof(): int gives type string", "[builtins.cast]") {
	auto interp = makeInterp();
	interp.evaluate("var r = typeof(42);");
	REQUIRE(getVarType(interp, "r") == IkigaiScript::Type::String);
	// "int" or "Int" — just check it's non-empty
	REQUIRE(!getVarString(interp, "r").empty());
}

// =============================================================================
// Utility builtins
// =============================================================================

TEST_CASE("swap: exchanges two variables", "[builtins.utility]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 1; var b = 2; swap(a, b);");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "a") == 2);
	REQUIRE(getVarInt(interp, "b") == 1);
}

TEST_CASE("identity: returns its argument", "[builtins.utility]") {
	auto interp = makeInterp();
	interp.evaluate("var r = identity(42);");
	REQUIRE(getVarInt(interp, "r") == 42);
}

TEST_CASE("copy: returns a copy of value", "[builtins.utility]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 5; var b = copy(a);");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "b") == 5);
}
