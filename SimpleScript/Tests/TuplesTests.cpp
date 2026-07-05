#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("tuple: create heterogeneous tuple", "[tuple.create]") {
    auto interp = makeInterp();
    interp.evaluate("var t = (1, \"hello\", true);");
    REQUIRE(run(interp, "print(t);") == "(1, hello, true)");
}

TEST_CASE("tuple: access first element", "[tuple.access.first]") {
    auto interp = makeInterp();
    interp.evaluate("var t = (42, \"world\");");
    REQUIRE(run(interp, "print(t.0);") == "42");
}

TEST_CASE("tuple: access second element", "[tuple.access.second]") {
    auto interp = makeInterp();
    interp.evaluate("var t = (42, \"world\");");
    REQUIRE(run(interp, "print(t.1);") == "world");
}

TEST_CASE("tuple: arithmetic on elements", "[tuple.arith]") {
    auto interp = makeInterp();
    interp.evaluate("var t = (10, 20);");
    REQUIRE(run(interp, "print(t.0 + t.1);") == "30");
}

TEST_CASE("tuple: iterate in for loop", "[tuple.foreach]") {
    auto interp = makeInterp();
    interp.evaluate("var t = (10, 20, 30);");
    interp.evaluate("var sum = 0;");
    interp.evaluate("for (x : t) { sum = sum + x; }");
    REQUIRE(run(interp, "print(sum);") == "60");
}

TEST_CASE("tuple: out-of-range access throws", "[tuple.bounds]") {
    auto interp = makeInterp();
    interp.evaluate("var t = (1, 2);");
    run(interp, "print(t.5);");
    REQUIRE(interp.__EXEPTION__ != ExceptionType::None);
}

TEST_CASE("tuple: three elements different types", "[tuple.three]") {
    auto interp = makeInterp();
    interp.evaluate("var t = (1, 2.5, \"hi\");");
    REQUIRE(run(interp, "print(t.2);") == "hi");
}
