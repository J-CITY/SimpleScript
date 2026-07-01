#include <iostream>
#include <string>
#include <memory>
#include "f:/projects/SimpleScript/SimpleScript/Library/ikigaiScript.hpp"
using namespace IkigaiScript;
int main() {
    IkigaiScriptInterpreter interp(ModulePrivilege::allPrivilege);
    interp.evaluate("var a = if (true) { 42; } else { 10; };");
    auto a = interp.getGlobalScope()->variables["a"];
    std::cout << "a type: " << (int)a->getType() << std::endl;
    if (a->getType() == Type::Int) {
        std::cout << "a int: " << a->getInt() << std::endl;
    } else if (a->getType() == Type::Float) {
        std::cout << "a float: " << a->getFloat() << std::endl;
    } else if (a->getType() == Type::Null) {
        std::cout << "a is null" << std::endl;
    } else {
        std::cout << "a other" << std::endl;
    }
    return 0;
}
