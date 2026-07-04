#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Chained assignment
// =============================================================================

TEST_CASE("chained assign: x = y = 5", "[operators.assignment]") {
    auto interp = makeInterp();
    run(interp, "var x = 0; var y = 0; x = y = 5; print(x); print(y);");
    REQUIRE(interp.__DEBUG_OUT__ == "55");
}

TEST_CASE("chained assign: a = b = c = 42", "[operators.assignment]") {
    auto interp = makeInterp();
    run(interp, "var a = 0; var b = 0; var c = 0; a = b = c = 42; print(a); print(b); print(c);");
    REQUIRE(interp.__DEBUG_OUT__ == "424242");
}

// =============================================================================
// Compound assignment operators
// =============================================================================

TEST_CASE("+=  int", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate("var a = 5; a += 3;");
    REQUIRE(getVarInt(interp, "a") == 8);
}

TEST_CASE("-=  int", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate("var a = 5; a -= 3;");
    REQUIRE(getVarInt(interp, "a") == 2);
}

TEST_CASE("*=  int", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate("var a = 4; a *= 3;");
    REQUIRE(getVarInt(interp, "a") == 12);
}

TEST_CASE("/=  int", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate("var a = 12; a /= 4;");
    REQUIRE(getVarInt(interp, "a") == 3);
}

TEST_CASE("+= float", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate("var a = 1.5; a += 1.5;");
    REQUIRE(nearEqual((float)getVarFloat(interp, "a"), 3.0f));
}

TEST_CASE("-= float", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate("var a = 5.0; a -= 2.5;");
    REQUIRE(nearEqual((float)getVarFloat(interp, "a"), 2.5f));
}

TEST_CASE("+= string concat", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var s = "hello"; s += " world";)");
    REQUIRE(getVarString(interp, "s") == "hello world");
}

// =============================================================================
// Assignment to typed variables
// =============================================================================

TEST_CASE("assign compatible: Int <- int literal", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate("var a: Int = 10;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "a") == 10);
}

TEST_CASE("assign compatible: Float <- int (promotion)", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.evaluate("var a: Float = 5;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

TEST_CASE("assign incompatible: Int <- float literal raises TypeConvert", "[operators.assignment]") {
    auto interp = makeInterp();
    interp.__EXEPTION__ = IkigaiScript::ExceptionType::None;
    interp.evaluate("var a: Int = 1.0;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::TypeConvert);
}

// =============================================================================
// Dynamic reassignment
// NOTE: Cross-type reassignment (e.g. int→string) fails silently in the
// current interpreter — the variable becomes null. Only same-type reassignment
// is reliably supported. Tests verify observable print output.
// =============================================================================

TEST_CASE("dynamic var: same-type reassignment works", "[operators.assignment]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = 10; a = 20; print(a);") == "20");
}

TEST_CASE("dynamic var: reassign to another int value", "[operators.assignment]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var a = 100; a = 42; print(a);") == "42");
}
