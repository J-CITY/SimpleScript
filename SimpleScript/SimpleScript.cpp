#define KATASCRIPT_IMPL
#define IMGUI_DEFINE_MATH_OPERATORS
#include <iostream>
#include <stack>

#include "VisualEditor/VisualCodeEditor.h"
#include "imgui/imgui_internal.h"
#include "Library/ikigaiScript.h"


IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);

//TODO:
// +lambda
// +coro
// +default args in functions
// +переменное число аргументов функции
// +static types
// +operators overriding
// +const
// +null types int? float? etc
// -generic
// try catch
// +operator hash
// +operator ? :
// +operator ?.
// set


/*
 * Копировать вставить итд
 *
 * Новые ноды:
 *
 * stl
 *
 */

void interpret() {
	std::string s;
	while (getline(std::cin, s) && s != "quit") {
		interp.readLine(s);
	}
}
struct Tester {
	inline static std::string code;
	inline static std::string testName;

	inline static std::list<std::function<void()>> tasks;

	static void reset() {
		testId = 1;
		code="";
		testName="";

		blocks = std::stack<Block>{};
	}

	struct Block {
		std::string name;
		int testFail = 0;
		int testCount = 0;
		int testCur = 0;
	};
	inline static std::stack<Block> blocks;

	static void pushBlock(const std::string& s) {
		blocks.push({ s, 0, 0 });

		std::string colorBegin = std::string("\033[37m");
		std::string colorEnd = "\033[0m";
		std::string blockRes = std::format("Begin: {}", s);
		auto sepCount = length - blockRes.size();
		if (sepCount <= 0) {
			sepCount = 1;
		}
		std::string separator = std::string(sepCount, '_');
		std::cout << colorBegin << blockRes << separator << colorEnd << "\n";
	}
	static void popBlock() {
		if (blocks.empty()) {
			std::cout << "Blocks is empty" << std::endl;
			return;
		}
		auto& block = blocks.top();

		std::string colorBegin = std::string(std::format("\033[3{}m", 0 == block.testFail ? 2 : (block.testCount == block.testFail ? 1 : 3)));
		std::string colorEnd = "\033[0m";
		std::string blockRes = std::format("End: {} Fail: {} ({}/{})", block.name, block.testFail, block.testCount - block.testFail, block.testCount);
		auto sepCount = length - blockRes.size();
		if (sepCount <= 0) {
			sepCount = 1;
		}
		std::string separator = std::string(sepCount, '_');
		std::cout << colorBegin << blockRes << separator << colorEnd << "\n" << std::endl;

		blocks.pop();
	}

private:
	inline static int testId = 1;
	inline static const int length = 60;

	static void check(std::string s2, bool b) {
		std::string res = (b ? "OK" : "FAIL");
		std::string colorBegin = std::string("\033[3") + (b ? "2" : "1") + "m";
		std::string colorEnd = "\033[0m";

		std::string blockRes;
		if (blocks.size()) {
			blocks.top().testCur++;
			blockRes = std::format("{}({}/{}): {}", testId, blocks.top().testCur, blocks.top().testCount, testName);
		}
		else {
			blockRes = std::format("{}: {}", testId, testName);
		}
		auto sepCount = length - blockRes.size() - res.size();
		std::string separator = std::string(sepCount > 0 ? sepCount : 1, '_');
		std::cout << colorBegin << blockRes << separator << res << colorEnd << std::endl;
		if (!b) {
			std::cout << "Expect: " << s2 << std::endl;
			std::cout << "Get: " << interp.__DEBUG_OUT__ << std::endl;
		}
		if (blocks.size()) {
			blocks.top().testFail += (int)(!b);
		}
		testId++;
	}
public:
	static void check(std::string s2) {
		interp.__DEBUG_OUT__ = "";
		interp.evaluate(code);
		bool b = (interp.__DEBUG_OUT__ == s2);
		check(s2, b);
	}

	static void checkFloat(std::string s2) {
		interp.__DEBUG_OUT__ = "";
		interp.evaluate(code);
		bool b = (std::fabs(std::stof(interp.__DEBUG_OUT__) - std::stof(s2)) < 0.00000000001f);
		check(s2, b);
	}

	static void check(float s2) {
		checkFloat(std::to_string(s2));
	}

	static void check(IkigaiScript::ExceptionType type) {
		interp.__DEBUG_OUT__ = "";
		interp.evaluate(code);
		bool b = (type == interp.__EXEPTION__);
		check(std::to_string((int)type), b);
	}

	static void run() {
		for (auto& e : tasks) {
			e();
		}
	}
	

	static void addGroupCount() {
		if (blocks.size()) {
			blocks.top().testCount++;
		}
	}
	
};

//#define BEGIN_TEST(name) Tester::testName = name;
//#define END_TEST() ;
//#define PUSH_BLOCK(name) Tester::pushBlock(name);
//#define POP_BLOCK() Tester::popBlock();

#define BEGIN_TEST_FRONT(name) Tester::addGroupCount(); Tester::tasks.push_front([]() { Tester::testName = name;
#define BEGIN_TEST(name) Tester::addGroupCount(); Tester::tasks.push_back([]() { Tester::testName = name;

#define END_TEST() });

#define PUSH_BLOCK(name) Tester::tasks.push_back([]() { Tester::pushBlock(name); });
#define POP_BLOCK() Tester::tasks.push_back([]() { Tester::popBlock(); });

#define CODE(val) Tester::code = val;
#define CHECK(val) Tester::check(val);

#define RUN_TESTS() Tester::run();

void expressionTests() {
	PUSH_BLOCK("OPERATORS")
		BEGIN_TEST("Test operator+")
		CODE(R"(
			var a = 2 + 2;
			print(a);
			)")
		CHECK("4")
		END_TEST()
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = -2 + 2;
			print(a);
			)")
			CHECK("0");
		END_TEST()
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = 2 + -2;
			print(a);
			)")
			CHECK("0");
		END_TEST()
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = 2 + (-2);
			print(a);
			)")
			CHECK("0");
		END_TEST()
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = (2 + (-2));
			print(a);
			)")
			CHECK("0");
		END_TEST()
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = (2 + -2);
			print(a);
			)")
			CHECK("0");
		END_TEST()
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = 2.0 + -2.0;
			print(a);
			)")
			CHECK(0.0);
		END_TEST()
		//Error in tokinazer
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = "a" + "b";
			print(a);
			)")
			CHECK("ab");
		END_TEST()
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = true + false;
			print(a);
			)")
			CHECK("true");
		END_TEST()
		BEGIN_TEST("Test operator+")
			CODE(R"(
			var a = false + false;
			print(a);
			)")
			CHECK("false");
		END_TEST()

		BEGIN_TEST("Test operator-")
			CODE(R"(
			var a = 2 - 2;
			print(a);
			)")
			CHECK("0");
		END_TEST()
		BEGIN_TEST("Test operator-")
			CODE(R"(
			var a = 2.0 - 2.0;
			print(a);
			)")
			CHECK(0.0f);
		END_TEST()
		BEGIN_TEST("Test operator-")
			CODE(R"(
			var a = 2 - +2.0;
			print(a);
			)")
			CHECK(0.0f);
		END_TEST()
		BEGIN_TEST("Test operator*")
			CODE(R"(
			var a = 2 * 2.0;
			print(a);
			)")
			CHECK(4.0f);
		END_TEST()
		BEGIN_TEST("Test operator*")
			CODE(R"(
			var a = 2 * -2.0;
			print(a);
			)")
			CHECK(-4.0f);
		END_TEST()
		BEGIN_TEST("Test operator/")
			CODE(R"(
			var a = 2 / -2.0;
			print(a);
			)")
			CHECK(-1.0f);
		END_TEST()
		BEGIN_TEST("Test operator%")
			CODE(R"(
			var a = 3 % 2;
			print(a);
			)")
			CHECK("1");
		END_TEST()
	POP_BLOCK()

	PUSH_BLOCK("EXPRESSIONS")
		BEGIN_TEST("Test expr")
			CODE(R"(
			var a = 2 * 2 + 2;
			print(a);
			)")
			CHECK("6");
		END_TEST()
		BEGIN_TEST("Test expr");
			CODE(R"(
			var a = 2 * (2 + 2);
			print(a);
			)")
			CHECK("8");
		END_TEST()
		BEGIN_TEST("Test expr");
			CODE(R"(
			var b = 2;
			var c = 3 + b;
			var a = 2 * b + c;
			print(a);
			)")
			CHECK("9");
		END_TEST()
		BEGIN_TEST("Test expr");
			CODE(R"(
			var a = 1/2;
			print(a);
			)")
			CHECK("0");
		END_TEST()
		BEGIN_TEST("Test expr");
			CODE(R"(
			var b = 2;
			var c = 3 + b;
			var a = (((2 * (b + c) * 1.0) + 1) - 1.0) / (1.0/2);
			print(a);
			)")
			CHECK(28.0f);
		END_TEST()
		BEGIN_TEST("Test expr");
			CODE(R"(
			var a = 1;
			a++;
			++a;
			a--;
			--a;
			a+=1;
			a-=1;
			print(a + 2 - 3);
			)")
			CHECK("0");
		END_TEST()
	POP_BLOCK()

	PUSH_BLOCK("If statement")
		BEGIN_TEST("If0");
			CODE(R"(
			if (1 < 2) {
				print(1);
			}
			)")
			CHECK("1");
		END_TEST()
		BEGIN_TEST("If1");
			CODE(R"(
			if (1 > 2) {
				print(1);
			}
			else {
				print(2);
			}
			)")
			CHECK("2");
		END_TEST()
		BEGIN_TEST("If1");
			CODE(R"(
			var a = 2;
			if (a < a * a) {
				print(1);
			}
			else {
				print(2);
			}
			)")
			CHECK("1");
		END_TEST()
		BEGIN_TEST("If1");
			CODE(R"(
			var a = 2;
			if (a > 1 && a % 2 == 0) {
				print(1);
			}
			else {
				print(2);
			}
			)")
			CHECK("1");
		END_TEST()
		BEGIN_TEST("If1");
			CODE(R"(
			var a = 2;
			if (a > 1 && a % 2 == 1) {
				print(1);
			}
			else {
				print(2);
			}
			)")
			CHECK("2");
		END_TEST()
		BEGIN_TEST("If1");
			CODE(R"(
			var a = 2;
			if (a > 1 || a % 2 == 0) {
				print(1);
			}
			else {
				print(2);
			}
			)")
			CHECK("1");
		END_TEST()
		BEGIN_TEST("If1");
			CODE(R"(
			var a = 2;
			if (a > 2 || a % 2 == 0) {
				print(1);
			}
			else {
				print(2);
			}
			)")
			CHECK("1");
		END_TEST()
		BEGIN_TEST_FRONT("If1")
			CODE(R"(
			if (1 < 2) {
				print(1);
			}

			if (1 > 2) {
				print(2);
			}
			else {
				print(3);
			}
			)")
			CHECK("13")
		END_TEST()
	POP_BLOCK()

	PUSH_BLOCK("Types")
		BEGIN_TEST("Types");
			CODE(R"(
			var a: Float = 1;
			print(a);
			)")
			CHECK(1.0f);
		END_TEST()
	POP_BLOCK()

	PUSH_BLOCK("ERRORS")
		BEGIN_TEST("Error");
			CODE(R"(
			var a: Int = 1.0;
			print(a);
			)")
			CHECK(IkigaiScript::ExceptionType::TypeConvert);
		END_TEST()
	POP_BLOCK()
	
	

	//FOR
	PUSH_BLOCK("FOR")
		BEGIN_TEST("For 1");
			CODE(R"(
			for (i = 0; i < 10; i++) {
				print(i);
			}
			)")
			CHECK("0123456789")
		END_TEST()
		BEGIN_TEST("For 1");
			CODE(R"(
			for (i = 0; i < 10;) {
				print(i);
				i++;
			}
			)")
			CHECK("0123456789")
		END_TEST()
		BEGIN_TEST("For 1");
			CODE(R"(
			var i = 0; 
			for (i < 10) {
				print(i);
				i++;
			}
			)")
			CHECK("0123456789")
		END_TEST()
		BEGIN_TEST("For 1");
			CODE(R"(
			var i = 0;
			for (;i < 10;) {
				print(i);
				i++;
			}
			)")
			CHECK("0123456789")
		END_TEST()
		BEGIN_TEST("For 5");
			CODE(R"(
			for (i = 0; ; i++) {
				print(i);
				if (i >= 10) {
					break;
				}
			}
			)")
			CHECK("012345678910")
		END_TEST()
		BEGIN_TEST("For 1");
			CODE(R"(
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
			)")
			CHECK("0123456789")
		END_TEST()
		BEGIN_TEST("For 1");
			CODE(R"(
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
			)")
			CHECK("0123456789")
		END_TEST()
	POP_BLOCK()

	//WHILE
	PUSH_BLOCK("WHILE")
		BEGIN_TEST("While 1");
			CODE(R"(
			var i = 10;
			while (i > 0) {
				print(i);
				--i;
			}
			)")
			CHECK("10987654321");
		END_TEST()
		BEGIN_TEST("While 2");
			CODE(R"(
			var i = 10;
			while (i > 0) {
				if (i % 2 == 1) {
					print(i);
				}
				--i;
			}
			)")
			CHECK("97531");
		END_TEST()
		BEGIN_TEST("While 3");
			CODE(R"(
			var i = 10;
			var j = 5;
			while (i > 0 || j > 0) {
				print(i);
				--i;
				j--;
			}
			)")
			CHECK("10987654321");
		END_TEST()
		BEGIN_TEST("While 3");
			CODE(R"(
			var i = 10;
			var j = 5;
			while (i > 0 && (j > 0)) {
				print(i);
				--i;
				j--;
			}
			)")
			CHECK("109876");
		END_TEST()

		BEGIN_TEST("Lambda");
			CODE(R"(
			var l = fun(a, b) {
				return a + b;
			};

			print(l(13, 2));
			)")
			CHECK("15");
		END_TEST()

		BEGIN_TEST("Func")
			CODE(R"(
			coro f(a, b) {
				yeld a + b + 2;
				yeld a + b + 4;
				return a + b;
			};
			
			var cr = f(13, 2);
			print(cr());
			print(cr());
			print(cr());
			)")
			CHECK("171915")
		END_TEST()
	POP_BLOCK()

	//FUNCS

	//CLASS

	RUN_TESTS()
}

int main(int argc, char** argv) {
	//if (argc == 1) {
	//	std::cout << "KataScript Interpreter:\n";
	//	interpret();
	//}
	//else if (argc == 2) {
	//	// run script from file
	//	return interp.evaluateFile(std::string(argv[1]));
	//}
	//else {
	//	std::cout << "Usage: \n\tKataScript -> Starts Interpreter\n\tKataScript [filepath] -> Execute Script File\n";
	//}
	//
	std::string code = R"(
		var a : Int = 2 + 5 * 4;

		var b = 4;

		a = b;

		var n1 = 0x01ea;
		var n2 = 0b0101010;
		var n3 = -24e-10;
		var n4 = +24e+10;
		var n5 = +4.43;
		var n6 = -4.43;
		var n7 = 4.43;
		var n8 = 4;
		var n9 = -4;
		var n10 = +4;

		print(a);
	)";

	{
		std::string code = R"(
		fun f(a, b) {
			return a + b;
		};

		fun f2(a, b) {
			return f(a, b) * 2;
		};

		var a = 0;
		var b = 0;

		a = f(a, b);
		a = f2(a, b);

	)";
	//interp.evaluate(code);

	//int n;
	//while (std::cin >> n) {
	//	interp.data_is_ready_ = true;
	//}
	}

	expressionTests();

	//auto tokens = KataScript::ViewTokenize(code);
	//for (auto& t : tokens) {
	//	std::cout << "'" << t << "'" << std::endl;
	//}
	return 0;
}

/*
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}



// TODO
// fix delete member from class
// fix class code gen with empty def values
// fix class value buttons when delete class node
// fix add class val buttons when load scene
// update type in combo box
 

int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;


	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
	if (window == nullptr)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	 // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		   // Enable Docking
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;


	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	
	auto g = Visual::VisualCodeEditor();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		g.draw();
		//OnFrame(GImGui->IO.DeltaTime);

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
*/