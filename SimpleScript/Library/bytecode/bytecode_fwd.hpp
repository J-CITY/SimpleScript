#pragma once
#include <memory>

namespace IkigaiScript {
    // Forward declaration — full definition is in bytecode/chunk.hpp.
    struct BytecodeFunction;
    using BytecodeFunctionRef = std::shared_ptr<BytecodeFunction>;
}
