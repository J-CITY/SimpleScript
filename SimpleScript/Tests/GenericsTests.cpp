#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("generics: identity monomorphizes for Int", "[generics.identity.int]") {
    auto interp = makeInterp();
    interp.evaluate("fun identity<T>(x: T): T { return x; }");
    interp.evaluate("var r = identity(42);");
    REQUIRE(run(interp, "print(r);") == "42");
}

TEST_CASE("generics: identity monomorphizes for String", "[generics.identity.string]") {
    auto interp = makeInterp();
    interp.evaluate("fun identity<T>(x: T): T { return x; }");
    interp.evaluate("var r = identity(\"hello\");");
    REQUIRE(run(interp, "print(r);") == "hello");
}

TEST_CASE("generics: identity monomorphizes for Float", "[generics.identity.float]") {
    auto interp = makeInterp();
    interp.evaluate("fun identity<T>(x: T): T { return x; }");
    interp.evaluate("var r = identity(3.14);");
    auto out = run(interp, "print(r);");
    REQUIRE(out.find("3.14") != std::string::npos);
}

TEST_CASE("generics: same function called twice with different types", "[generics.multi.type]") {
    auto interp = makeInterp();
    interp.evaluate("fun wrap<T>(x: T): T { return x; }");
    interp.evaluate("var a = wrap(1);");
    interp.evaluate("var b = wrap(\"world\");");
    REQUIRE(run(interp, "print(a);") == "1");
    REQUIRE(run(interp, "print(b);") == "world");
}

TEST_CASE("generics: monomorphized function cached second call", "[generics.cache]") {
    auto interp = makeInterp();
    interp.evaluate("fun double_it<T>(x: T): T { return x + x; }");
    interp.evaluate("var r1 = double_it(5);");
    interp.evaluate("var r2 = double_it(10);");
    REQUIRE(run(interp, "print(r1);") == "10");
    REQUIRE(run(interp, "print(r2);") == "20");
}

TEST_CASE("generics: bool argument", "[generics.bool]") {
    auto interp = makeInterp();
    interp.evaluate("fun passthrough<T>(x: T): T { return x; }");
    interp.evaluate("var r = passthrough(true);");
    REQUIRE(run(interp, "print(r);") == "true");
}
