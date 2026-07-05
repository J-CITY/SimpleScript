#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("range: .. exclusive print", "[range.exclusive]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var r = 1..5; print(r);") == "1..5");
}

TEST_CASE("range: ..= inclusive print", "[range.inclusive]") {
    auto interp = makeInterp();
    REQUIRE(run(interp, "var r = 1..=5; print(r);") == "1..=5");
}

TEST_CASE("range: for loop with exclusive range sums 1..4", "[range.for.exclusive]") {
    auto interp = makeInterp();
    interp.evaluate("var result = 0;");
    interp.evaluate("for (i : 1..5) { result = result + i; }");
    // sum 1+2+3+4 = 10
    REQUIRE(run(interp, "print(result);") == "10");
}

TEST_CASE("range: for loop with inclusive range sums 1..5", "[range.for.inclusive]") {
    auto interp = makeInterp();
    interp.evaluate("var result = 0;");
    interp.evaluate("for (i : 1..=5) { result = result + i; }");
    // sum 1+2+3+4+5 = 15
    REQUIRE(run(interp, "print(result);") == "15");
}

TEST_CASE("range: for loop 0..3 iterates 3 times", "[range.for.zero]") {
    auto interp = makeInterp();
    interp.evaluate("var count = 0;");
    interp.evaluate("for (i : 0..3) { count = count + 1; }");
    REQUIRE(run(interp, "print(count);") == "3");
}

TEST_CASE("range: empty range 5..5 iterates 0 times", "[range.empty]") {
    auto interp = makeInterp();
    interp.evaluate("var count = 0;");
    interp.evaluate("for (i : 5..5) { count = count + 1; }");
    REQUIRE(run(interp, "print(count);") == "0");
}

TEST_CASE("range: inclusive single element 5..=5", "[range.single]") {
    auto interp = makeInterp();
    interp.evaluate("var count = 0;");
    interp.evaluate("for (i : 5..=5) { count = count + 1; }");
    REQUIRE(run(interp, "print(count);") == "1");
}

TEST_CASE("range: range() builtin inclusive (compatibility)", "[range.builtin.compat]") {
    auto interp = makeInterp();
    interp.evaluate("var arr = range(0, 3);");
    interp.evaluate("var sum = 0;");
    interp.evaluate("for (x : arr) { sum = sum + x; }");
    // range(0,3) inclusive → 0+1+2+3 = 6
    REQUIRE(run(interp, "print(sum);") == "6");
}

TEST_CASE("range: last value in loop is end of range", "[range.collect]") {
    auto interp = makeInterp();
    interp.evaluate("var last = -1;");
    interp.evaluate("for (x : 0..=4) { last = x; }");
    REQUIRE(run(interp, "print(last);") == "4");
}
