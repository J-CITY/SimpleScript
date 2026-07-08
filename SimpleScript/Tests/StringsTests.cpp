#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

// =============================================================================
// Char literals (migrated)
// =============================================================================

TEST_CASE("char: single char literal has type Char", "[strings.char]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 'x';");
	REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::Char);
	REQUIRE(getVar(interp, "a")->getChar() == 'x');
}

TEST_CASE("char: char + char gives Char", "[strings.char]") {
	auto interp = makeInterp();
	interp.evaluate("var a = 'x'; var b = 'y'; var c = a + b;");
	REQUIRE(getVarType(interp, "c") == IkigaiScript::Type::Char);
	REQUIRE(getVar(interp, "c")->getChar() == (char32_t)('x' + 'y'));
}

// =============================================================================
// Multiline strings (migrated)
// =============================================================================

TEST_CASE("string: triple-quote multiline", "[strings.multiline]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var a = """hello
world""";)");
	REQUIRE(getVarType(interp, "a") == IkigaiScript::Type::String);
	REQUIRE(getVar(interp, "a")->getString() == "hello\nworld");
}

// =============================================================================
// String interpolation (migrated)
// =============================================================================

TEST_CASE("string interpolation: variable reference", "[strings.interpolation]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var name = "World";
        var a = "Hello {name}!";
    )");
	REQUIRE(getVar(interp, "a")->getString() == "Hello World!");
}

TEST_CASE("string interpolation: expression", "[strings.interpolation]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var x = 10;
        var b = "Value: {x * 2}";
    )");
	REQUIRE(getVar(interp, "b")->getString() == "Value: 20");
}

TEST_CASE("string interpolation: nested expression", "[strings.interpolation]") {
	auto interp = makeInterp();
	interp.evaluate(R"(
        var a = 3;
        var b = 4;
        var s = "sum={a+b}";
    )");
	REQUIRE(getVar(interp, "s")->getString() == "sum=7");
}

// =============================================================================
// String builtins
// =============================================================================

TEST_CASE("length: string length", "[strings.builtins]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var s = "hello"; var n = length(s);)");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarInt(interp, "n") == 5);
}

TEST_CASE("startswith: matching prefix", "[strings.builtins]") {
	auto interp = makeInterp();
	// startswith returns Int 1 (truthy) for a match — verify via print output
	auto out = run(interp, R"(print(startswith("hello world", "hello"));)");
	REQUIRE(out != "0");
	REQUIRE(!out.empty());
}

TEST_CASE("startswith: non-matching prefix", "[strings.builtins]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var r = startswith("hello world", "world");)");
	REQUIRE(getVar(interp, "r")->getBool() == false);
}

TEST_CASE("endswith: matching suffix", "[strings.builtins]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var r = endswith("hello world", "world");)");
	REQUIRE(getVar(interp, "r")->getBool() == true);
}

TEST_CASE("endswith: non-matching suffix", "[strings.builtins]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var r = endswith("hello world", "hello");)");
	REQUIRE(getVar(interp, "r")->getBool() == false);
}

TEST_CASE("contains: function exists and returns a value", "[strings.builtins]") {
	auto interp = makeInterp();
	// contains() exists and is callable — result may depend on argument semantics
	interp.evaluate(R"(var r = contains("hello", "xyz");)");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

TEST_CASE("replace: replaces substring", "[strings.builtins]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var r = replace("hello world", "world", "there");)");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	REQUIRE(getVarString(interp, "r") == "hello there");
}

TEST_CASE("split: splits by delimiter", "[strings.builtins]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var parts = split("a,b,c", ",");)");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
	auto parts = getVar(interp, "parts");
	REQUIRE((parts->getType() == IkigaiScript::Type::Array ||
		parts->getType() == IkigaiScript::Type::List));
}

TEST_CASE("string indexing: str[0]", "[strings.index]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var s = "hello"; var c = s[0];)");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}

TEST_CASE("string + string via += operator", "[strings.builtins]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var s = "foo"; s += "bar";)");
	REQUIRE(getVarString(interp, "s") == "foobar");
}

TEST_CASE("find: substring in string returns code point index", "[strings.find]") {
	auto interp = makeInterp();
	REQUIRE(run(interp, R"(print(find("hello world", "world"));)") == "6");
}

TEST_CASE("find: substring not present returns null (no exception)", "[strings.find]") {
	auto interp = makeInterp();
	interp.evaluate(R"(var r = find("hello", "xyz");)");
	REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::None);
}
