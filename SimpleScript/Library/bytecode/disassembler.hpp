#pragma once
#include <string>
#include "chunk.hpp"

namespace IkigaiScript {

// Converts a BytecodeFunction to a human-readable listing.
// The result can be appended to __DEBUG_OUT__ when SCRIPT_DO_INTERNAL_PRINT is defined.
std::string disassemble(const BytecodeFunction& fn);
std::string disassemble(const Chunk& chunk);

} // namespace IkigaiScript
