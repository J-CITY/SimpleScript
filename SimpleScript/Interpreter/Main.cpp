#include <iostream>
#include <string>
#include "../Library/ikigaiScript.h"

IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);

void interpret() {
	std::string s;
	while (getline(std::cin, s) && s != "q") {
		interp.readLine(s);
	}
}

int main(int argc, char** argv) {
	if (argc == 1) {
		std::cout << "SimpleScript Interpreter:\n";
		interpret();
	}
	else if (argc == 2) {
		return interp.evaluateFile(std::string(argv[1]));
	}
	else {
		std::cout << "Usage: \n\tSimpleScript -> Starts Interpreter\n\tSimpleScript [filepath] -> Execute Script File\n";
	}
	return 0;
}
