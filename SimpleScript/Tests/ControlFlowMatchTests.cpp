#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("match: literal int arms", "[match.int]") {
    auto interp = makeInterp();
    // Single-line version of [match.stmt] - should behave identically
    interp.evaluate("var n = 3; var out = 0; match (n) { case 1 => { out = 10; } case 3 => { out = 30; } default => { out = -1; } };");
    REQUIRE(run(interp, "print(out);") == "30");
}

TEST_CASE("match: default arm", "[match.default]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var n = 99;
        var r = match (n) {
            case 1 => { "one"; }
            default => { "other"; }
        };
    )");
    REQUIRE(run(interp, "print(r);") == "other");
}

TEST_CASE("match: string arms", "[match.string]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var s = "hello";
        var r = match (s) {
            case "hi"    => { 1; }
            case "hello" => { 2; }
            default      => { 0; }
        };
    )");
    REQUIRE(run(interp, "print(r);") == "2");
}

TEST_CASE("match: bool arms", "[match.bool]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var b = true;
        var r = match (b) {
            case false => { 0; }
            case true  => { 1; }
        };
    )");
    REQUIRE(run(interp, "print(r);") == "1");
}

TEST_CASE("match: no match throws", "[match.nomatch]") {
    auto interp = makeInterp();
    run(interp, R"(
        var n = 42;
        match (n) {
            case 1 => { }
            case 2 => { }
        };
    )");
    REQUIRE(interp.__EXEPTION__ != ExceptionType::None);
}

TEST_CASE("match: statement form with side effect", "[match.stmt]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var n = 3;
        var out = 0;
        match (n) {
            case 1 => { out = 10; }
            case 3 => { out = 30; }
            default => { out = -1; }
        };
    )");
    REQUIRE(run(interp, "print(out);") == "30");
}

TEST_CASE("match: expression result", "[match.expr]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var n = 1;
        var r = match (n) { case 1 => { 100; } default => { 0; } };
    )");
    REQUIRE(run(interp, "print(r);") == "100");
}

TEST_CASE("match: float arms", "[match.float]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var x = 2.5;
        var r = match (x) {
            case 1.0 => { "one"; }
            case 2.5 => { "two.five"; }
            default  => { "other"; }
        };
    )");
    REQUIRE(run(interp, "print(r);") == "two.five");
}

TEST_CASE("match: range pattern exclusive", "[match.range]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var n = 3;
        var r = match (n) {
            case 1..5 => { "small"; }
            default   => { "large"; }
        };
    )");
    REQUIRE(run(interp, "print(r);") == "small");
}

TEST_CASE("match: range pattern boundary exclusive", "[match.range.boundary]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var n = 5;
        var r = match (n) {
            case 1..5 => { "in"; }
            default   => { "out"; }
        };
    )");
    // 1..5 is exclusive, so 5 is NOT in range
    REQUIRE(run(interp, "print(r);") == "out");
}

TEST_CASE("match: range pattern inclusive", "[match.range.inclusive]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var n = 5;
        var r = match (n) {
            case 1..=5 => { "in"; }
            default    => { "out"; }
        };
    )");
    REQUIRE(run(interp, "print(r);") == "in");
}

TEST_CASE("match: wildcard _ arm", "[match.wildcard]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        var n = 42;
        var r = match (n) {
            case 1 => { "one"; }
            case _ => { "other"; }
        };
    )");
    REQUIRE(run(interp, "print(r);") == "other");
}
