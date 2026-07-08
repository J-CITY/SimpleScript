#include "TestHelpers.hpp"
#include <fstream>
#include <cstdio>

// Bytecode VM dual-run test suite.
//
// The helper `expectSameOutput` runs the same script in both Interpreter and
// Bytecode mode and asserts the print output is identical.  Any divergence
// indicates a semantic difference between the two paths.

using namespace TestHelpers;
using namespace IkigaiScript;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string runBytecode(const std::string& code) {
	IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
	interp.setExecutionMode(ExecutionMode::Bytecode);
	interp.__DEBUG_OUT__ = "";
	interp.__EXEPTION__ = ExceptionType::None;
	interp.evaluate(code);
	return interp.__DEBUG_OUT__;
}

// Run in both modes and verify output matches.
static void expectSameOutput(const std::string& code) {
	auto interpOut = runFresh(code);
	auto bcOut = runBytecode(code);
	// Use a fresh interpreter for comparison since runFresh creates one
	INFO("Interpreter: " << interpOut);
	INFO("Bytecode:    " << bcOut);
	REQUIRE(interpOut == bcOut);
}

// ---------------------------------------------------------------------------
// Phase 2a: literals, arithmetic, variable definition
// ---------------------------------------------------------------------------

TEST_CASE("BC: integer literal", "[bytecode]") {
	expectSameOutput(R"(
        var x = 42;
        print(x);
    )");
}

TEST_CASE("BC: arithmetic", "[bytecode]") {
	expectSameOutput(R"(
        var a = 3 + 4 * 2;
        print(a);
    )");
}

TEST_CASE("BC: float arithmetic", "[bytecode]") {
	expectSameOutput(R"(
        var x: Float = 1.5 + 2.5;
        print(x);
    )");
}

TEST_CASE("BC: string concatenation", "[bytecode]") {
	expectSameOutput(R"(
        var s = "hello" + " " + "world";
        print(s);
    )");
}

TEST_CASE("BC: boolean logic", "[bytecode]") {
	expectSameOutput(R"(
        var a = true && false;
        var b = true || false;
        print(a);
        print(b);
    )");
}

// ---------------------------------------------------------------------------
// Phase 2a: if / while
// ---------------------------------------------------------------------------

TEST_CASE("BC: if-else", "[bytecode]") {
	expectSameOutput(R"(
        var x = 10;
        if (x > 5) {
            print("big");
        } else {
            print("small");
        };
    )");
}

TEST_CASE("BC: while loop", "[bytecode]") {
	expectSameOutput(R"(
        var i = 0;
        while (i < 3) {
            print(i);
            i = i + 1;
        };
    )");
}

TEST_CASE("BC: for loop", "[bytecode]") {
	expectSameOutput(R"(
        for (var i = 0; i < 3; i = i + 1) {
            print(i);
        };
    )");
}

// ---------------------------------------------------------------------------
// Phase 2a: functions
// ---------------------------------------------------------------------------

TEST_CASE("BC: simple function call", "[bytecode]") {
	expectSameOutput(R"(
        fun add(a: Int, b: Int): Int {
            return a + b;
        };
        print(add(3, 4));
    )");
}

TEST_CASE("BC: recursive function", "[bytecode]") {
	expectSameOutput(R"(
        fun fact(n: Int): Int {
            if (n <= 1) { return 1; };
            return n * fact(n - 1);
        };
        print(fact(5));
    )");
}

TEST_CASE("BC: function returns value", "[bytecode]") {
	expectSameOutput(R"(
        fun double(x: Int): Int { return x * 2; };
        var r = double(7);
        print(r);
    )");
}

// ---------------------------------------------------------------------------
// Phase 2b: operators, comparison, short-circuit
// ---------------------------------------------------------------------------

TEST_CASE("BC: comparison operators", "[bytecode]") {
	expectSameOutput(R"(
        print(1 < 2);
        print(2 > 1);
        print(1 == 1);
        print(1 != 2);
        print(1 <= 1);
        print(2 >= 2);
    )");
}

TEST_CASE("BC: short-circuit &&", "[bytecode]") {
	expectSameOutput(R"(
        var x = false && (1/0 > 0);
        print(x);
    )");
}

TEST_CASE("BC: short-circuit ||", "[bytecode]") {
	expectSameOutput(R"(
        var x = true || (1/0 > 0);
        print(x);
    )");
}

// ---------------------------------------------------------------------------
// Phase 2b: for-in / foreach
// ---------------------------------------------------------------------------

TEST_CASE("BC: for-in list", "[bytecode]") {
	expectSameOutput(R"(
        var arr = [1, 2, 3];
        for (x : arr) {
            print(x);
        };
    )");
}

TEST_CASE("BC: for-in range", "[bytecode]") {
	expectSameOutput(R"(
        for (i : 0..3) {
            print(i);
        };
    )");
}

// ---------------------------------------------------------------------------
// Phase 2b: match
// ---------------------------------------------------------------------------

TEST_CASE("BC: match expression", "[bytecode]") {
	expectSameOutput(R"(
        var n = 2;
        var r = match (n) {
            case 1 => { "one"; }
            case 2 => { "two"; }
            default => { "other"; }
        };
        print(r);
    )");
}

// ---------------------------------------------------------------------------
// Phase 4: closures (lambdas)
// ---------------------------------------------------------------------------

TEST_CASE("BC: lambda in higher-order function", "[bytecode]") {
	expectSameOutput(R"(
        var arr = [1, 2, 3];
        var doubled = map(arr, fun(x: Int): Int { return x * 2; });
        for (v : doubled) { print(v); };
    )");
}

// ---------------------------------------------------------------------------
// Phase 4: coroutines (basic yield/await)
// ---------------------------------------------------------------------------

TEST_CASE("BC: basic coro yield and await", "[bytecode]") {
	expectSameOutput(R"(
        coro gen() {
            yield 1;
            yield 2;
            return 3;
        };
        var t = gen();
        print(await t);
    )");
}

// ---------------------------------------------------------------------------
// ExecutionMode API
// ---------------------------------------------------------------------------

TEST_CASE("BC: setExecutionMode switches modes", "[bytecode]") {
	IkigaiScriptInterpreter interp;
	REQUIRE(interp.getExecutionMode() == ExecutionMode::Interpreter);
	interp.setExecutionMode(ExecutionMode::Bytecode);
	REQUIRE(interp.getExecutionMode() == ExecutionMode::Bytecode);
	interp.setExecutionMode(ExecutionMode::Interpreter);
	REQUIRE(interp.getExecutionMode() == ExecutionMode::Interpreter);
}

TEST_CASE("BC: compileFunctionToBytecode skips generic functions", "[bytecode]") {
	IkigaiScriptInterpreter interp;
	interp.evaluate("fun identity<T>(x: T): T { return x; };");
	auto fnc = interp.resolveFunction("identity");
	REQUIRE(fnc != nullptr);
	// Generic functions must not be compiled to bytecode.
	bool compiled = interp.compileFunctionToBytecode(fnc);
	REQUIRE_FALSE(compiled);
	REQUIRE(fnc->getBodyType() == FunctionBodyType::Subexpressions);
}

TEST_CASE("BC: forceInterpret prevents bytecode compilation", "[bytecode]") {
	IkigaiScriptInterpreter interp;
	interp.evaluate("fun simple(x: Int): Int { return x + 1; };");
	auto fnc = interp.resolveFunction("simple");
	REQUIRE(fnc != nullptr);
	fnc->forceInterpret = true;
	bool compiled = interp.compileFunctionToBytecode(fnc);
	REQUIRE_FALSE(compiled);
}

TEST_CASE("BC: compileFunctionToBytecode produces Bytecode body", "[bytecode]") {
	IkigaiScriptInterpreter interp;
	interp.evaluate("fun add(a: Int, b: Int): Int { return a + b; };");
	auto fnc = interp.resolveFunction("add");
	REQUIRE(fnc != nullptr);
	bool compiled = interp.compileFunctionToBytecode(fnc);
	REQUIRE(compiled);
	REQUIRE(fnc->getBodyType() == FunctionBodyType::Bytecode);
}

// ---------------------------------------------------------------------------
// Compile I/O API
// ---------------------------------------------------------------------------

TEST_CASE("BC: compileScript returns non-null chunk with functions", "[bytecode][compile_io]") {
	IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
	auto chunk = interp.compileScript("fun f(): Int { return 42; };");
	REQUIRE(chunk != nullptr);
	REQUIRE(chunk->functions.count("f") > 0);
}

TEST_CASE("BC: compileScript does not run top-level code", "[bytecode][compile_io]") {
	IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
	interp.__DEBUG_OUT__ = "";
	auto chunk = interp.compileScript("print(\"should not print\");");
	REQUIRE(chunk != nullptr);
	REQUIRE(interp.__DEBUG_OUT__.empty());
}

TEST_CASE("BC: compileScriptFile compiles a .ss file", "[bytecode][compile_io]") {
	// Write a temp script file.
	const std::string tmpPath = "test_compile_io_tmp.ss";
	{
		std::ofstream f(tmpPath);
		f << "fun square(n: Int): Int { return n * n; };";
	}
	IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
	auto chunk = interp.compileScriptFile(tmpPath);
	REQUIRE(chunk != nullptr);
	REQUIRE(chunk->main != nullptr);
	std::remove(tmpPath.c_str());
}

TEST_CASE("BC: serialize/deserialize round-trip preserves output", "[bytecode][compile_io]") {
	const std::string code = R"(
        fun greet(name: String): String {
            return "hello " + name;
        };
        print(greet("world"));
    )";

	// Compile → serialize → deserialize → run
	IkigaiScriptInterpreter compileInterp(ModulePrivilege::allPrivilege);
	auto chunk = compileInterp.compileScript(code);
	REQUIRE(chunk != nullptr);

	std::string blob = compileInterp.serializeCompiledScript(*chunk);
	REQUIRE(blob.size() > 4);
	REQUIRE(blob.substr(0, 4) == "IKBC");

	IkigaiScriptInterpreter runInterp(ModulePrivilege::allPrivilege);
	runInterp.__DEBUG_OUT__ = "";
	auto loaded = runInterp.deserializeCompiledScript(blob);
	REQUIRE(loaded != nullptr);
	runInterp.runCompiledScript(loaded);

	REQUIRE(runInterp.__DEBUG_OUT__ == "hello world");
}

TEST_CASE("BC: saveCompiledScript / loadCompiledScriptFile round-trip", "[bytecode][compile_io]") {
	const std::string code = "fun add(a: Int, b: Int): Int { return a + b; }; print(add(3, 4));";
	const std::string tmpPath = "test_compile_io_roundtrip.ikbc";

	IkigaiScriptInterpreter compileInterp(ModulePrivilege::allPrivilege);
	auto chunk = compileInterp.compileScript(code);
	REQUIRE(chunk != nullptr);
	bool saved = compileInterp.saveCompiledScript(*chunk, tmpPath);
	REQUIRE(saved);

	IkigaiScriptInterpreter runInterp(ModulePrivilege::allPrivilege);
	runInterp.__DEBUG_OUT__ = "";
	runInterp.runCompiledScriptFile(tmpPath);
	REQUIRE(runInterp.__DEBUG_OUT__ == "7");

	std::remove(tmpPath.c_str());
}

TEST_CASE("BC: runCompiledScript evaluate-then-run", "[bytecode][compile_io]") {
	IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
	interp.setExecutionMode(ExecutionMode::Bytecode);
	interp.__DEBUG_OUT__ = "";
	interp.evaluate("print(100 + 1);");
	REQUIRE(interp.__DEBUG_OUT__ == "101");
}

TEST_CASE("BC: runCompiledScript in-memory (no serialize)", "[bytecode][compile_io]") {
	const std::string code = "print(100 + 1);";

	IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
	auto chunk = interp.compileScript(code);
	REQUIRE(chunk != nullptr);
	REQUIRE(chunk->main != nullptr);
	REQUIRE(chunk->main->code.size() > 0);

	interp.__DEBUG_OUT__ = "";
	interp.runCompiledScript(chunk);
	REQUIRE(interp.__DEBUG_OUT__ == "101");
}

TEST_CASE("BC: runCompiledScript executes main chunk body", "[bytecode][compile_io]") {
	const std::string code = "print(100 + 1);";

	IkigaiScriptInterpreter compileInterp(ModulePrivilege::allPrivilege);
	auto chunk = compileInterp.compileScript(code);
	REQUIRE(chunk != nullptr);
	REQUIRE(chunk->main != nullptr);
	REQUIRE(chunk->main->code.size() > 0);

	std::string blob = compileInterp.serializeCompiledScript(*chunk);
	REQUIRE(blob.size() > 4);
	REQUIRE(blob.substr(0, 4) == "IKBC");

	IkigaiScriptInterpreter runInterp(ModulePrivilege::allPrivilege);
	auto loaded = runInterp.deserializeCompiledScript(blob);
	REQUIRE(loaded != nullptr);
	REQUIRE(loaded->main != nullptr);
	REQUIRE(loaded->main->code.size() == chunk->main->code.size());

	runInterp.__DEBUG_OUT__ = "";
	runInterp.runCompiledScript(loaded);
	REQUIRE(runInterp.__DEBUG_OUT__ == "101");
}
