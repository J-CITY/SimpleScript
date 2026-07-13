#include "TestHelpers.hpp"

using namespace TestHelpers;
using namespace std::string_literals;

TEST_CASE("result: basic ok construction & api", "[result.ok]") {
	auto interp = makeInterp();

	// Создание через resultOk
	interp.evaluate("var r = resultOk(42);");

	// Проверка методов
	REQUIRE(run(interp, "print(resultIsOk(r));") == "true");
	REQUIRE(run(interp, "print(resultGet(r));") == "42");

	// Вывод в строку
	REQUIRE(run(interp, "print(r);") == "ok(42)");
}

TEST_CASE("result: basic err construction & api", "[result.err]") {
	auto interp = makeInterp();

	// Создание через resultErr
	interp.evaluate("var r = resultErr(\"failure\");");

	// Проверка методов
	REQUIRE(run(interp, "print(resultIsOk(r));") == "false");
	REQUIRE(run(interp, "print(resultGetErr(r));") == "failure");

	// Вывод в строку
	REQUIRE(run(interp, "print(r);") == "err(failure)");
}

TEST_CASE("result: type checking & conversions", "[result.types]") {
	auto interp = makeInterp();

	// Инициализация переменной с типом
	interp.evaluate("var r : Result<Int, String> = resultOk(10);");
	REQUIRE(run(interp, "print(resultGet(r));") == "10");

	// Изменение значения на Ok с тем же типом
	interp.evaluate("r = resultOk(20);");
	REQUIRE(run(interp, "print(resultGet(r));") == "20");

	// Изменение на Err с корректным типом
	interp.evaluate("r = resultErr(\"oops\");");
	REQUIRE(run(interp, "print(resultGetErr(r));") == "oops");

	// Ошибочное присвоение Ok с неверным типом (String вместо Int)
	bool typeErrorOk = interp.evaluate("r = resultOk(\"bad type\");");
	REQUIRE(typeErrorOk == true); // Ожидаем исключение при присваивании несовместимого типа

	// Ошибочное присвоение Err с неверным типом (Int вместо String)
	bool typeErrorErr = interp.evaluate("r = resultErr(123);");
	REQUIRE(typeErrorErr == true);
}

TEST_CASE("result: function return types", "[result.functions]") {
	auto interp = makeInterp();

	interp.evaluate(
		"fun divide(a: Float, b: Float): Result<Float, String> {\n"
		"    if (b == 0.0) {\n"
		"        return resultErr(\"division by zero\");\n"
		"    }\n"
		"    return resultOk(a / b);\n"
		"}\n"
	);

	// Успешный вызов
	interp.evaluate("var res1 = divide(10.0, 2.0);");
	REQUIRE(run(interp, "print(resultIsOk(res1));") == "true");
	REQUIRE(run(interp, "print(resultGet(res1));") == "5.000000");

	// Ошибочный вызов
	interp.evaluate("var res2 = divide(10.0, 0.0);");
	REQUIRE(run(interp, "print(resultIsOk(res2));") == "false");
	REQUIRE(run(interp, "print(resultGetErr(res2));") == "division by zero");
}

TEST_CASE("result: copy & assign semantics", "[result.copy]") {
	auto interp = makeInterp();

	interp.evaluate(
		"var a = resultOk(100);\n"
		"var b = a;\n"
	);
	REQUIRE(run(interp, "print(resultGet(b));") == "100");

	// В IkigaiScript переменные в скоупе разделяют один и тот же объект Value при присваивании (ссылочная семантика).
	// Поэтому изменение 'a' через '=' обновит и значение 'b'.
	interp.evaluate("a = resultOk(200);");
	REQUIRE(run(interp, "print(resultGet(a));") == "200");
	REQUIRE(run(interp, "print(resultGet(b));") == "200");
}

TEST_CASE("result: error throws on wrong api", "[result.exceptions]") {
	auto interp = makeInterp();

	interp.evaluate("var okVal = resultOk(1);");
	interp.evaluate("var errVal = resultErr(\"err\");");

	// Вызов resultGet на Err
	bool getErr = interp.evaluate("resultGet(errVal);");
	REQUIRE(getErr == true); // Ожидаем исключение

	// Вызов resultGetErr на Ok
	bool getOkErr = interp.evaluate("resultGetErr(okVal);");
	REQUIRE(getOkErr == true); // Ожидаем исключение
}
