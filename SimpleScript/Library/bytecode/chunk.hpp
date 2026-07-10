#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include "../types.hpp"
#include "opcodes.hpp"

namespace IkigaiScript {

	// A compiled function body.  Lives in a shared_ptr so it survives arena resets
	// that would otherwise reclaim the AST nodes.
	struct BytecodeFunction {
		std::string name;
		std::vector<uint8_t> code;       // opcodes + inline operands
		std::vector<ValuePtr> constants; // constant pool: literals + FunctionRefs
		std::vector<std::string> names;  // name pool: variable / member names
		std::vector<int> lines;          // source line per opcode (for error msgs)
		// Blueprint node IDs per opcode — 0 means "not from a blueprint node".
		// Parallel to `lines`; used by ExecutionObserver for node highlighting.
		std::vector<int> bpNodeIds;
		uint16_t localCount = 0;       // number of local variable stack slots
		uint16_t upvalueCount = 0;       // number of captured upvalues
		bool isCoro = false;             // mirrors Function::isCoro
		// Parameter names — stored so the function can be re-registered after deserialization.
		std::vector<std::string> argNames;

		// --- Emit helpers ---

		void emitByte(uint8_t b, int line = 0, int bpNodeId = 0) {
			code.push_back(b);
			lines.push_back(line);
			bpNodeIds.push_back(bpNodeId);
		}

		void emit(OpCode op, int line = 0, int bpNodeId = 0) {
			emitByte(static_cast<uint8_t>(op), line, bpNodeId);
		}

		// Emit opcode + 2-byte little-endian operand.
		void emitU16(OpCode op, uint16_t operand, int line = 0, int bpNodeId = 0) {
			emit(op, line, bpNodeId);
			emitByte(static_cast<uint8_t>(operand & 0xFF), line, bpNodeId);
			emitByte(static_cast<uint8_t>((operand >> 8) & 0xFF), line, bpNodeId);
		}

		// Emit opcode + 1-byte operand.
		void emitU8(OpCode op, uint8_t operand, int line = 0, int bpNodeId = 0) {
			emit(op, line, bpNodeId);
			emitByte(operand, line, bpNodeId);
		}

		// Emit a 2-byte placeholder jump; returns the offset to patch later.
		size_t emitJump(OpCode op, int line = 0, int bpNodeId = 0) {
			emit(op, line, bpNodeId);
			emitByte(0xFF, line, bpNodeId);
			emitByte(0xFF, line, bpNodeId);
			return code.size() - 2; // position of the low byte
		}

		// Patch a previously emitted jump to jump to the current position.
		void patchJump(size_t jumpOffset) {
			int32_t target = static_cast<int32_t>(code.size());
			int32_t from = static_cast<int32_t>(jumpOffset + 2); // after operand bytes
			int32_t offset = target - from;
			// Stored as signed 16-bit offset (relative to instruction end).
			code[jumpOffset] = static_cast<uint8_t>(offset & 0xFF);
			code[jumpOffset + 1] = static_cast<uint8_t>((offset >> 8) & 0xFF);
		}

		// Emit a backward loop jump.
		void emitLoop(size_t loopStart, OpCode op, int line = 0, int bpNodeId = 0) {
			emit(op, line, bpNodeId);
			int32_t offset = static_cast<int32_t>(code.size() + 2) - static_cast<int32_t>(loopStart);
			emitByte(static_cast<uint8_t>((-offset) & 0xFF), line, bpNodeId);
			emitByte(static_cast<uint8_t>(((-offset) >> 8) & 0xFF), line, bpNodeId);
		}

		// Add a constant to the pool and return its index.
		uint16_t addConstant(ValuePtr val) {
			constants.push_back(val);
			return static_cast<uint16_t>(constants.size() - 1);
		}

		// Add a name to the name pool (deduplication).
		uint16_t addName(const std::string& name_) {
			for (size_t i = 0; i < names.size(); ++i) {
				if (names[i] == name_) return static_cast<uint16_t>(i);
			}
			names.push_back(name_);
			return static_cast<uint16_t>(names.size() - 1);
		}

		// Current bytecode size — used as a loop head offset.
		size_t currentOffset() const {
			return code.size();
		}

		// Read a uint16 from code at position pos (little-endian).
		static uint16_t readU16(const std::vector<uint8_t>& code, size_t pos) {
			return static_cast<uint16_t>(code[pos] | (code[pos + 1] << 8));
		}

		// Read a signed int16 from code at position pos.
		static int16_t readI16(const std::vector<uint8_t>& code, size_t pos) {
			return static_cast<int16_t>(readU16(code, pos));
		}
	};

	using BytecodeFunctionRef = std::shared_ptr<BytecodeFunction>;

	// A compiled module / top-level script.
	struct Chunk {
		// Top-level module initialiser (statements executed at script load).
		BytecodeFunctionRef main;

		// Named function bodies compiled from the same script.
		std::unordered_map<std::string, BytecodeFunctionRef> functions;

		Chunk(): main(std::make_shared<BytecodeFunction>()) {
			main->name = "__main__";
		}
	};

	using ChunkRef = std::shared_ptr<Chunk>;

} // namespace IkigaiScript
