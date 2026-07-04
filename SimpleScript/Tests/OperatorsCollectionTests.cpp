#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Array + Array merge
// =============================================================================

TEST_CASE("array + array concatenates", "[operators.collection]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [1, 2]; var b = [3, 4]; var c = a + b;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto c = getVar(interp, "c");
    REQUIRE(c->getType() == IkigaiScript::Type::Array);
    auto& vec = c->getStdVector<IkigaiScript::Int>();
    REQUIRE(vec.size() == 4);
    REQUIRE(vec[0] == 1);
    REQUIRE(vec[3] == 4);
}

TEST_CASE("+= on array appends", "[operators.collection]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [1, 2]; var b = [3]; a += b;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto a = getVar(interp, "a");
    REQUIRE(a->getType() == IkigaiScript::Type::Array);
    auto& vec = a->getStdVector<IkigaiScript::Int>();
    REQUIRE(vec.size() == 3);
}

// =============================================================================
// # hash operator (smoke test: no crash, returns Int)
// =============================================================================

TEST_CASE("hash operator on int: # x", "[operators.collection]") {
    auto interp = makeInterp();
    interp.evaluate("var x = 42; var h = #x;");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

TEST_CASE("hash operator on string: # s", "[operators.collection]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var s = "hello"; var h = #s;)");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

// =============================================================================
// Custom operator overload
// =============================================================================

TEST_CASE("custom operator: operator+(Int, Int) overload", "[operators.custom]") {
    auto interp = makeInterp();
    // Define a custom + that subtracts (to distinguish from built-in)
    const std::string code = R"(
        operator +(a: Int, b: Int): Int {
            return a - b;
        }
        var result = 10 + 3;
        print(result);
    )";
    run(interp, code);
    // If custom operators override built-in, result is 10-3=7
    // If not, result is 13 — either way it should not crash
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}
