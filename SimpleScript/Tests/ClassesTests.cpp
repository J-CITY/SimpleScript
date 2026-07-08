#include "TestHelpers.hpp"
#include <iostream>

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Class definition and instantiation
// =============================================================================

TEST_CASE("class: define and instantiate", "[classes.basic]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Point {
            var x = 0;
            var y = 0;
            fun Point(a, b) { x = a; y = b; }
        }
        var p = Point(3, 4);
    )");
	if (interp.__EXEPTION__ != IkigaiScript::ExceptionType::None) {
		std::cout << "\n=== EXCEPTION IN TEST ===\n";
		std::cout << "DEBUG OUT:\n" << interp.__DEBUG_OUT__ << "\n";
	}
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarType(interp, "p") == IkigaiScript::Type::Class);
}

TEST_CASE("class: field access via member variable", "[classes.fields]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Box {
            var value = 0;
            fun Box(v) { value = v; }
        }
        var b = Box(99);
        var v = b.value;
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "v") == 99);
}

// NOTE: Class methods that return values or access string members hang.
// Tested: constructor-only with int fields works (see classes.basic).
// Skipping method-call tests until class method resolution is stable.
TEST_CASE("class: method smoke — construct with int field",
	"[classes.methods]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Meter {
            var value = 0;
            fun Meter(v) { value = v; }
        }
        var m = Meter(100);
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarType(interp, "m") == IkigaiScript::Type::Class);
}

TEST_CASE("class: method returns computed value", "[classes.methods]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Point {
            var x = 0;
            var y = 0;
            fun Point(a, b) { x = a; y = b; }
            fun sum() { return x + y; }
        }
        var p = Point(3, 4);
        var a = p.sum();
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "a") == 7);
}

TEST_CASE("class: method returns string field", "[classes.methods]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Greeter {
            var msg = "hello";
            fun Greeter(m) { msg = m; }
            fun get() { return msg; }
        }
        var g = Greeter("world");
        var r = g.get();
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarString(interp, "r") == "world");
}

TEST_CASE("class: multiple instances are independent", "[classes.instances]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Item {
            var val = 0;
            fun Item(v) { val = v; }
        }
        var a = Item(1);
        var b = Item(2);
        var va = a.val;
        var vb = b.val;
    )");
	REQUIRE(getVarInt(interp, "va") == 1);
	REQUIRE(getVarInt(interp, "vb") == 2);
}

TEST_CASE("class: default field values without constructor arg",
	"[classes.fields]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Config {
            var debug = false;
            var version = 1;
            fun Config() { }
        }
        var cfg = Config();
        var d = cfg.debug;
        var v = cfg.version;
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

// =============================================================================
// Class with decorator (regression: decorators still work on classes)
// =============================================================================

TEST_CASE("class: decorator on class scope still accessible via metadata",
	"[classes.decorator]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        @editable
        class MyClass {
            var a = 50;
        }
    )");
	auto classScope = interp.resolveScope("MyClass", interp.getGlobalScope());
	auto classMeta = interp.getMetadata(classScope);
	REQUIRE(classMeta.size() == 1);
	REQUIRE(classMeta[0].name == "editable");
}

TEST_CASE("class: basic inheritance", "[classes.inheritance]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Base {
            var x = 10;
            fun getX() { return x; }
        }
        class Derived, Base {
            var y = 20;
            fun getBoth() { return getX() + y; }
        }
        var d = Derived();
        var vx = d.x;
        var vy = d.y;
        var both = d.getBoth();
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "vx") == 10);
	REQUIRE(getVarInt(interp, "vy") == 20);
	REQUIRE(getVarInt(interp, "both") == 30);
}

TEST_CASE("class: type compatibility with inheritance", "[classes.inheritance]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Base {
            fun name() { return "base"; }
        }
        class Derived, Base {
            fun name() { return "derived"; }
        }
        var obj: Base = Derived();
        var n = obj.name();
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarString(interp, "n") == "derived");
}

TEST_CASE("class: explicit super constructor", "[classes.inheritance]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Base {
            var x = 0;
            fun Base(val) { x = val; }
        }
        class Derived, Base {
            var y = 0;
            fun Derived(v1, v2) {
                super(v1);
                y = v2;
            }
        }
        var d = Derived(42, 100);
        var vx = d.x;
        var vy = d.y;
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "vx") == 42);
	REQUIRE(getVarInt(interp, "vy") == 100);
}

TEST_CASE("class: implicit super constructor", "[classes.inheritance]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Base {
            var x = 10;
            fun Base() { x = 77; }
        }
        class Derived, Base {
            var y = 0;
            fun Derived(v) { y = v; }
        }
        var d = Derived(200);
        var vx = d.x;
        var vy = d.y;
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "vx") == 77);
	REQUIRE(getVarInt(interp, "vy") == 200);
}

TEST_CASE("class: method overriding & dynamic dispatch", "[classes.inheritance]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Base {
            fun greet() { return "hello from base"; }
        }
        class Derived, Base {
            fun greet() { return "hello from derived"; }
        }
        var b = Base();
        var d = Derived();
        var r1 = b.greet();
        var r2 = d.greet();
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarString(interp, "r1") == "hello from base");
	REQUIRE(getVarString(interp, "r2") == "hello from derived");
}

TEST_CASE("class: super method call", "[classes.inheritance]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Base {
            var msg = "base";
            fun greet() { return msg; }
        }
        class Derived, Base {
            fun greet() { return super.greet() + " & derived"; }
        }
        var d = Derived();
        var r = d.greet();
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarString(interp, "r") == "base & derived");
}

TEST_CASE("class: default constructor generation for subclass", "[classes.inheritance]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        class Base {
            var x = 100;
            fun Base() { x = 999; }
        }
        class Derived, Base {
            var y = 200;
        }
        var d = Derived();
        var vx = d.x;
        var vy = d.y;
    )");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "vx") == 999);
	REQUIRE(getVarInt(interp, "vy") == 200);
}
