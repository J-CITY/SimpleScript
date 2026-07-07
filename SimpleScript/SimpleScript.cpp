#include "Library/ikigaiScript.h"

IkigaiScript::IkigaiScriptInterpreter interp(IkigaiScript::ModulePrivilege::allPrivilege);

int main(int argc, char** argv) {
	interp.evaluate(
        "var x = 0;\n"
        "if (true) {\n"
        "    // x = 10;\n"
        "    x = 20; // set x\n"
        "}\n"
    );
	return 0;
}
