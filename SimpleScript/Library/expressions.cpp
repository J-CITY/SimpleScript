#include "expressions.hpp"

#include "ikigaiScript.h"

using namespace IkigaiScript;

LineInfo::LineInfo() {
	line = IkigaiScriptInterpreter::currentLine;
}
