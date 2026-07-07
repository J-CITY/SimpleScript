#include "disassembler.hpp"
#include "../value.hpp"
#include <sstream>
#include <iomanip>

namespace IkigaiScript {

static std::string pad(size_t ip) {
    std::ostringstream os;
    os << std::setw(5) << std::setfill('0') << ip;
    return os.str();
}

static size_t disassembleInstruction(std::ostringstream& out,
                                      const BytecodeFunction& fn,
                                      size_t ip) {
    const auto& code = fn.code;
    int srcLine = ip < fn.lines.size() ? fn.lines[ip] : -1;
    out << pad(ip);
    if (srcLine >= 0) out << " [L" << std::setw(4) << std::setfill(' ') << srcLine << "]";
    else              out << "        ";
    out << "  ";

    auto op = static_cast<OpCode>(code[ip]);
    out << opcodeName(op);

    switch (op) {
    // --- 2-byte operand (u16 name/const index) ---
    case OpCode::OP_PUSH_CONST: {
        uint16_t idx = BytecodeFunction::readU16(code, ip + 1);
        out << "  [" << idx << "]";
        if (idx < fn.constants.size() && fn.constants[idx]) {
            auto& v = fn.constants[idx];
            out << "  ; " << v->getPrintString();
        }
        return ip + 3;
    }
    case OpCode::OP_GET_GLOBAL:
    case OpCode::OP_SET_GLOBAL:
    case OpCode::OP_DEFINE_VAR:
    case OpCode::OP_DEFINE_CONST:
    case OpCode::OP_GET_MEMBER:
    case OpCode::OP_SET_MEMBER:
    case OpCode::OP_CALL_BUILTIN:
    case OpCode::OP_CALL_OPERATOR:
    case OpCode::OP_CALL_METHOD:
    case OpCode::OP_DEFER_REGISTER: {
        uint16_t idx = BytecodeFunction::readU16(code, ip + 1);
        out << "  [" << idx << "]";
        if (idx < fn.names.size()) out << "  ; '" << fn.names[idx] << "'";
        return ip + 3;
    }
    case OpCode::OP_GET_LOCAL:
    case OpCode::OP_SET_LOCAL: {
        uint16_t slot = BytecodeFunction::readU16(code, ip + 1);
        out << "  slot=" << slot;
        return ip + 3;
    }
    // --- Signed 2-byte offset ---
    case OpCode::OP_JUMP:
    case OpCode::OP_JUMP_IF_FALSE:
    case OpCode::OP_JUMP_IF_TRUE:
    case OpCode::OP_AND_SHORT:
    case OpCode::OP_OR_SHORT: {
        int16_t offset = BytecodeFunction::readI16(code, ip + 1);
        size_t target  = static_cast<size_t>(static_cast<int64_t>(ip + 3) + offset);
        out << "  offset=" << offset << "  -> " << pad(target);
        return ip + 3;
    }
    // --- 1-byte operand ---
    case OpCode::OP_CALL:
    case OpCode::OP_SPAWN: {
        out << "  argc=" << static_cast<int>(code[ip + 1]);
        return ip + 2;
    }
    case OpCode::OP_MAKE_RANGE: {
        out << "  inclusive=" << static_cast<int>(code[ip + 1]);
        return ip + 2;
    }
    case OpCode::OP_CLOSE_SCOPE:
    case OpCode::OP_EXIT_TX: {
        out << "  flag=" << static_cast<int>(code[ip + 1]);
        return ip + 2;
    }
    // --- 2-byte list/tuple count ---
    case OpCode::OP_MAKE_LIST:
    case OpCode::OP_MAKE_TUPLE: {
        uint16_t cnt = BytecodeFunction::readU16(code, ip + 1);
        out << "  count=" << cnt;
        return ip + 3;
    }
    // --- 3-byte: u16 name_idx + u8 argc ---
    // (CALL_BUILTIN, CALL_OPERATOR, CALL_METHOD already handled above for name)
    // --- Zero-operand instructions ---
    default:
        return ip + 1;
    }
}

std::string disassemble(const BytecodeFunction& fn) {
    std::ostringstream out;
    out << "=== " << fn.name << " ===\n";
    size_t ip = 0;
    while (ip < fn.code.size()) {
        out << "\n";
        ip = disassembleInstruction(out, fn, ip);
    }
    out << "\n";
    return out.str();
}

std::string disassemble(const Chunk& chunk) {
    std::ostringstream out;
    if (chunk.main) out << disassemble(*chunk.main);
    for (auto& [name, fn] : chunk.functions) {
        if (fn) out << disassemble(*fn);
    }
    return out.str();
}

} // namespace IkigaiScript
