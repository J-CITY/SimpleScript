#include "TestHelpers.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using namespace TestHelpers;
using namespace std::string_literals;

class TempFile {
	fs::path p;
public:
	TempFile(const std::string& filename, const std::string& content) {
		p = fs::temp_directory_path() / filename;
		std::ofstream out(p);
		out << content;
	}
	std::string path() const {
		return p.generic_string();
	}
	~TempFile() {
		std::error_code ec;
		fs::remove(p, ec);
	}
};

TEST_CASE("modules: qualified variable and function access", "[modules.qualified]") {
	TempFile f("math_mod.ss",
		"module Math;\n"
		"export fun add(a, b) { return a + b; }\n"
		"export var PI = 3.14;\n"
		"fun helper() { return 100; }\n"
	);

	auto interp = makeInterp();
	interp.evaluate("import \"" + f.path() + "\";");

	// Проверяем qualified call
	REQUIRE(run(interp, "print(Math.add(2, 3));") == "5");

	// Проверяем qualified variable
	REQUIRE(run(interp, "print(Math.PI);") == "3.140000");

	// Проверяем, что helper не экспортирован (должно выбросить исключение)
	REQUIRE(hadException(interp, "print(Math.helper());", ExceptionType::Unknown));
}

TEST_CASE("modules: selective import using using", "[modules.using.selective]") {
	TempFile f("utils_mod.ss",
		"module Utils;\n"
		"export fun mult(a, b) { return a * b; }\n"
		"export var greet = \"hello\";\n"
	);

	auto interp = makeInterp();
	interp.evaluate("import \"" + f.path() + "\";");

	// Selective import
	interp.evaluate("using { mult, greet } from Utils;");

	REQUIRE(run(interp, "print(mult(3, 4));") == "12");
	REQUIRE(run(interp, "print(greet);") == "hello");
}

TEST_CASE("modules: selective import with alias", "[modules.using.alias]") {
	TempFile f("alias_mod.ss",
		"module Foo;\n"
		"export fun bar() { return 42; }\n"
	);

	auto interp = makeInterp();
	interp.evaluate("import \"" + f.path() + "\";");

	// Selective import with alias
	interp.evaluate("using { bar as myBar } from Foo;");

	REQUIRE(run(interp, "print(myBar());") == "42");
}

TEST_CASE("modules: single using alias", "[modules.using.single_alias]") {
	TempFile f("single_mod.ss",
		"module Bar;\n"
		"export var val = 100;\n"
	);

	auto interp = makeInterp();
	interp.evaluate("import \"" + f.path() + "\";");

	// Single alias
	interp.evaluate("using myVal = Bar.val;");

	REQUIRE(run(interp, "print(myVal);") == "100");
}

TEST_CASE("modules: circular import check", "[modules.circular]") {
	auto pathA = (fs::temp_directory_path() / "circA.ss").generic_string();
	auto pathB = (fs::temp_directory_path() / "circB.ss").generic_string();

	std::ofstream outA(pathA);
	outA << "import \"" << pathB << "\";\nmodule CircA;\nexport var a = 1;\n";
	outA.close();

	std::ofstream outB(pathB);
	outB << "import \"" << pathA << "\";\nmodule CircB;\nexport var b = 2;\n";
	outB.close();

	auto interp = makeInterp();
	REQUIRE(hadException(interp, "import \"" + pathA + "\";", ExceptionType::Unknown));

	std::error_code ec;
	fs::remove(pathA, ec);
	fs::remove(pathB, ec);
}

TEST_CASE("modules: caching check", "[modules.caching]") {
	TempFile f("count_mod.ss",
		"module Count;\n"
		"export var value = 0;\n"
		"value = value + 1;\n"
	);

	auto interp = makeInterp();
	interp.evaluate("import \"" + f.path() + "\";");
	REQUIRE(run(interp, "print(Count.value);") == "1");

	interp.evaluate("import \"" + f.path() + "\";");
	REQUIRE(run(interp, "print(Count.value);") == "1");
}

TEST_CASE("modules: unknown module error", "[modules.unknown]") {
	auto interp = makeInterp();
	REQUIRE(hadException(interp, "import NonExistentModule;", ExceptionType::Unknown));
}

TEST_CASE("modules: built-in file module permission check", "[modules.privilege]") {
	auto interpPass = makeInterp(ModulePrivilege::allPrivilege);
	REQUIRE(noException(interpPass, "import file;"));

	auto interpFail = makeInterp(ModulePrivilege::unrestricted);
	REQUIRE(hadException(interpFail, "import file;", ExceptionType::Unknown));
}
