#include "catch.hpp"
#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace IkigaiScript;
using namespace std::string_literals;

TEST_CASE("Live variables track dependencies", "[LiveVariables]") {
    auto interp = makeInterp();
    
    SECTION("Basic live var with init") {
        noException(interp, R"(
            var a = 1;
            var b = 2;
            live var c = a + b;
        )");
        REQUIRE(getVarInt(interp, "c") == 3);
        
        noException(interp, "a = 5;");
        REQUIRE(getVarInt(interp, "c") == 7);
        
        noException(interp, "b = 10;");
        REQUIRE(getVarInt(interp, "c") == 15);
    }
    
    SECTION("Pending live slot: live var x;, then live x = expr") {
        noException(interp, R"(
            var y = 3;
            live var x;
        )");
        // x is registered as pending — reading it gives null/uninit
        noException(interp, "live x = y + 1;");
        REQUIRE(getVarInt(interp, "x") == 4);
        
        noException(interp, "y = 10;");
        REQUIRE(getVarInt(interp, "x") == 11);
    }
    
    SECTION("Pending live dynamic slot, then rebind") {
        noException(interp, R"(
            var v = 5;
            live dynamic label;
            live label = v * 2;
        )");
        REQUIRE(getVarInt(interp, "label") == 10);
        
        noException(interp, "v = 7;");
        REQUIRE(getVarInt(interp, "label") == 14);
    }
    
    SECTION("Live variable can be dynamic (type-changing)") {
        noException(interp, R"(
            var a = 1;
            live dynamic d = a;
        )");
        REQUIRE(getVarInt(interp, "d") == 1);
        
        noException(interp, R"(a = "hello";)");
        REQUIRE(getVarType(interp, "d") == Type::String);
        REQUIRE(getVarString(interp, "d") == "hello");
    }
    
    SECTION("Live rebind syntax on existing var") {
        noException(interp, R"(
            var a = 1;
            var c = 0;
            live c = a * 2;
        )");
        REQUIRE(getVarInt(interp, "c") == 2);
        
        noException(interp, "a = 10;");
        REQUIRE(getVarInt(interp, "c") == 20);
    }
    
    SECTION("Chained live variables") {
        noException(interp, R"(
            var x = 1;
            live var y = x + 1;
            live var z = y * 2;
        )");
        REQUIRE(getVarInt(interp, "y") == 2);
        REQUIRE(getVarInt(interp, "z") == 4);
        
        noException(interp, "x = 5;");
        REQUIRE(getVarInt(interp, "y") == 6);
        REQUIRE(getVarInt(interp, "z") == 12);
    }
    
    SECTION("Live binding tracks multiple dependencies") {
        noException(interp, R"(
            var a = 1;
            var b = 2;
            var c = 3;
            live var sum = a + b + c;
        )");
        REQUIRE(getVarInt(interp, "sum") == 6);
        
        noException(interp, "a = 10;");
        REQUIRE(getVarInt(interp, "sum") == 15);
        noException(interp, "b = 10;");
        REQUIRE(getVarInt(interp, "sum") == 23);
        noException(interp, "c = 10;");
        REQUIRE(getVarInt(interp, "sum") == 30);
    }
    
    SECTION("Live rebind changes guard completely") {
        noException(interp, R"(
            var a = 10;
            var b = 20;
            var x = 0;
            live x = a;
        )");
        REQUIRE(getVarInt(interp, "x") == 10);
        
        noException(interp, "a = 15;");
        REQUIRE(getVarInt(interp, "x") == 15);
        
        noException(interp, "live x = b;");
        REQUIRE(getVarInt(interp, "x") == 20);
        
        // changing a must no longer affect x after rebind
        noException(interp, "a = 99;");
        REQUIRE(getVarInt(interp, "x") == 20);
        
        noException(interp, "b = 30;");
        REQUIRE(getVarInt(interp, "x") == 30);
    }

    SECTION("Direct assignment to active live variable throws") {
        noException(interp, R"(
            var a = 5;
            live var b = a * 2;
        )");
        REQUIRE(getVarInt(interp, "b") == 10);
        
        // Direct write to an active live target must throw.
        REQUIRE(hadException(interp, "b = 100;", ExceptionType::Unknown));
        // Value must remain reactive after the failed assignment.
        noException(interp, "a = 10;");
        REQUIRE(getVarInt(interp, "b") == 20);
    }
    
    SECTION("Mutual cycle detection throws") {
        // live a = b + 1; live b = a + 1; — mutual cycle must throw.
        REQUIRE(hadException(interp, R"(
            var a = 0;
            var b = 0;
            live a = b + 1;
            live b = a + 1;
        )", ExceptionType::Unknown));
    }
    
    SECTION("Transaction rollback does not trigger live recompute") {
        noException(interp, R"(
            var x = 1;
            live var c = x + 1;
        )");
        REQUIRE(getVarInt(interp, "c") == 2);
        
        // The transaction modifies x but then throws — rollback must leave c unchanged.
        noException(interp, ">>>{ x = 10; var err = 1 / 0; };");
        REQUIRE(getVarInt(interp, "x") == 1);
        REQUIRE(getVarInt(interp, "c") == 2);
    }
    
    SECTION("Transaction commit triggers live recompute") {
        noException(interp, R"(
            var x = 1;
            live var c = x + 1;
        )");
        REQUIRE(getVarInt(interp, "c") == 2);
        
        noException(interp, ">>>{ x = 10; };");
        REQUIRE(getVarInt(interp, "x") == 10);
        REQUIRE(getVarInt(interp, "c") == 11);
    }
    
    SECTION("Defer does not interfere with live graph") {
        noException(interp, R"(
            var a = 1;
            live var b = a + 1;
            fun testFn() {
                defer { a = 5; };
            };
            testFn();
        )");
        // After defer fires (a = 5), b should recompute to 6.
        REQUIRE(getVarInt(interp, "a") == 5);
        REQUIRE(getVarInt(interp, "b") == 6);
    }
}
