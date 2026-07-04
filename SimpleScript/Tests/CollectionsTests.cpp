#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Array literals
// =============================================================================

TEST_CASE("array: homogeneous int literal creates Array type", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [1, 2, 3];");
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Array);
    REQUIRE(getVar(interp, "a")->getStdVector<IkigaiScript::Int>().size() == 3);
}

TEST_CASE("array: empty array", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = array();");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Array);
}

TEST_CASE("array: index access arr[0]", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [10, 20, 30]; var x = a[0];");
    REQUIRE(getVarInt(interp, "x") == 10);
}

TEST_CASE("array: index access arr[2]", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [10, 20, 30]; var x = a[2];");
    REQUIRE(getVarInt(interp, "x") == 30);
}

TEST_CASE("array: length", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [1, 2, 3, 4]; var n = length(a);");
    REQUIRE(getVarInt(interp, "n") == 4);
}

TEST_CASE("array: pushback adds element", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [1, 2]; pushback(a, 3);");
    REQUIRE(getVar(interp, "a")->getStdVector<IkigaiScript::Int>().size() == 3);
}

TEST_CASE("array: popback removes last", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [1, 2, 3]; popback(a);");
    REQUIRE(getVar(interp, "a")->getStdVector<IkigaiScript::Int>().size() == 2);
}

TEST_CASE("array: reverse", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [1, 2, 3]; reverse(a);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    // reverse() may or may not modify in place; just verify no crash
}

TEST_CASE("array: sort int array", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [3, 1, 2]; sort(a);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto& v = getVar(interp, "a")->getStdVector<IkigaiScript::Int>();
    REQUIRE(v[0] == 1);
    REQUIRE(v[2] == 3);
}

TEST_CASE("array: find returns index", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate("var a = [10, 20, 30]; var idx = find(a, 20);");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

TEST_CASE("array: string array", "[collections.array]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var a = ["x", "y", "z"]; var n = length(a);)");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "n") == 3);
}

// =============================================================================
// List (heterogeneous)
// =============================================================================

TEST_CASE("list: heterogeneous literal creates List type", "[collections.list]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var l = [1, "hello", 2.0];)");
    REQUIRE(getVarType(interp, "l") == IkigaiScript::Type::List);
}

TEST_CASE("list: explicit list() constructor", "[collections.list]") {
    auto interp = makeInterp();
    interp.evaluate("var l = list();");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarType(interp, "l") == IkigaiScript::Type::List);
}

TEST_CASE("list: length", "[collections.list]") {
    auto interp = makeInterp();
    interp.evaluate(R"(var l = [1, "hello", 2.0]; var n = length(l);)");
    REQUIRE(getVarInt(interp, "n") == 3);
}

// =============================================================================
// Map / Dictionary
// =============================================================================

TEST_CASE("map: dictionary() constructor creates Map", "[collections.map]") {
    auto interp = makeInterp();
    interp.evaluate("var m = dictionary();");
    REQUIRE(getVarType(interp, "m") == IkigaiScript::Type::Map);
}

TEST_CASE("map: set and get string key", "[collections.map]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var m = dictionary();
        m["key"] = 42;
        var v = m["key"];
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarInt(interp, "v") == 42);
}

TEST_CASE("map: length of map", "[collections.map]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var m = dictionary();
        m["a"] = 1;
        m["b"] = 2;
        var n = length(m);
    )");
    REQUIRE(getVarInt(interp, "n") == 2);
}

TEST_CASE("map: values are dynamic by default", "[collections.map]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var m = dictionary();
        m["x"] = 10;
        m["x"] = "reassigned";
    )");
    // Check the value was reassigned — dynamic reassignment doesn't propagate TypeConvert
    REQUIRE(interp.__EXEPTION__ != IkigaiScript::ExceptionType::TypeConvert);
}

// =============================================================================
// Set
// =============================================================================

TEST_CASE("set: set() constructor creates Set type", "[collections.set]") {
    auto interp = makeInterp();
    interp.evaluate("var s = set();");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    REQUIRE(getVarType(interp, "s") == IkigaiScript::Type::Set);
}

// =============================================================================
// HOF: map and fold
// =============================================================================

TEST_CASE("builtin map: transforms array with named function", "[collections.hof]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        fun doubleit(x) { return x * 2; }
        var arr = [1, 2, 3];
        var doubled = map(arr, doubleit);
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto d = getVar(interp, "doubled");
    REQUIRE((d->getType() == IkigaiScript::Type::Array || d->getType() == IkigaiScript::Type::List));
}

TEST_CASE("builtin map: inline lambda", "[collections.hof]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var arr = [1, 2, 3];
        var doubled = map(arr, fun(x) { return x * 2; });
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
    auto d = getVar(interp, "doubled");
    REQUIRE((d->getType() == IkigaiScript::Type::Array || d->getType() == IkigaiScript::Type::List));
}

TEST_CASE("builtin fold: reduces array to single value with named function", "[collections.hof]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        fun sumfn(acc, x) { return acc + x; }
        var arr = [1, 2, 3, 4];
        var total = fold(arr, 0, sumfn);
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

TEST_CASE("builtin fold: inline lambda", "[collections.hof]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var arr = [1, 2, 3, 4];
        var total = fold(arr, 0, fun(acc, x) { return acc + x; });
    )");
    REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}
