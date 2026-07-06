#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("transactions: outer var commit", "[tx.commit]") {
    auto interp = makeInterp();
    interp.evaluate("var x = 1;");
    auto res = run(interp, "print(>>>{ x = 2; var dummy = 0; });");
    REQUIRE(res == "true");
    REQUIRE(run(interp, "print(x);") == "2");
}

TEST_CASE("transactions: outer var rollback", "[tx.rollback]") {
    auto interp = makeInterp();
    interp.evaluate("var x = 1;");
    auto res = run(interp, "print(>>>{ x = 2; var err = 1 / 0; var dummy = 0; });");
    REQUIRE(res == "false");
    REQUIRE(run(interp, "print(x);") == "1");
}

TEST_CASE("transactions: local var rollback", "[tx.local]") {
    auto interp = makeInterp();
    run(interp, ">>>{ var y = 5; var err = 1 / 0; var dummy = 0; };");
    // y не должна существовать снаружи скоупа транзакции
    bool hadError = interp.evaluate("print(y);");
    REQUIRE(hadError == true);
}

TEST_CASE("transactions: optional return success", "[tx.optional.success]") {
    auto interp = makeInterp();
    interp.evaluate(
        "var res = >>>{\n"
        "    var a = 40;\n"
        "    return a + 2;\n"
        "};"
    );
    REQUIRE(run(interp, "print(res);") == "some(42)");
    REQUIRE(run(interp, "print(optionalHas(res));") == "true");
    REQUIRE(run(interp, "print(optionalGet(res));") == "42");
}

TEST_CASE("transactions: optional return fail", "[tx.optional.fail]") {
    auto interp = makeInterp();
    interp.evaluate(
        "var res = >>>{\n"
        "    var a = 42;\n"
        "    var err = 1 / 0;\n"
        "    return a;\n"
        "};"
    );
    REQUIRE(run(interp, "print(optionalHas(res));") == "false");
}

TEST_CASE("transactions: tail value return success", "[tx.tail.success]") {
    auto interp = makeInterp();
    interp.evaluate(
        "var res = >>>{\n"
        "    var x = 100;\n"
        "    x + 5;\n"
        "};"
    );
    REQUIRE(run(interp, "print(optionalHas(res));") == "true");
    REQUIRE(run(interp, "print(optionalGet(res));") == "105");
}

TEST_CASE("transactions: defer execution on commit", "[tx.defer.commit]") {
    auto interp = makeInterp();
    interp.evaluate(
        "var log = \"\";\n"
        ">>>{ \n"
        "    defer { log = log + \"A\"; }\n"
        "    log = log + \"1\";\n"
        "};"
    );
    REQUIRE(run(interp, "print(log);") == "1A");
}

TEST_CASE("transactions: defer skipped on rollback", "[tx.defer.rollback]") {
    auto interp = makeInterp();
    interp.evaluate(
        "var log = \"\";\n"
        ">>>{ \n"
        "    defer { log = log + \"A\"; }\n"
        "    log = log + \"1\";\n"
        "    var err = 1 / 0;\n"
        "};"
    );
    // Изменения log = log + "1" должны быть откатаны, а дефер не должен выполниться
    REQUIRE(run(interp, "print(log);") == "");
}

TEST_CASE("transactions: nested transactions", "[tx.nested]") {
    auto interp = makeInterp();
    interp.evaluate(
        "var x = 10;\n"
        "var outer_res = >>>{\n"
        "    x = 20;\n"
        "    var inner_res = >>>{\n"
        "        x = 30;\n"
        "        var err = 1 / 0;\n"
        "    };\n"
        "    print(inner_res);\n" // false
        "};\n"
    );
    REQUIRE(run(interp, "print(x);") == "20");
}
