#include "TestHelpers.hpp"
#include "../Library/native/native_class_builder.hpp"
#include <iostream>
#include <atomic>

using namespace TestHelpers;
using namespace IkigaiScript;
using namespace IkigaiScript::Native;
using namespace std::string_literals;

// ---------------------------------------------------------------------------
// Reference native struct: Vec2
// ---------------------------------------------------------------------------
struct Vec2 {
    float x = 0.f, y = 0.f;
    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    explicit Vec2(float v) : x(v), y(v) {}

    Vec2 sum(const Vec2& o)  const { return { x + o.x, y + o.y }; }
    Vec2 scale(float s)      const { return { x * s,   y * s   }; }
    float dot(const Vec2& o) const { return x * o.x + y * o.y;    }
    float length()           const { return std::sqrt(x*x + y*y); }

    Vec2 operator+(const Vec2& o)  const { return sum(o); }
    Vec2 operator*(float s)        const { return scale(s); }
    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
};

// =============================================================================
// Phase 0 – legacy newClass + ClassLambda API
// =============================================================================

TEST_CASE("native legacy: construct and type check", "[native.legacy.basic]") {
	auto interp = makeInterp();

	auto ctor = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["x"] = std::make_shared<Value>(args[0]->getInt());
		inst->variables["y"] = std::make_shared<Value>(args[1]->getInt());
		return nullptr;
	};
	interp.newClass("Point",
		{{"x", std::make_shared<Value>(Int(0))}, {"y", std::make_shared<Value>(Int(0))}},
		ctor, {});

	interp.evaluate(R"(
		var p = Point(3, 4);
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarType(interp, "p") == Type::Class);
}

TEST_CASE("native legacy: field read", "[native.legacy.fields]") {
	auto interp = makeInterp();

	auto ctor = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["x"] = std::make_shared<Value>(args[0]->getInt());
		inst->variables["y"] = std::make_shared<Value>(args[1]->getInt());
		return nullptr;
	};
	interp.newClass("Point",
		{{"x", std::make_shared<Value>(Int(0))}, {"y", std::make_shared<Value>(Int(0))}},
		ctor, {});

	interp.evaluate(R"(
		var p = Point(3, 4);
		var vx = p.x;
		var vy = p.y;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarInt(interp, "vx") == 3);
	REQUIRE(getVarInt(interp, "vy") == 4);
}

TEST_CASE("native legacy: field write", "[native.legacy.fields]") {
	auto interp = makeInterp();

	auto ctor = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["x"] = std::make_shared<Value>(args[0]->getInt());
		inst->variables["y"] = std::make_shared<Value>(args[1]->getInt());
		return nullptr;
	};
	interp.newClass("Point",
		{{"x", std::make_shared<Value>(Int(0))}, {"y", std::make_shared<Value>(Int(0))}},
		ctor, {});

	interp.evaluate(R"(
		var p = Point(3, 4);
		p.x = 10;
		var vx = p.x;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarInt(interp, "vx") == 10);
}

TEST_CASE("native legacy: method call", "[native.legacy.method]") {
	auto interp = makeInterp();

	auto ctor = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["x"] = std::make_shared<Value>(args[0]->getInt());
		inst->variables["y"] = std::make_shared<Value>(args[1]->getInt());
		return nullptr;
	};
	auto sum = [](Class* inst, ScopePtr, const List&) -> ValuePtr {
		auto x = inst->variables.at("x")->getInt();
		auto y = inst->variables.at("y")->getInt();
		return std::make_shared<Value>(x + y);
	};
	interp.newClass("Point",
		{{"x", std::make_shared<Value>(Int(0))}, {"y", std::make_shared<Value>(Int(0))}},
		ctor, {{"sum", sum}});

	interp.evaluate(R"(
		var p = Point(3, 4);
		var s = p.sum();
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarInt(interp, "s") == 7);
}

TEST_CASE("native legacy: method with args", "[native.legacy.method]") {
	auto interp = makeInterp();

	auto ctor = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["val"] = std::make_shared<Value>(args[0]->getInt());
		return nullptr;
	};
	auto add = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		auto v = inst->variables.at("val")->getInt();
		return std::make_shared<Value>(v + args[0]->getInt());
	};
	interp.newClass("Counter",
		{{"val", std::make_shared<Value>(Int(0))}},
		ctor, {{"add", add}});

	interp.evaluate(R"(
		var c = Counter(10);
		var r = c.add(5);
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarInt(interp, "r") == 15);
}

static void setupPointClass(IkigaiScriptInterpreter& interp) {
	auto ctor = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["x"] = std::make_shared<Value>(args[0]->getInt());
		inst->variables["y"] = std::make_shared<Value>(args[1]->getInt());
		return nullptr;
	};
	auto sum = [](Class* inst, ScopePtr, const List&) -> ValuePtr {
		return std::make_shared<Value>(
			inst->variables.at("x")->getInt() + inst->variables.at("y")->getInt());
	};
	interp.newClass("Point",
		{{"x", std::make_shared<Value>(Int(0))}, {"y", std::make_shared<Value>(Int(0))}},
		ctor, {{"sum", sum}});
}

static void expectNativeSameOutput(
	std::function<void(IkigaiScriptInterpreter&)> setup,
	const std::string& code)
{
	auto interpOut = [&]() -> std::string {
		IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
		setup(interp);
		interp.__DEBUG_OUT__ = "";
		interp.__EXEPTION__ = ExceptionType::None;
		interp.evaluate(code);
		return interp.__DEBUG_OUT__;
	}();
	auto bcOut = [&]() -> std::string {
		IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
		interp.setExecutionMode(ExecutionMode::Bytecode);
		setup(interp);
		interp.__DEBUG_OUT__ = "";
		interp.__EXEPTION__ = ExceptionType::None;
		interp.evaluate(code);
		return interp.__DEBUG_OUT__;
	}();
	INFO("Interpreter: " << interpOut);
	INFO("Bytecode:    " << bcOut);
	REQUIRE(interpOut == bcOut);
}

TEST_CASE("native legacy: bytecode dual-run method", "[native.legacy.bytecode]") {
	expectNativeSameOutput(setupPointClass, R"(
		var p = Point(3, 4);
		print(p.sum());
	)");
}

TEST_CASE("native legacy: bytecode dual-run field access", "[native.legacy.bytecode]") {
	expectNativeSameOutput(setupPointClass, R"(
		var p = Point(5, 6);
		print(p.x);
		print(p.y);
	)");
}

// =============================================================================
// Phase 1 – constructor overloads via newConstructorOverload
// =============================================================================

TEST_CASE("native ctor overload: zero vs two args", "[native.ctor.overload]") {
	auto interp = makeInterp();

	// Register class manually with no ctor first
	auto ctor0 = [](Class* inst, ScopePtr, const List&) -> ValuePtr {
		inst->variables["x"] = std::make_shared<Value>(Int(0));
		inst->variables["y"] = std::make_shared<Value>(Int(0));
		return nullptr;
	};
	auto ctor2 = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["x"] = std::make_shared<Value>(args[0]->getInt());
		inst->variables["y"] = std::make_shared<Value>(args[1]->getInt());
		return nullptr;
	};

	// Use newClass with ctor0 as primary
	interp.newClass("Vec",
		{{"x", std::make_shared<Value>(Int(0))}, {"y", std::make_shared<Value>(Int(0))}},
		ctor0, {});

	// Register ctor2 as additional overload
	auto f2 = std::make_shared<Function>("Vec", ctor2);
	f2->argNames = {"a", "b"};
	interp.newConstructorOverload("Vec", interp.getGlobalScope(), f2);

	interp.evaluate(R"(
		var a = Vec();
		var b = Vec(3, 4);
		var ax = a.x;
		var bx = b.x;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarInt(interp, "ax") == 0);
	REQUIRE(getVarInt(interp, "bx") == 3);
}

TEST_CASE("native ctor overload: typed int vs float", "[native.ctor.overload]") {
	auto interp = makeInterp();

	auto ctorInt = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["val"] = std::make_shared<Value>(args[0]->getInt());
		inst->variables["kind"] = std::make_shared<Value>(std::string("int"));
		return nullptr;
	};
	auto ctorFloat = [](Class* inst, ScopePtr, const List& args) -> ValuePtr {
		inst->variables["val"] = std::make_shared<Value>(Float(args[0]->getFloat()));
		inst->variables["kind"] = std::make_shared<Value>(std::string("float"));
		return nullptr;
	};

	interp.newClass("Scalar",
		{{"val", std::make_shared<Value>(Int(0))}, {"kind", std::make_shared<Value>(std::string(""))}},
		ctorInt, {});

	auto fFloat = std::make_shared<Function>("Scalar", ctorFloat);
	fFloat->argNames = {"v"};
	fFloat->argTypes = {TypeDescriptor{Type::Float, true, false, false, false}};
	interp.newConstructorOverload("Scalar", interp.getGlobalScope(), fFloat);

	interp.evaluate(R"(
		var a = Scalar(42);
		var b = Scalar(3.14);
		var ka = a.kind;
		var kb = b.kind;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarString(interp, "ka") == "int");
	REQUIRE(getVarString(interp, "kb") == "float");
}

// =============================================================================
// Phase 3 – custom class operators
// =============================================================================

// =============================================================================
// Phase 2 – NativeClassBuilder<T> (Vec2 reference implementation)
// =============================================================================

static void registerVec2(IkigaiScriptInterpreter& interp) {
	NativeClassBuilder<Vec2>(interp, "Vec2")
		.member("x", &Vec2::x)
		.member("y", &Vec2::y)
		.constructor<>()
		.constructor<float, float>()
		.constructor<float>()
		.method("sum",    &Vec2::sum)
		.method("scale",  &Vec2::scale)
		.method("dot",    &Vec2::dot)
		.method("length", &Vec2::length)
		.operator_("+", static_cast<Vec2(Vec2::*)(const Vec2&) const>(&Vec2::operator+))
		.operator_("*", [](const Vec2& a, float s) -> Vec2 { return a * s; })
		.finalize();
}

TEST_CASE("native builder: construct and type check", "[native.basic]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var v = Vec2(1.0, 2.0);
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarType(interp, "v") == Type::Class);
}

TEST_CASE("native builder: field read", "[native.basic]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var v = Vec2(3.0, 4.0);
		var vx = v.x;
		var vy = v.y;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "vx") == Approx(3.0));
	REQUIRE(getVarFloat(interp, "vy") == Approx(4.0));
}

TEST_CASE("native builder: field write", "[native.basic]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var v = Vec2(1.0, 2.0);
		v.x = 10.0;
		var vx = v.x;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "vx") == Approx(10.0));
}

TEST_CASE("native builder: default constructor", "[native.basic]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var v = Vec2();
		var vx = v.x;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "vx") == Approx(0.0));
}

TEST_CASE("native builder: method sum", "[native.method]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var a = Vec2(1.0, 2.0);
		var b = Vec2(3.0, 4.0);
		var c = a.sum(b);
		var cx = c.x;
		var cy = c.y;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "cx") == Approx(4.0));
	REQUIRE(getVarFloat(interp, "cy") == Approx(6.0));
}

TEST_CASE("native builder: method returning scalar", "[native.method]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var v = Vec2(3.0, 4.0);
		var d = v.dot(v);
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "d") == Approx(25.0));
}

TEST_CASE("native builder: constructor overloads", "[native.ctor.overload]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var a = Vec2();
		var b = Vec2(3.0, 4.0);
		var c = Vec2(5.0);
		var ax = a.x;
		var bx = b.x;
		var cx = c.x;
		var cy = c.y;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "ax") == Approx(0.0));
	REQUIRE(getVarFloat(interp, "bx") == Approx(3.0));
	REQUIRE(getVarFloat(interp, "cx") == Approx(5.0));
	REQUIRE(getVarFloat(interp, "cy") == Approx(5.0));
}

TEST_CASE("native builder: operator Vec2 + Vec2", "[native.operator]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var a = Vec2(1.0, 2.0);
		var b = Vec2(3.0, 4.0);
		var c = a + b;
		var cx = c.x;
		var cy = c.y;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "cx") == Approx(4.0));
	REQUIRE(getVarFloat(interp, "cy") == Approx(6.0));
}

TEST_CASE("native builder: operator Vec2 * float", "[native.overload]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var v = Vec2(2.0, 3.0);
		var r = v * 2.0;
		var rx = r.x;
		var ry = r.y;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "rx") == Approx(4.0));
	REQUIRE(getVarFloat(interp, "ry") == Approx(6.0));
}

TEST_CASE("native builder: bytecode dual-run fields", "[native.bytecode]") {
	expectNativeSameOutput(registerVec2, R"(
		var v = Vec2(3.0, 4.0);
		print(v.x);
		print(v.y);
	)");
}

TEST_CASE("native builder: bytecode dual-run method", "[native.bytecode]") {
	expectNativeSameOutput(registerVec2, R"(
		var a = Vec2(1.0, 2.0);
		var b = Vec2(3.0, 4.0);
		var c = a.sum(b);
		print(c.x);
	)");
}

TEST_CASE("native builder: bytecode dual-run length", "[native.bytecode]") {
	expectNativeSameOutput(registerVec2, R"(
		var v = Vec2(3.0, 4.0);
		print(v.length());
	)");
}

// =============================================================================
// Phase 3 – class operators (low-level ClassLambda style, as reference test)
// =============================================================================

TEST_CASE("native operator: Vec2 + Vec2 via builder in interpreter", "[native.operator]") {
	auto interp = makeInterp();
	registerVec2(interp);

	interp.evaluate(R"(
		var a = Vec2(1.0, 2.0);
		var b = Vec2(3.0, 4.0);
		var c = a + b;
		var cx = c.x;
		var cy = c.y;
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "cx") == Approx(4.0));
	REQUIRE(getVarFloat(interp, "cy") == Approx(6.0));
}

// =============================================================================
// Phase 4 – Math module: registerMath() reference implementation
// =============================================================================

// Exports Vec2 as a named module so scripts can `import Math` to use it.
static void registerMath(IkigaiScriptInterpreter& interp) {
	auto mathScope = interp.newModule("Math", 0, {});
	NativeClassBuilder<Vec2>(interp, "Vec2", mathScope)
		.member("x", &Vec2::x)
		.member("y", &Vec2::y)
		.constructor<>()
		.constructor<float, float>()
		.method("sum",    &Vec2::sum)
		.method("dot",    &Vec2::dot)
		.method("length", &Vec2::length)
		.operator_("+", static_cast<Vec2(Vec2::*)(const Vec2&) const>(&Vec2::operator+))
		.operator_("*", [](const Vec2& a, float s) -> Vec2 { return a * s; })
		.finalize();
}

TEST_CASE("math module: Vec2 via registerMath", "[math.module]") {
	auto interp = makeInterp();
	registerMath(interp);

	// Vec2 is in the Math module scope — access it directly via the global scope
	// (newClass with mathScope parent registers the ctor in mathScope)
	interp.evaluate(R"(
		var v = Vec2(3.0, 4.0);
		var l = v.length();
	)");
	REQUIRE(interp.__EXEPTION__ == ExceptionType::None);
	REQUIRE(getVarFloat(interp, "l") == Approx(5.0f).epsilon(0.0001));
}
