#include "Library/ikigaiScript.h"
#include "Library/value.hpp"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace std::string_literals;

TEST_CASE("OPERATORS - Test operator+ 0", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 + 2;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "4");
}

TEST_CASE("OPERATORS - Test operator+ 1", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = -2 + 2;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0");
}

TEST_CASE("OPERATORS - Test operator+ 2", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 + -2;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0");
}

TEST_CASE("OPERATORS - Test operator+ 3", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 + (-2);
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0");
}

TEST_CASE("OPERATORS - Test operator+ 4", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = (2 + (-2));
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0");
}

TEST_CASE("OPERATORS - Test operator+ 5", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = (2 + -2);
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0");
}

TEST_CASE("OPERATORS - Test operator+ 6", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2.0 + -2.0;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  if (interp.__DEBUG_OUT__.empty())
    interp.__DEBUG_OUT__ = "0.0";
  REQUIRE(std::fabs(std::stof(interp.__DEBUG_OUT__) - (0.0)) < 0.000001f);
}

TEST_CASE("OPERATORS - Test operator+ 7", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = "a" + "b";
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "ab");
}

TEST_CASE("OPERATORS - Test operator+ 8", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = true + false;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "true");
}

TEST_CASE("OPERATORS - Test operator+ 9", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = false + false;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "false");
}

TEST_CASE("OPERATORS - Test operator- 10", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 - 2;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0");
}

TEST_CASE("OPERATORS - Test operator- 11", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2.0 - 2.0;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  if (interp.__DEBUG_OUT__.empty())
    interp.__DEBUG_OUT__ = "0.0";
  REQUIRE(std::fabs(std::stof(interp.__DEBUG_OUT__) - (0.0)) < 0.000001f);
}

TEST_CASE("OPERATORS - Test operator- 12", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 - +2.0;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  if (interp.__DEBUG_OUT__.empty())
    interp.__DEBUG_OUT__ = "0.0";
  REQUIRE(std::fabs(std::stof(interp.__DEBUG_OUT__) - (0.0)) < 0.000001f);
}

TEST_CASE("OPERATORS - Test operator* 13", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 * 2.0;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  if (interp.__DEBUG_OUT__.empty())
    interp.__DEBUG_OUT__ = "0.0";
  REQUIRE(std::fabs(std::stof(interp.__DEBUG_OUT__) - (4.0)) < 0.000001f);
}

TEST_CASE("OPERATORS - Test operator* 14", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 * -2.0;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  if (interp.__DEBUG_OUT__.empty())
    interp.__DEBUG_OUT__ = "0.0";
  REQUIRE(std::fabs(std::stof(interp.__DEBUG_OUT__) - (-4.0)) < 0.000001f);
}

TEST_CASE("OPERATORS - Test operator/ 15", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 / -2.0;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  if (interp.__DEBUG_OUT__.empty())
    interp.__DEBUG_OUT__ = "0.0";
  REQUIRE(std::fabs(std::stof(interp.__DEBUG_OUT__) - (-1.0)) < 0.000001f);
}

TEST_CASE("OPERATORS - Test operator% 16", "[OPERATORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 3 % 2;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "1");
}

TEST_CASE("EXPRESSIONS - Test expr 17", "[EXPRESSIONS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 * 2 + 2;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "6");
}

TEST_CASE("EXPRESSIONS - Test expr 18", "[EXPRESSIONS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2 * (2 + 2);
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "8");
}

TEST_CASE("EXPRESSIONS - Test expr 19", "[EXPRESSIONS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var b = 2;
			var c = 3 + b;
			var a = 2 * b + c;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "9");
}

TEST_CASE("EXPRESSIONS - Test expr 20", "[EXPRESSIONS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 1/2;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0");
}

TEST_CASE("EXPRESSIONS - Test expr 21", "[EXPRESSIONS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var b = 2;
			var c = 3 + b;
			var a = (((2 * (b + c) * 1.0) + 1) - 1.0) / (1.0/2);
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  if (interp.__DEBUG_OUT__.empty())
    interp.__DEBUG_OUT__ = "0.0";
  REQUIRE(std::fabs(std::stof(interp.__DEBUG_OUT__) - (28.0)) < 0.000001f);
}

TEST_CASE("EXPRESSIONS - Test expr 22", "[EXPRESSIONS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 1;
			a++;
			++a;
			a--;
			--a;
			a+=1;
			a-=1;
			print(a + 2 - 3);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0");
}

TEST_CASE("If statement - If0 23", "[If statement]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			if (1 < 2) {
				print(1);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "1");
}

TEST_CASE("If statement - If1 24", "[If statement]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			if (1 > 2) {
				print(1);
			}
			else {
				print(2);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "2");
}

TEST_CASE("If statement - If1 25", "[If statement]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2;
			if (a < a * a) {
				print(1);
			}
			else {
				print(2);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "1");
}

TEST_CASE("If statement - If1 26", "[If statement]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2;
			if (a > 1 && a % 2 == 0) {
				print(1);
			}
			else {
				print(2);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "1");
}

TEST_CASE("If statement - If1 27", "[If statement]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2;
			if (a > 1 && a % 2 == 1) {
				print(1);
			}
			else {
				print(2);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "2");
}

TEST_CASE("If statement - If1 28", "[If statement]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2;
			if (a > 1 || a % 2 == 0) {
				print(1);
			}
			else {
				print(2);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "1");
}

TEST_CASE("If statement - If1 29", "[If statement]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a = 2;
			if (a > 2 || a % 2 == 0) {
				print(1);
			}
			else {
				print(2);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "1");
}

TEST_CASE("If statement - If1 30", "[If statement]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			if (1 < 2) {
				print(1);
			}

			if (1 > 2) {
				print(2);
			}
			else {
				print(3);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "13");
}

TEST_CASE("Types - Types 31", "[Types]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a: Float = 1;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  if (interp.__DEBUG_OUT__.empty())
    interp.__DEBUG_OUT__ = "0.0";
  REQUIRE(std::fabs(std::stof(interp.__DEBUG_OUT__) - (1.0)) < 0.000001f);
}

TEST_CASE("ERRORS - Error 32", "[ERRORS]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var a: Int = 1.0;
			print(a);
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__EXEPTION__ == IkigaiScript::ExceptionType::TypeConvert);
}

TEST_CASE("FOR - For 1 33", "[FOR]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			for (i = 0; i < 10; i++) {
				print(i);
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0123456789");
}

TEST_CASE("FOR - For 1 34", "[FOR]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			for (i = 0; i < 10;) {
				print(i);
				i++;
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0123456789");
}

TEST_CASE("FOR - For 1 35", "[FOR]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var i = 0; 
			for (i < 10) {
				print(i);
				i++;
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0123456789");
}

TEST_CASE("FOR - For 1 36", "[FOR]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var i = 0;
			for (;i < 10;) {
				print(i);
				i++;
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0123456789");
}

TEST_CASE("FOR - For 5 37", "[FOR]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			for (i = 0; ; i++) {
				print(i);
				if (i >= 10) {
					break;
				}
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "012345678910");
}

TEST_CASE("FOR - For 1 38", "[FOR]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var i = 0;
			for (;;) {
				print(i);
				i++;
				if (i < 10) {
					continue;
				}
				else {
					break;
				}
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0123456789");
}

TEST_CASE("FOR - For 1 39", "[FOR]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var i = 0;
			for () {
				print(i);
				i++;
				if (i < 10) {
					continue;
				}
				else {
					break;
				}
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "0123456789");
}

TEST_CASE("WHILE - While 1 40", "[WHILE]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var i = 10;
			while (i > 0) {
				print(i);
				--i;
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "10987654321");
}

TEST_CASE("WHILE - While 2 41", "[WHILE]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var i = 10;
			while (i > 0) {
				if (i % 2 == 1) {
					print(i);
				}
				--i;
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "97531");
}

TEST_CASE("WHILE - While 3 42", "[WHILE]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var i = 10;
			var j = 5;
			while (i > 0 || j > 0) {
				print(i);
				--i;
				j--;
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "10987654321");
}

TEST_CASE("WHILE - While 3 43", "[WHILE]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var i = 10;
			var j = 5;
			while (i > 0 && (j > 0)) {
				print(i);
				--i;
				j--;
			}
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "109876");
}

TEST_CASE("WHILE - Lambda 44", "[WHILE]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			var l = fun(a, b) {
				return a + b;
			};

			print(l(13, 2));
			)";
  interp.__DEBUG_OUT__ = "";
  interp.evaluate(code);
  REQUIRE(interp.__DEBUG_OUT__ == "15");
}

TEST_CASE("WHILE - Func 45", "[WHILE]") {
  IkigaiScript::IkigaiScriptInterpreter interp(
      IkigaiScript::ModulePrivilege::allPrivilege);
  std::string code = R"(
			coro f(a, b) {
				yeld a + b + 2;
				yeld a + b + 4;
				return a + b;
			};
			
			var cr = f(13, 2);
			print(cr());
			print(cr());
			print(cr());
			)";
}

TEST_CASE("DECORATOR - Var", "[DECORATOR]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		@test(min=0, max=100, widget="slider")
		var a = 50;
	)";
	std::vector<IkigaiScript::Metadata> meta;
	try {
		interp.evaluate(code);
		meta = interp.getMetadata(interp.getGlobalScope(), "a");
		REQUIRE(meta.size() == 1);
		REQUIRE(meta[0].name == "test");
	} catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		REQUIRE(false);
	}
	REQUIRE(meta[0].arguments.size() == 3);
	REQUIRE(meta[0].arguments[0].first == "min");
	REQUIRE(meta[0].arguments[0].second->value.asFloat == 0.0);
	REQUIRE(meta[0].arguments[1].first == "max");
	REQUIRE(meta[0].arguments[1].second->value.asFloat == 100.0);
	REQUIRE(meta[0].arguments[2].first == "widget");
	REQUIRE(meta[0].arguments[2].second->value.asString == "slider");
}

TEST_CASE("DECORATOR - Class", "[DECORATOR]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		@editable
		class MyClass {
			@test(range=0)
			var a = 50;
		}
	)";
	interp.evaluate(code);
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

TEST_CASE("DECORATOR - Func", "[DECORATOR]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		@test(min=0, max=100)
		fun a() {
			return 50;
		}
	)";
	interp.evaluate(code);
	auto funcMeta = interp.getMetadata(interp.getGlobalScope(), "a");
	REQUIRE(funcMeta.size() == 1);
	REQUIRE(funcMeta[0].name == "test");
	REQUIRE(funcMeta[0].arguments.size() == 2);
	REQUIRE(funcMeta[0].arguments[0].first == "min");
	REQUIRE(funcMeta[0].arguments[0].second->value.asFloat == 0.0);
	REQUIRE(funcMeta[0].arguments[1].first == "max");
	REQUIRE(funcMeta[0].arguments[1].second->value.asFloat == 100.0);
}

TEST_CASE("STRINGS - Char Literals", "[STRINGS]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var a = 'x';
		var b = 'y';
		var c = a + b;
	)";
	interp.evaluate(code);
	auto res = interp.resolveVariable("a", interp.getGlobalScope());
	REQUIRE(res->getType() == IkigaiScript::Type::Char);
	REQUIRE(res->getChar() == 'x');
    
    auto resc = interp.resolveVariable("c", interp.getGlobalScope());
	REQUIRE(resc->getType() == IkigaiScript::Type::Char);
	REQUIRE(resc->getChar() == (char32_t)('x' + 'y'));
}

TEST_CASE("STRINGS - Multiline", "[STRINGS]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var a = """hello
world""";
	)";
	interp.evaluate(code);
	auto res = interp.resolveVariable("a", interp.getGlobalScope());
	REQUIRE(res->getType() == IkigaiScript::Type::String);
	REQUIRE(res->getString() == "hello\nworld");
}

TEST_CASE("STRINGS - Interpolation", "[STRINGS]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var name = "World";
		var a = "Hello {name}!";
        var x = 10;
        var b = "Value: {x * 2}";
	)";
	interp.evaluate(code);
	auto res = interp.resolveVariable("a", interp.getGlobalScope());
	REQUIRE(res->getType() == IkigaiScript::Type::String);
	REQUIRE(res->getString() == "Hello World!");
    
    auto resb = interp.resolveVariable("b", interp.getGlobalScope());
	REQUIRE(resb->getType() == IkigaiScript::Type::String);
	REQUIRE(resb->getString() == "Value: 20");
}

TEST_CASE("EXPRESSION - Block", "[EXPRESSION]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var a = { 
			var b = 40;
			b + 2; 
		};
	)";
	interp.evaluate(code);
	auto res = interp.getGlobalScope()->variables["a"]->getInt();
	REQUIRE(res == 42);
}

TEST_CASE("EXPRESSION - Block Nested", "[EXPRESSION]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var a = { 
			var b = { 10 + 20; };
			b + 12; 
		};
	)";
	interp.evaluate(code);
	auto res = interp.getGlobalScope()->variables["a"]->getInt();
	REQUIRE(res == 42);
}

TEST_CASE("EXPRESSION - Block with Return", "[EXPRESSION]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		fun test() {
			var a = {
				return 42;
				100;
			};
			return a;
		}
		var b = test();
	)";
	interp.evaluate(code);
	auto res = interp.getGlobalScope()->variables["b"]->getInt();
	REQUIRE(res == 42);
}

TEST_CASE("EXPRESSION - If", "[EXPRESSION]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var a = if (true) { 42; } else { 10; };
		var b = if (false) { 42; } else { 10; };
	)";
	interp.evaluate(code);
	auto a = interp.getGlobalScope()->variables["a"];
	std::cout << "DEBUG TYPE A: " << (int)a->getType() << std::endl;
	REQUIRE(a->getInt() == 42);
	auto b = interp.getGlobalScope()->variables["b"];
	std::cout << "DEBUG TYPE B: " << (int)b->getType() << std::endl;
	REQUIRE(b->getInt() == 10);
}

TEST_CASE("EXPRESSION - For Comprehension", "[EXPRESSION]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var arr = [1, 2, 3];
		var a = for (x : arr) { x * 2; };
	)";
	interp.evaluate(code);
	auto a = interp.getGlobalScope()->variables["a"];
	REQUIRE(a->getType() == IkigaiScript::Type::Array);
	auto& vec = a->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 3);
	REQUIRE(vec[0] == 2);
	REQUIRE(vec[1] == 4);
	REQUIRE(vec[2] == 6);
}

TEST_CASE("EXPRESSION - For Comprehension Multi Arr", "[EXPRESSION]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var arr = [10, 20, 30];
		var a = for (idx, x : arr) { idx + x; };
	)";
	interp.evaluate(code);
	auto a = interp.getGlobalScope()->variables["a"];
	REQUIRE(a->getType() == IkigaiScript::Type::Array);
	auto& vec = a->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 3);
	REQUIRE(vec[0] == 10);
	REQUIRE(vec[1] == 21);
	REQUIRE(vec[2] == 32);
}

TEST_CASE("EXPRESSION - For Comprehension Multi Map", "[EXPRESSION]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var map = dictionary();
		map["a"] = 100;
		map["b"] = 200;
		var a = for (key, val : map) { val * 2; };
	)";
	interp.evaluate(code);
	auto a = interp.getGlobalScope()->variables["a"];
	REQUIRE(a->getType() == IkigaiScript::Type::Array);
	auto& vec = a->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 2);
	// We can't guarantee map iteration order, so just check sum
	REQUIRE((vec[0] + vec[1]) == 600);
}

TEST_CASE("EXPRESSION - For Comprehension Multi Map 3", "[EXPRESSION]") {
	IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);
	std::string code = R"(
		var map = dictionary();
		map["a"] = 100;
		map["b"] = 200;
		var a = for (idx, key, val : map) { idx + val; };
	)";
	interp.evaluate(code);
	auto a = interp.getGlobalScope()->variables["a"];
	REQUIRE(a->getType() == IkigaiScript::Type::Array);
	auto& vec = a->getStdVector<IkigaiScript::Int>();
	REQUIRE(vec.size() == 2);
	// We can't guarantee map iteration order, so just check sum (0+100 + 1+200 = 301)
	REQUIRE((vec[0] + vec[1]) == 301);
}
