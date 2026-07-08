#pragma once
#include <cstdint>

namespace IkigaiScript {

	// Stack-machine opcodes for the IkigaiScript bytecode VM.
	//
	// Encoding: 1 byte opcode.  Most instructions that reference the constant
	// pool or name table carry a 2-byte (uint16_t) little-endian operand
	// immediately following the opcode byte.  The CALL instruction carries a
	// 1-byte arg-count operand.  Zero-operand instructions occupy exactly 1 byte.
	//
	// --- Operand encoding summary ---
	//   OP_PUSH_CONST  <u16 const_idx>    push constants[const_idx]
	//   OP_GET_GLOBAL  <u16 name_idx>     push globals[ names[name_idx] ]
	//   OP_SET_GLOBAL  <u16 name_idx>     pop → globals[ names[name_idx] ]
	//   OP_GET_LOCAL   <u16 slot>         push stack[frameBase + slot]
	//   OP_SET_LOCAL   <u16 slot>         pop → stack[frameBase + slot]
	//   OP_DEFINE_VAR  <u16 name_idx>     pop value, define in current scope
	//   OP_GET_MEMBER  <u16 name_idx>     pop obj, push obj.field
	//   OP_SET_MEMBER  <u16 name_idx>     pop val+obj, set obj.field = val
	//   OP_CALL        <u8  argc>         pop argc args + callee, push result
	//   OP_CALL_BUILTIN<u16 name_idx>     call stdlib fn by name; argc on stack
	//   OP_CALL_OPERATOR<u16 name_idx>    call operator by name; argc on stack
	//   OP_CALL_METHOD <u16 name_idx>     pop argc args + receiver, push result
	//   OP_JUMP        <i16 offset>       ip += offset (relative, after instruction)
	//   OP_JUMP_IF_FALSE<i16 offset>      pop; if falsy ip += offset
	//   OP_JUMP_IF_TRUE <i16 offset>      pop; if truthy ip += offset
	//   OP_AND_SHORT   <i16 offset>       if TOS falsy jump (keep TOS), else pop
	//   OP_OR_SHORT    <i16 offset>       if TOS truthy jump (keep TOS), else pop
	//   OP_MAKE_LIST   <u16 count>        pop count items, push List
	//   OP_MAKE_TUPLE  <u16 count>        pop count items, push Tuple
	//   OP_GET_INDEX                      pop idx + container, push result
	//   OP_SET_INDEX                      pop val + idx + container
	//   OP_MAKE_RANGE  <u8 inclusive>     pop end + start, push Range
	//
	// Coroutine / concurrency:
	//   OP_YELD                           pop payload, suspend current coro
	//   OP_AWAIT                          pop task, run to completion, push result
	//   OP_SPAWN                          pop callee+args, create Task, push it
	//
	// Transactions:
	//   OP_ENTER_TX                       open transaction scope
	//   OP_EXIT_TX                        commit or rollback; push Bool/Optional
	//
	// Scope / defer:
	//   OP_CLOSE_SCOPE <u8 runDeferred>   close current scope (1=run defers)
	//   OP_DEFER_REGISTER<u16 const_idx>  register defer body (ExpressionPtr stored as constant)

	enum class OpCode: uint8_t {
		// --- Stack manipulation ---
		OP_PUSH_CONST,      // u16 const_idx
		OP_PUSH_NULL,       // push null value
		OP_POP,             // discard TOS
		OP_DUP,             // duplicate TOS

		// --- Locals (stack slots relative to frame base) ---
		OP_GET_LOCAL,       // u16 slot
		OP_SET_LOCAL,       // u16 slot

		// --- Globals (named variables in scope chain) ---
		OP_GET_GLOBAL,      // u16 name_idx
		OP_SET_GLOBAL,      // u16 name_idx
		OP_DEFINE_VAR,      // u16 name_idx  (pops value; creates in current scope)
		OP_DEFINE_CONST,    // u16 name_idx  (like DEFINE_VAR but marks isConst)

		// --- Member access ---
		OP_GET_MEMBER,      // u16 name_idx  (pops object)
		OP_SET_MEMBER,      // u16 name_idx  (pops value then object)

		// --- Function calls ---
		OP_CALL,            // u8  argc  (callee below args on stack)
		OP_CALL_BUILTIN,    // u16 name_idx + u8 argc
		OP_CALL_OPERATOR,   // u16 name_idx + u8 argc
		OP_CALL_METHOD,     // u16 name_idx + u8 argc  (receiver below args)
		OP_RETURN,          // pop return value, restore frame
		OP_RETURN_NIL,      // return null, restore frame

		// --- Arithmetic ---
		OP_ADD,
		OP_SUB,
		OP_MUL,
		OP_DIV,
		OP_MOD,
		OP_NEG,             // unary minus

		// --- Comparison ---
		OP_EQ,
		OP_NE,
		OP_LT,
		OP_LE,
		OP_GT,
		OP_GE,

		// --- Logic ---
		OP_NOT,             // unary !
		OP_AND_SHORT,       // i16 offset  (short-circuit &&)
		OP_OR_SHORT,        // i16 offset  (short-circuit ||)

		// --- Control flow ---
		OP_JUMP,            // i16 offset
		OP_JUMP_IF_FALSE,   // i16 offset
		OP_JUMP_IF_TRUE,    // i16 offset

		// --- Collections ---
		OP_MAKE_LIST,       // u16 count
		OP_MAKE_TUPLE,      // u16 count
		OP_GET_INDEX,       // pop idx + container → push result
		OP_SET_INDEX,       // pop val + idx + container
		OP_MAKE_RANGE,      // u8 inclusive (0=exclusive, 1=inclusive)

		// --- Coroutines / concurrency ---
		OP_YELD,            // suspend coro; pop payload
		OP_AWAIT,           // pop task, drive to completion, push result
		OP_SPAWN,           // u8 argc; pop callee+args, push TaskRef

		// --- Transactions ---
		OP_ENTER_TX,
		OP_EXIT_TX,         // u8 producesValue

		// --- Scope / defer ---
		OP_CLOSE_SCOPE,     // u8 runDeferred
		OP_DEFER_REGISTER,  // u16 const_idx  (index of deferred ExpressionPtr in constants)

		// --- Sentinel ---
		OP_COUNT
	};

	// Human-readable name for debugging / disassembler.
	inline const char* opcodeName(OpCode op) {
		switch (op) {
			case OpCode::OP_PUSH_CONST:      return "PUSH_CONST";
			case OpCode::OP_PUSH_NULL:       return "PUSH_NULL";
			case OpCode::OP_POP:             return "POP";
			case OpCode::OP_DUP:             return "DUP";
			case OpCode::OP_GET_LOCAL:       return "GET_LOCAL";
			case OpCode::OP_SET_LOCAL:       return "SET_LOCAL";
			case OpCode::OP_GET_GLOBAL:      return "GET_GLOBAL";
			case OpCode::OP_SET_GLOBAL:      return "SET_GLOBAL";
			case OpCode::OP_DEFINE_VAR:      return "DEFINE_VAR";
			case OpCode::OP_DEFINE_CONST:    return "DEFINE_CONST";
			case OpCode::OP_GET_MEMBER:      return "GET_MEMBER";
			case OpCode::OP_SET_MEMBER:      return "SET_MEMBER";
			case OpCode::OP_CALL:            return "CALL";
			case OpCode::OP_CALL_BUILTIN:    return "CALL_BUILTIN";
			case OpCode::OP_CALL_OPERATOR:   return "CALL_OPERATOR";
			case OpCode::OP_CALL_METHOD:     return "CALL_METHOD";
			case OpCode::OP_RETURN:          return "RETURN";
			case OpCode::OP_RETURN_NIL:      return "RETURN_NIL";
			case OpCode::OP_ADD:             return "ADD";
			case OpCode::OP_SUB:             return "SUB";
			case OpCode::OP_MUL:             return "MUL";
			case OpCode::OP_DIV:             return "DIV";
			case OpCode::OP_MOD:             return "MOD";
			case OpCode::OP_NEG:             return "NEG";
			case OpCode::OP_EQ:              return "EQ";
			case OpCode::OP_NE:              return "NE";
			case OpCode::OP_LT:              return "LT";
			case OpCode::OP_LE:              return "LE";
			case OpCode::OP_GT:              return "GT";
			case OpCode::OP_GE:              return "GE";
			case OpCode::OP_NOT:             return "NOT";
			case OpCode::OP_AND_SHORT:       return "AND_SHORT";
			case OpCode::OP_OR_SHORT:        return "OR_SHORT";
			case OpCode::OP_JUMP:            return "JUMP";
			case OpCode::OP_JUMP_IF_FALSE:   return "JUMP_IF_FALSE";
			case OpCode::OP_JUMP_IF_TRUE:    return "JUMP_IF_TRUE";
			case OpCode::OP_MAKE_LIST:       return "MAKE_LIST";
			case OpCode::OP_MAKE_TUPLE:      return "MAKE_TUPLE";
			case OpCode::OP_GET_INDEX:       return "GET_INDEX";
			case OpCode::OP_SET_INDEX:       return "SET_INDEX";
			case OpCode::OP_MAKE_RANGE:      return "MAKE_RANGE";
			case OpCode::OP_YELD:            return "YELD";
			case OpCode::OP_AWAIT:           return "AWAIT";
			case OpCode::OP_SPAWN:           return "SPAWN";
			case OpCode::OP_ENTER_TX:        return "ENTER_TX";
			case OpCode::OP_EXIT_TX:         return "EXIT_TX";
			case OpCode::OP_CLOSE_SCOPE:     return "CLOSE_SCOPE";
			case OpCode::OP_DEFER_REGISTER:  return "DEFER_REGISTER";
			default:                         return "<?>";
		}
	}

} // namespace IkigaiScript
