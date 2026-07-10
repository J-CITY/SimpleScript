#include "serializer.hpp"
#include "../value.hpp"
#include "../enums.h"
#include <cstring>
#include <stdexcept>

namespace IkigaiScript {

	static constexpr char kMagic[4] = {'I', 'K', 'B', 'C'};
	static constexpr uint32_t kVersion = 1;

	// ConstEntry tags
	static constexpr uint8_t TAG_NULL = 0;
	static constexpr uint8_t TAG_BOOL = 1;
	static constexpr uint8_t TAG_INT = 2;
	static constexpr uint8_t TAG_FLOAT = 3;
	static constexpr uint8_t TAG_STRING = 4;
	static constexpr uint8_t TAG_TYPEDESC = 5;
	static constexpr uint8_t TAG_FN_EMBEDDED = 6;

	// ---------------------------------------------------------------------------
	// Write helpers
	// ---------------------------------------------------------------------------

	void IKBCSerializer::writeU8(std::string& buf, uint8_t v) {
		buf.push_back(static_cast<char>(v));
	}

	void IKBCSerializer::writeU16(std::string& buf, uint16_t v) {
		buf.push_back(static_cast<char>(v & 0xFF));
		buf.push_back(static_cast<char>((v >> 8) & 0xFF));
	}

	void IKBCSerializer::writeU32(std::string& buf, uint32_t v) {
		for (int i = 0; i < 4; ++i) buf.push_back(static_cast<char>((v >> (8 * i)) & 0xFF));
	}

	void IKBCSerializer::writeI32(std::string& buf, int32_t v) {
		writeU32(buf, static_cast<uint32_t>(v));
	}

	void IKBCSerializer::writeI64(std::string& buf, int64_t v) {
		for (int i = 0; i < 8; ++i) buf.push_back(static_cast<char>((v >> (8 * i)) & 0xFF));
	}

	void IKBCSerializer::writeF64(std::string& buf, double v) {
		uint64_t bits;
		std::memcpy(&bits, &v, sizeof(bits));
		for (int i = 0; i < 8; ++i) buf.push_back(static_cast<char>((bits >> (8 * i)) & 0xFF));
	}

	void IKBCSerializer::writeString(std::string& buf, const std::string& s) {
		writeU32(buf, static_cast<uint32_t>(s.size()));
		buf.append(s);
	}

	void IKBCSerializer::writeConstEntry(std::string& buf, const ValuePtr& v) {
		if (!v) {
			writeU8(buf, TAG_NULL); return;
		}

		// TypeDescriptor sentinels are created with Value() then typeDescriptor overridden.
		// They have isInit=false (TypeDescriptor default), while all real Values set isInit=true.
		// The VM never reads these constants; serialize as TAG_TYPEDESC so round-trip is clean.
		if (!v->typeDescriptor.isInit) {
			writeU8(buf, TAG_TYPEDESC);
			writeU8(buf, static_cast<uint8_t>(v->typeDescriptor.type));
			uint8_t flags = 0;
			if (v->typeDescriptor.isInit)     flags |= 0x01;
			if (v->typeDescriptor.isNullable) flags |= 0x02;
			if (v->typeDescriptor.isConst)    flags |= 0x04;
			if (v->typeDescriptor.isDynamic)  flags |= 0x08;
			writeU8(buf, flags);
			return;
		}

		switch (v->getType()) {
			case Type::Null:
			writeU8(buf, TAG_NULL);
			break;
			case Type::Bool:
			writeU8(buf, TAG_BOOL);
			writeU8(buf, v->getBool() ? 1 : 0);
			break;
			case Type::Int:
			writeU8(buf, TAG_INT);
			writeI64(buf, v->getInt());
			break;
			case Type::Float:
			writeU8(buf, TAG_FLOAT);
			writeF64(buf, v->getFloat());
			break;
			case Type::String:
			writeU8(buf, TAG_STRING);
			writeString(buf, v->getString());
			break;
			case Type::Function:
			{
				auto fnc = v->getFunction();
				if (!fnc || fnc->getBodyType() != FunctionBodyType::Bytecode) {
					// Fallback: serialize as null — C++ builtins are never in the pool
					writeU8(buf, TAG_NULL);
					break;
				}
				auto& bfn = std::get<BytecodeFunctionRef>(fnc->body);
				writeU8(buf, TAG_FN_EMBEDDED);
				writeString(buf, fnc->name);
				writeU16(buf, static_cast<uint16_t>(fnc->argNames.size()));
				for (auto& a : fnc->argNames) writeString(buf, a);
				writeU8(buf, fnc->isCoro ? 1 : 0);
				writeBFPayload(buf, *bfn);
				break;
			}
			default:
			// Unsupported constant type — serialize as null
			writeU8(buf, TAG_NULL);
			break;
		}
	}

	void IKBCSerializer::writeBFPayload(std::string& buf, const BytecodeFunction& bfn) {
		writeU16(buf, bfn.localCount);
		writeU16(buf, bfn.upvalueCount);
		writeU8(buf, bfn.isCoro ? 1 : 0);

		// code
		writeU32(buf, static_cast<uint32_t>(bfn.code.size()));
		buf.append(reinterpret_cast<const char*>(bfn.code.data()), bfn.code.size());

		// lines
		writeU32(buf, static_cast<uint32_t>(bfn.lines.size()));
		for (auto ln : bfn.lines) writeI32(buf, ln);

		// bpNodeIds (parallel to lines)
		writeU32(buf, static_cast<uint32_t>(bfn.bpNodeIds.size()));
		for (auto id : bfn.bpNodeIds) writeI32(buf, id);

		// name pool
		writeU32(buf, static_cast<uint32_t>(bfn.names.size()));
		for (auto& n : bfn.names) writeString(buf, n);

		// constant pool
		writeU32(buf, static_cast<uint32_t>(bfn.constants.size()));
		for (auto& c : bfn.constants) writeConstEntry(buf, c);
	}

	void IKBCSerializer::writeFnEntry(std::string& buf, const std::string& name,
		const BytecodeFunction& bfn) {
		writeString(buf, name);
		writeU16(buf, static_cast<uint16_t>(bfn.argNames.size()));
		for (auto& a : bfn.argNames) writeString(buf, a);
		writeU8(buf, bfn.isCoro ? 1 : 0);
		writeBFPayload(buf, bfn);
	}

	// ---------------------------------------------------------------------------
	// serialize
	// ---------------------------------------------------------------------------

	std::string IKBCSerializer::serialize(const Chunk& chunk) {
		std::string buf;
		buf.reserve(4096);

		// Header
		buf.append(kMagic, 4);
		writeU32(buf, kVersion);

		// Named functions
		writeU32(buf, static_cast<uint32_t>(chunk.functions.size()));
		for (auto& [name, bfn] : chunk.functions) {
			if (bfn) writeFnEntry(buf, name, *bfn);
		}

		// main
		writeBFPayload(buf, *chunk.main);

		return buf;
	}

	// ---------------------------------------------------------------------------
	// Read helpers
	// ---------------------------------------------------------------------------

	uint8_t IKBCSerializer::readU8(std::string_view data, size_t& pos) {
		if (pos >= data.size()) throw std::runtime_error("IKBC: unexpected end of data");
		return static_cast<uint8_t>(data[pos++]);
	}

	uint16_t IKBCSerializer::readU16(std::string_view data, size_t& pos) {
		uint8_t lo = readU8(data, pos);
		uint8_t hi = readU8(data, pos);
		return static_cast<uint16_t>(lo | (static_cast<uint16_t>(hi) << 8));
	}

	uint32_t IKBCSerializer::readU32(std::string_view data, size_t& pos) {
		uint32_t v = 0;
		for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(readU8(data, pos)) << (8 * i);
		return v;
	}

	int32_t IKBCSerializer::readI32(std::string_view data, size_t& pos) {
		return static_cast<int32_t>(readU32(data, pos));
	}

	int64_t IKBCSerializer::readI64(std::string_view data, size_t& pos) {
		int64_t v = 0;
		for (int i = 0; i < 8; ++i) v |= static_cast<int64_t>(readU8(data, pos)) << (8 * i);
		return v;
	}

	double IKBCSerializer::readF64(std::string_view data, size_t& pos) {
		uint64_t bits = 0;
		for (int i = 0; i < 8; ++i) bits |= static_cast<uint64_t>(readU8(data, pos)) << (8 * i);
		double v;
		std::memcpy(&v, &bits, sizeof(v));
		return v;
	}

	std::string IKBCSerializer::readString(std::string_view data, size_t& pos) {
		uint32_t len = readU32(data, pos);
		if (pos + len > data.size()) throw std::runtime_error("IKBC: string out of bounds");
		std::string s(data.substr(pos, len));
		pos += len;
		return s;
	}

	ValuePtr IKBCSerializer::readConstEntry(std::string_view data, size_t& pos) {
		uint8_t tag = readU8(data, pos);
		switch (tag) {
			case TAG_NULL:
			return std::make_shared<Value>();
			case TAG_BOOL:
			{
				bool b = readU8(data, pos) != 0;
				return std::make_shared<Value>(b);
			}
			case TAG_INT:
			{
				Int v = readI64(data, pos);
				return std::make_shared<Value>(v);
			}
			case TAG_FLOAT:
			{
				Float v = readF64(data, pos);
				return std::make_shared<Value>(v);
			}
			case TAG_STRING:
			{
				return std::make_shared<Value>(readString(data, pos));
			}
			case TAG_TYPEDESC:
			{
				auto v = std::make_shared<Value>();
				v->typeDescriptor.type = static_cast<Type>(readU8(data, pos));
				uint8_t flags = readU8(data, pos);
				v->typeDescriptor.isInit = (flags & 0x01) != 0;
				v->typeDescriptor.isNullable = (flags & 0x02) != 0;
				v->typeDescriptor.isConst = (flags & 0x04) != 0;
				v->typeDescriptor.isDynamic = (flags & 0x08) != 0;
				return v;
			}
			case TAG_FN_EMBEDDED:
			{
				std::string fname = readString(data, pos);
				uint16_t argc = readU16(data, pos);
				std::vector<std::string> argNames;
				argNames.reserve(argc);
				for (uint16_t i = 0; i < argc; ++i) argNames.push_back(readString(data, pos));
				bool isCoro = readU8(data, pos) != 0;
				auto bfn = readBFPayload(data, pos);
				bfn->name = fname;
				bfn->isCoro = isCoro;
				bfn->argNames = std::move(argNames);

				auto fnc = std::make_shared<Function>(fname);
				fnc->isCoro = isCoro;
				fnc->argNames = bfn->argNames;
				fnc->body = bfn;
				auto v = std::make_shared<Value>(fnc);
				return v;
			}
			default:
			return std::make_shared<Value>();  // unknown tag → null
		}
	}

	BytecodeFunctionRef IKBCSerializer::readBFPayload(std::string_view data, size_t& pos) {
		auto bfn = std::make_shared<BytecodeFunction>();
		bfn->localCount = readU16(data, pos);
		bfn->upvalueCount = readU16(data, pos);
		bfn->isCoro = readU8(data, pos) != 0;

		// code
		uint32_t codeLen = readU32(data, pos);
		if (pos + codeLen > data.size()) throw std::runtime_error("IKBC: code out of bounds");
		bfn->code.assign(reinterpret_cast<const uint8_t*>(data.data() + pos),
			reinterpret_cast<const uint8_t*>(data.data() + pos + codeLen));
		pos += codeLen;

		// lines
		uint32_t linesLen = readU32(data, pos);
		bfn->lines.reserve(linesLen);
		for (uint32_t i = 0; i < linesLen; ++i) bfn->lines.push_back(readI32(data, pos));

		// bpNodeIds (may be absent in older files — treat as all-zero)
		uint32_t bpLen = readU32(data, pos);
		bfn->bpNodeIds.reserve(bpLen);
		for (uint32_t i = 0; i < bpLen; ++i) bfn->bpNodeIds.push_back(readI32(data, pos));
		// Pad to code.size() if serialized with fewer entries
		while (bfn->bpNodeIds.size() < bfn->code.size()) bfn->bpNodeIds.push_back(0);

		// name pool
		uint32_t namesCount = readU32(data, pos);
		bfn->names.reserve(namesCount);
		for (uint32_t i = 0; i < namesCount; ++i) bfn->names.push_back(readString(data, pos));

		// constant pool
		uint32_t constCount = readU32(data, pos);
		bfn->constants.reserve(constCount);
		for (uint32_t i = 0; i < constCount; ++i) bfn->constants.push_back(readConstEntry(data, pos));

		return bfn;
	}

	// ---------------------------------------------------------------------------
	// deserialize
	// ---------------------------------------------------------------------------

	ChunkRef IKBCSerializer::deserialize(std::string_view data) {
		size_t pos = 0;

		// Header
		if (data.size() < 8) throw std::runtime_error("IKBC: data too small");
		if (data[0] != 'I' || data[1] != 'K' || data[2] != 'B' || data[3] != 'C')
			throw std::runtime_error("IKBC: invalid magic");
		pos = 4;
		uint32_t version = readU32(data, pos);
		if (version != kVersion)
			throw std::runtime_error("IKBC: unsupported version " + std::to_string(version));

		auto chunk = std::make_shared<Chunk>();

		// Named functions
		uint32_t fnCount = readU32(data, pos);
		for (uint32_t i = 0; i < fnCount; ++i) {
			std::string name = readString(data, pos);
			uint16_t argc = readU16(data, pos);
			std::vector<std::string> argNames;
			argNames.reserve(argc);
			for (uint16_t j = 0; j < argc; ++j) argNames.push_back(readString(data, pos));
			bool isCoro = readU8(data, pos) != 0;
			auto bfn = readBFPayload(data, pos);
			bfn->name = name;
			bfn->isCoro = isCoro;
			bfn->argNames = std::move(argNames);
			chunk->functions[name] = bfn;
		}

		// main
		chunk->main = readBFPayload(data, pos);
		chunk->main->name = "__main__";

		return chunk;
	}

} // namespace IkigaiScript
