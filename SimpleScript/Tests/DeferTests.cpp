#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("defer: basic LIFO execution order", "[defer.lifo]") {
	auto interp = makeInterp();
	interp.evaluate(
		"fun testLIFO() {\n"
		"    defer { print(\"1\"); }\n"
		"    defer { print(\"2\"); }\n"
		"    print(\"start\");\n"
		"}\n"
	);
	REQUIRE(run(interp, "testLIFO();") == "start21");
}

TEST_CASE("defer: variables capture at execution time", "[defer.capture]") {
	auto interp = makeInterp();
	interp.evaluate(
		"fun testCapture() {\n"
		"    var x = 10;\n"
		"    defer { print(x); }\n"
		"    x = 20;\n"
		"}\n"
	);
	REQUIRE(run(interp, "testCapture();") == "20");
}

TEST_CASE("defer: run before return", "[defer.return]") {
	auto interp = makeInterp();
	interp.evaluate(
		"fun testReturn() {\n"
		"    var log = \"\";\n"
		"    defer { log = log + \"A\"; print(log); }\n"
		"    log = \"1\";\n"
		"    return log;\n"
		"}\n"
	);
	REQUIRE(run(interp, "print(testReturn());") == "1A1");
}

TEST_CASE("defer: block scope isolation", "[defer.block]") {
	auto interp = makeInterp();
	interp.evaluate(
		"fun testBlock() {\n"
		"    var log = \"\";\n"
		"    defer { log = log + \"A\"; print(log); }\n"
		"    {\n"
		"        defer { log = log + \"B\"; print(log); }\n"
		"        log = log + \"1\";\n"
		"    }\n"
		"    log = log + \"2\";\n"
		"}\n"
	);
	REQUIRE(run(interp, "testBlock();") == "1B1B2A");
}

TEST_CASE("defer: watermark inside while loop", "[defer.loop]") {
	auto interp = makeInterp();
	interp.evaluate(
		"fun testLoop() {\n"
		"    var i = 0;\n"
		"    while (i < 3) {\n"
		"        defer { print(i); }\n"
		"        i = i + 1;\n"
		"    }\n"
		"}\n"
	);
	REQUIRE(run(interp, "testLoop();") == "123");
}

TEST_CASE("defer: exception guard triggers defer", "[defer.exception]") {
	auto interp = makeInterp();
	interp.evaluate(
		"fun testException() {\n"
		"    defer { print(\"cleanup\"); }\n"
		"    var error = \"hello\" - \"world\";\n"
		"}\n"
	);
	run(interp, "testException();");
	REQUIRE(interp.__DEBUG_OUT__ == "cleanup");
	REQUIRE(interp.__EXEPTION__ != ExceptionType::None);
}

TEST_CASE("defer: forbidden control flow transfers in defer body", "[defer.restrictions]") {
	auto interp = makeInterp();
	interp.evaluate(
		"fun testReturnRestriction() {\n"
		"    defer { return 1; }\n"
		"}\n"
	);
	REQUIRE(hadException(interp, "testReturnRestriction();", ExceptionType::Unknown));

	auto interp2 = makeInterp();
	interp2.evaluate(
		"fun testBreakRestriction() {\n"
		"    var i = 0;\n"
		"    while (i < 3) {\n"
		"        defer { break; }\n"
		"        i = i + 1;\n"
		"    }\n"
		"}\n"
	);
	REQUIRE(hadException(interp2, "testBreakRestriction();", ExceptionType::Unknown));
}
