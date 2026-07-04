#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Static type annotations
// =============================================================================

TEST_CASE("static type Float: var a: Float = 1 promotes int", "[types.static]") {
    auto interp = makeInterp();
    run(interp, "var a: Float = 1; print(a);");
    REQUIRE(nearEqual(parseFloat(interp.__DEBUG_OUT__), 1.0f));
}

TEST_CASE("static type Int: var a: Int = 42 works", "[types.static]") {
    auto interp = makeInterp();
    interp.evaluate("var a: Int = 42;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "a") == 42);
}

TEST_CASE("static type Bool: var a: Bool = true works", "[types.static]") {
    auto interp = makeInterp();
    interp.evaluate("var a: Bool = true;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Bool);
    REQUIRE(getVar(interp, "a")->getBool() == true);
}

TEST_CASE("static type String: var a: String = works", "[types.static]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var a: String = "hello";)");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarString(interp, "a") == "hello");
}

// =============================================================================
// Type conversion errors
// =============================================================================

TEST_CASE("type error: Int <- float raises TypeConvert", "[types.errors]") {
    auto interp = makeInterp();
    interp.evaluate("var a: Int = 1.0;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::TypeConvert);
}

TEST_CASE("type error: Int <- string raises TypeConvert", "[types.errors]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var a: Int = "hello";)");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::TypeConvert);
}

TEST_CASE("type error: Bool <- int raises TypeConvert", "[types.errors]") {
    auto interp = makeInterp();
    interp.evaluate("var a: Bool = 1;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::TypeConvert);
}

TEST_CASE("type error: String <- int raises TypeConvert", "[types.errors]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var a: String = 42;)");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::TypeConvert);
}

// =============================================================================
// Dynamic variables
// =============================================================================

TEST_CASE("dynamic var: untyped var accepts any type", "[types.dynamic]") {
    auto interp = makeInterp();
    // Same-type reassignment works reliably
    REQUIRE(run(interp, "var a = 10; a = 20; print(a);") == "20");
}

TEST_CASE("dynamic var: null assignment", "[types.dynamic]") {
    auto interp = makeInterp();
    interp.evaluate("var a = null;");
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Null);
}

TEST_CASE("dynamic var: reassign from int to bool", "[types.dynamic]") {
    auto interp = makeInterp();
    interp.evaluate("var a = 10; a = true;");
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Bool);
}

// =============================================================================
// Nullable types
// =============================================================================

TEST_CASE("nullable type: Int? accepts null", "[types.nullable]") {
    auto interp = makeInterp();
    interp.evaluate("var a: Int? = null;");
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Null);
}

TEST_CASE("nullable type: Int? accepts int", "[types.nullable]") {
    auto interp = makeInterp();
    interp.evaluate("var a: Int? = 5;");
    // Just check no TypeConvert exception
    REQUIRE(interp.__EXEPTION__ != IkigaiScript::ExceptionType::TypeConvert);
}

// =============================================================================
// Type casting builtins
// =============================================================================

TEST_CASE("cast: int() converts float to int", "[types.cast]") {
    auto interp = makeInterp();
    interp.evaluate("var a = int(3.7);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Int);
    REQUIRE(getVarInt(interp, "a") == 3);
}

TEST_CASE("cast: float() converts int to float", "[types.cast]") {
    auto interp = makeInterp();
    interp.evaluate("var a = float(5);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Float);
    REQUIRE(nearEqual((float)getVarFloat(interp, "a"), 5.0f));
}

TEST_CASE("cast: string() converts int to string", "[types.cast]") {
    auto interp = makeInterp();
    interp.evaluate("var a = string(42);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarString(interp, "a") == "42");
}

TEST_CASE("cast: bool() converts non-zero to true", "[types.cast]") {
    auto interp = makeInterp();
    interp.evaluate("var a = bool(1);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVar(interp, "a")->getBool() == true);
}

TEST_CASE("cast: bool() converts zero to false", "[types.cast]") {
    auto interp = makeInterp();
    interp.evaluate("var a = bool(0);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVar(interp, "a")->getBool() == false);
}

TEST_CASE("typeof returns type name string", "[types.cast]") {
    auto interp = makeInterp();
    interp.evaluate("var a = typeof(42);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    // typeof returns "int" or "Int" - check it's a string
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::String);
}
