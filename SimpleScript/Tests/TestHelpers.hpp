#pragma once

#include "../Library/ikigaiScript.h"
#include "../Library/value.hpp"
#include "../Library/exception.h"
#include "catch.hpp"

#include <cmath>
#include <string>

namespace TestHelpers {

using namespace IkigaiScript;
using namespace std::string_literals;

// ---------------------------------------------------------------------------
// Interpreter factory
// ---------------------------------------------------------------------------

inline IkigaiScriptInterpreter makeInterp(
    ModulePrivilege priv = ModulePrivilege::allPrivilege)
{
    return IkigaiScriptInterpreter(priv);
}

// ---------------------------------------------------------------------------
// Script runner helpers
// ---------------------------------------------------------------------------

// Evaluate code, reset debug output and exception state first.
inline std::string run(IkigaiScriptInterpreter& interp, const std::string& code)
{
    interp.__DEBUG_OUT__ = "";
    interp.__EXEPTION__ = ExceptionType::None;
    interp.evaluate(code);
    return interp.__DEBUG_OUT__;
}

// Evaluate code in a fresh interpreter (one-shot) and return print output.
inline std::string runFresh(const std::string& code)
{
    auto interp = makeInterp();
    return run(interp, code);
}

// ---------------------------------------------------------------------------
// Exception helpers
// ---------------------------------------------------------------------------

inline bool hadException(IkigaiScriptInterpreter& interp,
                         const std::string& code,
                         ExceptionType expected = ExceptionType::TypeConvert)
{
    run(interp, code);
    return interp.__EXEPTION__ == expected;
}

inline bool noException(IkigaiScriptInterpreter& interp, const std::string& code)
{
    run(interp, code);
    return interp.__EXEPTION__ == ExceptionType::None;
}

// ---------------------------------------------------------------------------
// Float helpers
// ---------------------------------------------------------------------------

inline float parseFloat(const std::string& s)
{
    if (s.empty()) return 0.0f;
    return std::stof(s);
}

inline bool nearEqual(float a, float b, float eps = 0.000001f)
{
    return std::fabs(a - b) < eps;
}

// ---------------------------------------------------------------------------
// Variable accessors
// ---------------------------------------------------------------------------

inline ValuePtr getVar(IkigaiScriptInterpreter& interp, const std::string& name)
{
    return interp.resolveVariable(name, interp.getGlobalScope());
}

inline Int getVarInt(IkigaiScriptInterpreter& interp, const std::string& name)
{
    return getVar(interp, name)->getInt();
}

inline Float getVarFloat(IkigaiScriptInterpreter& interp, const std::string& name)
{
    return getVar(interp, name)->getFloat();
}

inline std::string getVarString(IkigaiScriptInterpreter& interp,
                                const std::string& name)
{
    return getVar(interp, name)->getString();
}

inline Type getVarType(IkigaiScriptInterpreter& interp, const std::string& name)
{
    return getVar(interp, name)->getType();
}

} // namespace TestHelpers
