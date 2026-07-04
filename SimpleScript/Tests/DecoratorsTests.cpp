#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Decorator on variable (migrated)
// =============================================================================

TEST_CASE("decorator: var with arguments", "[decorators.var]") {
    auto interp = makeInterp();
    std::vector<IkigaiScript::Metadata> meta;
    try {
        interp.evaluate(R"(
            @test(min=0, max=100, widget="slider")
            var a = 50;
        )");
        meta = interp.getMetadata(interp.getGlobalScope(), "a");
        REQUIRE(meta.size() == 1);
        REQUIRE(meta[0].name == "test");
    } catch (std::exception& e) {
        FAIL("Exception: " + std::string(e.what()));
    }
    REQUIRE(meta[0].arguments.size() == 3);
    REQUIRE(meta[0].arguments[0].first == "min");
    REQUIRE(meta[0].arguments[0].second->value.asFloat == 0.0);
    REQUIRE(meta[0].arguments[1].first == "max");
    REQUIRE(meta[0].arguments[1].second->value.asFloat == 100.0);
    REQUIRE(meta[0].arguments[2].first == "widget");
    REQUIRE(meta[0].arguments[2].second->value.asString == "slider");
}

// =============================================================================
// Decorator on class and class member (migrated)
// =============================================================================

TEST_CASE("decorator: class-level and member decorator", "[decorators.class]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        @editable
        class MyClass {
            @test(range=0)
            var a = 50;
        }
    )");
    auto classScope = interp.resolveScope("MyClass", interp.getGlobalScope());
    auto classMeta = interp.getMetadata(classScope);
    REQUIRE(classMeta.size() == 1);
    REQUIRE(classMeta[0].name == "editable");

    auto memberMeta = interp.getMetadata(classScope, "a");
    REQUIRE(memberMeta.size() == 1);
    REQUIRE(memberMeta[0].name == "test");
    REQUIRE(memberMeta[0].arguments.size() == 1);
    REQUIRE(memberMeta[0].arguments[0].first == "range");
    REQUIRE(memberMeta[0].arguments[0].second->value.asFloat == 0.0);
}

// =============================================================================
// Decorator on function (migrated)
// =============================================================================

TEST_CASE("decorator: function decorator with args", "[decorators.function]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        @test(min=0, max=100)
        fun a() { return 50; }
    )");
    auto funcMeta = interp.getMetadata(interp.getGlobalScope(), "a");
    REQUIRE(funcMeta.size() == 1);
    REQUIRE(funcMeta[0].name == "test");
    REQUIRE(funcMeta[0].arguments.size() == 2);
    REQUIRE(funcMeta[0].arguments[0].first == "min");
    REQUIRE(funcMeta[0].arguments[0].second->value.asFloat == 0.0);
    REQUIRE(funcMeta[0].arguments[1].first == "max");
    REQUIRE(funcMeta[0].arguments[1].second->value.asFloat == 100.0);
}

// =============================================================================
// Decorator without arguments
// =============================================================================

TEST_CASE("decorator: marker decorator without parens", "[decorators.var]") {
    auto interp = makeInterp();
    interp.evaluate(R"(
        @editable
        var x = 10;
    )");
    auto meta = interp.getMetadata(interp.getGlobalScope(), "x");
    REQUIRE(meta.size() == 1);
    REQUIRE(meta[0].name == "editable");
}
