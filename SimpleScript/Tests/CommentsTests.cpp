#include "TestHelpers.hpp"
#include "../Library/lexer.h"
#include <iostream>
#include <vector>

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("comments: inline comment at the beginning of a line", "[comments.inline.start]") {
    auto interp = makeInterp();
    interp.evaluate("// var x = 10;");
    interp.evaluate("var y = 20;");
    REQUIRE(run(interp, "print(y);") == "20");
    REQUIRE(hadException(interp, "print(x);", ExceptionType::Unknown));
}

TEST_CASE("comments: inline comment after statement", "[comments.inline.after]") {
    auto interp = makeInterp();
    interp.evaluate("var x = 42; // this is a comment");
    REQUIRE(run(interp, "print(x);") == "42");
}

TEST_CASE("comments: inside control flow block", "[comments.control_flow]") {
    auto interp = makeInterp();
    interp.evaluate(
        "var x = 0;\n"
        "if (true) {\n"
        "    // x = 10;\n"
        "    x = 20; // set x\n"
        "}\n"
    );
    REQUIRE(run(interp, "print(x);") == "20");
}

TEST_CASE("comments: inside loop block", "[comments.loop]") {
    std::string script = 
        "var sum = 0;\n"
        "var i = 0;\n"
        "for (i = 0; i < 3; i = i + 1) {\n"
        "    // sum = sum + 100;\n"
        "    sum = sum + i;\n"
        "}\n";

    auto interp = makeInterp();
    interp.evaluate(script);
    REQUIRE(run(interp, "print(sum);") == "3");
}

TEST_CASE("comments: inside function definition", "[comments.function]") {
    auto interp = makeInterp();
    interp.evaluate(
        "fun test() {\n"
        "    // return 1;\n"
        "    return 2; // return value\n"
        "}\n"
    );
    REQUIRE(run(interp, "print(test());") == "2");
}

TEST_CASE("comments: with special characters and symbols", "[comments.special_chars]") {
    auto interp = makeInterp();
    interp.evaluate("var x = 100; // !@#$%^&*()_+=-{}[]|\\:\";'<>?,./");
    REQUIRE(run(interp, "print(x);") == "100");
}
