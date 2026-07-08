#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// operator +
// =============================================================================

TEST_CASE("operator+ int: 2+2", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2 + 2; print(a);") == "4");
}

TEST_CASE("operator+ int: -2+2", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = -2 + 2; print(a);") == "0");
}

TEST_CASE("operator+ int: 2+-2", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2 + -2; print(a);") == "0");
}

TEST_CASE("operator+ int: 2+(-2) parenthesised", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2 + (-2); print(a);") == "0");
}

TEST_CASE("operator+ int: (2+(-2)) fully parenthesised", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = (2 + (-2)); print(a);") == "0");
}

TEST_CASE("operator+ int: (2+-2)", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = (2 + -2); print(a);") == "0");
}

TEST_CASE("operator+ float: 2.0+-2.0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 2.0 + -2.0; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 0.0f));
}

TEST_CASE("operator+ string concat", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(var a = "a" + "b"; print(a);)") == "ab");
}

TEST_CASE("operator+ bool: true+false = true (OR)", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = true + false; print(a);") == "true");
}

TEST_CASE("operator+ bool: false+false = false (OR)", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = false + false; print(a);") == "false");
}

TEST_CASE("operator+ char+char", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 'x'; var b = 'y'; var c = a + b;");
	REQUIRE(getVarType(interp, "c") == IkigaiScript::Type::Char);
	REQUIRE(getVar(interp, "c")->getChar() == (char32_t)('x' + 'y'));
}

TEST_CASE("operator+ int+float promotion", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 1 + 2.5; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 3.5f));
}

// =============================================================================
// operator -
// =============================================================================

TEST_CASE("operator- int: 2-2", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2 - 2; print(a);") == "0");
}

TEST_CASE("operator- float: 2.0-2.0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 2.0 - 2.0; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 0.0f));
}

TEST_CASE("operator- int-float promotion: 2-+2.0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 2 - +2.0; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 0.0f));
}

TEST_CASE("operator- unary negation int", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = -5; print(a);") == "-5");
}

TEST_CASE("operator- unary negation float", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = -3.0; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), -3.0f));
}

// =============================================================================
// operator *
// =============================================================================

TEST_CASE("operator* int*float: 2*2.0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 2 * 2.0; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 4.0f));
}

TEST_CASE("operator* int*(-float): 2*-2.0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 2 * -2.0; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), -4.0f));
}

TEST_CASE("operator* int*int", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 3 * 4; print(a);") == "12");
}

// =============================================================================
// operator /
// =============================================================================

TEST_CASE("operator/ int/(-float): 2/-2.0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 2 / -2.0; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), -1.0f));
}

TEST_CASE("operator/ integer division truncates: 1/2 == 0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 1/2; print(a);") == "0");
}

TEST_CASE("operator/ float division: 1.0/2.0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 1.0 / 2.0; print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 0.5f));
}

// =============================================================================
// operator %
// =============================================================================

TEST_CASE("operator% int modulo: 3%2 == 1", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 3 % 2; print(a);") == "1");
}

TEST_CASE("operator% float modulo via fmod", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var a = 5.5 % 2.0; print(a);");
	// fmod(5.5, 2.0) = 1.5
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 1.5f));
}

// =============================================================================
// Operator precedence & complex expressions
// =============================================================================

TEST_CASE("expr precedence: 2*2+2 == 6", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2 * 2 + 2; print(a);") == "6");
}

TEST_CASE("expr precedence: 2*(2+2) == 8", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 2 * (2 + 2); print(a);") == "8");
}

TEST_CASE("expr with variables", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var b = 2; var c = 3 + b; var a = 2 * b + c; print(a);") == "9");
}

TEST_CASE("expr nested complex with floats", "[operators.arithmetic]") {
	auto interp = makeInterp();
	run(interp, "var b = 2; var c = 3 + b;"
		"var a = (((2 * (b + c) * 1.0) + 1) - 1.0) / (1.0/2); print(a);");
	REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 28.0f));
}

// =============================================================================
// Increment / decrement / compound assignment
// =============================================================================

TEST_CASE("inc/dec compound: a++ ++a a-- --a a+=1 a-=1", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(
        var a = 1;
        a++;
        ++a;
        a--;
        --a;
        a+=1;
        a-=1;
        print(a + 2 - 3);
    )") == "0");
}

TEST_CASE("prefix increment returns incremented value", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 5; ++a;");
	REQUIRE(getVarInt(interp, "a") == 6);
}

TEST_CASE("prefix decrement returns decremented value", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 5; --a;");
	REQUIRE(getVarInt(interp, "a") == 4);
}

TEST_CASE("prefix ++ as expression yields new value", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 5; var b = ++a;");
	REQUIRE(getVarInt(interp, "a") == 6);
	REQUIRE(getVarInt(interp, "b") == 6);
}

TEST_CASE("postfix ++ as expression yields old value", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 5; var b = a++;");
	REQUIRE(getVarInt(interp, "a") == 6);
	REQUIRE(getVarInt(interp, "b") == 5);
}

TEST_CASE("prefix -- as expression yields new value", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 5; var b = --a;");
	REQUIRE(getVarInt(interp, "a") == 4);
	REQUIRE(getVarInt(interp, "b") == 4);
}

TEST_CASE("postfix -- as expression yields old value", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 5; var b = a--;");
	REQUIRE(getVarInt(interp, "a") == 4);
	REQUIRE(getVarInt(interp, "b") == 5);
}

TEST_CASE("compound assign *=", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 3; a *= 4;");
	REQUIRE(getVarInt(interp, "a") == 12);
}

TEST_CASE("compound assign /= int", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 10; a /= 2;");
	REQUIRE(getVarInt(interp, "a") == 5);
}

// =============================================================================
// Hex and binary literals
// =============================================================================

TEST_CASE("hex literal 0xFF == 255", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 0xFF;");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "a") == 255);
}

TEST_CASE("binary literal 0b1010 == 10", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 0b1010;");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "a") == 10);
}

TEST_CASE("hex literal arithmetic: 0x10 + 1 == 17", "[operators.arithmetic]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, "var a = 0x10 + 1; print(a);") == "17");
}

TEST_CASE("binary literal 0b0000 == 0", "[operators.arithmetic]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 0b0000;");
	REQUIRE(getVarInt(interp, "a") == 0);
}
