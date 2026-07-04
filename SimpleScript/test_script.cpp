#include "../ikigaiScript.h"
#include <iostream>

using namespace IkigaiScript;

int main() {
    IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
    interp.__DEBUG_OUT__ = "";
    std::string code = R"(
        for (i = 0; i < 3; i++) {
            print(i);
        }
    )";
    try {
        interp.evaluate(code);
        std::cout << "OUTPUT: " << interp.__DEBUG_OUT__ << std::endl;
    } catch (std::exception& e) {
        std::cout << "EXC: " << e.what() << std::endl;
    }
    return 0;
}
