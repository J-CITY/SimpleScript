#pragma once
#include "chunk.hpp"
#include <string>
#include <string_view>

namespace IkigaiScript {

	// ---------------------------------------------------------------------------
	// IKBC v1 binary serializer / deserializer
	//
	// Wire format (little-endian throughout):
	//
	//   Header   : magic[4]="IKBC" + version=uint32(1)
	//   Chunk    : named_fn_count:uint32
	//              repeat named_fn_count { FnEntry }
	//              main:BFPayload
	//
	//   FnEntry  : name:String + arg_count:uint16 + arg_names[] + is_coro:uint8
	//              + bfn:BFPayload
	//
	//   BFPayload: localCount:uint16 + upvalueCount:uint16 + is_coro:uint8
	//              + code_len:uint32 + code[]
	//              + lines_len:uint32 + int32[]
	//              + names_count:uint32 + name_strings[]
	//              + const_count:uint32 + ConstEntry[]
	//
	//   ConstEntry tag:uint8 then payload:
	//     0 Null
	//     1 Bool   : value:uint8
	//     2 Int    : value:int64
	//     3 Float  : value:double
	//     4 String : String
	//     5 TypeDescriptor : type:uint8 + flags:uint8 (isInit|isNullable|isConst|isDynamic)
	//     6 FunctionEmbedded: fn_name:String + arg_count:uint16 + arg_names[] + is_coro:uint8
	//                         + BFPayload (recursive)
	// ---------------------------------------------------------------------------

	class IKBCSerializer {
	public:
		// Write chunk to a binary blob (returned as std::string).
		static std::string serialize(const Chunk& chunk);

		// Read chunk from binary blob.  Throws std::runtime_error on bad magic/version.
		static ChunkRef deserialize(std::string_view data);

	private:
		// --- Write helpers ---
		static void writeU8(std::string& buf, uint8_t  v);
		static void writeU16(std::string& buf, uint16_t v);
		static void writeU32(std::string& buf, uint32_t v);
		static void writeI32(std::string& buf, int32_t  v);
		static void writeI64(std::string& buf, int64_t  v);
		static void writeF64(std::string& buf, double   v);
		static void writeString(std::string& buf, const std::string& s);

		static void writeBFPayload(std::string& buf, const BytecodeFunction& bfn);
		static void writeConstEntry(std::string& buf, const ValuePtr& v);
		static void writeFnEntry(std::string& buf, const std::string& name,
			const BytecodeFunction& bfn);

		// --- Read helpers ---
		static uint8_t  readU8(std::string_view data, size_t& pos);
		static uint16_t readU16(std::string_view data, size_t& pos);
		static uint32_t readU32(std::string_view data, size_t& pos);
		static int32_t  readI32(std::string_view data, size_t& pos);
		static int64_t  readI64(std::string_view data, size_t& pos);
		static double   readF64(std::string_view data, size_t& pos);
		static std::string readString(std::string_view data, size_t& pos);

		static BytecodeFunctionRef readBFPayload(std::string_view data, size_t& pos);
		static ValuePtr            readConstEntry(std::string_view data, size_t& pos);
	};

} // namespace IkigaiScript
