#pragma once
#include <memory>

namespace IkigaiScript {
	// Forward declarations — full definitions are in bytecode/chunk.hpp.
	struct BytecodeFunction;
	using BytecodeFunctionRef = std::shared_ptr<BytecodeFunction>;

	struct Chunk;
	using ChunkRef = std::shared_ptr<Chunk>;
}
