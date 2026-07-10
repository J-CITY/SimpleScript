#include "vm.hpp"
#include "../ikigaiScript.h"
#include "../value.hpp"
#include "../exception.h"
#include "../dependencyManager.hpp"
#include <cassert>
#include <stdexcept>

namespace IkigaiScript {

	VM::VM(IkigaiScriptInterpreter* interp): interp_(interp) {
	}

	// ---------------------------------------------------------------------------
	// Public API
	// ---------------------------------------------------------------------------

	ValuePtr VM::runChunk(ChunkRef chunk, ScopePtr scope) {
		if (!chunk || !chunk->main) return interp_->makeValue();
		return runFunction(chunk->main, scope, {});
	}

	ValuePtr VM::runFunction(BytecodeFunctionRef fn, ScopePtr scope,
		const List& args, Class* classs) {
		// Create call frame scope
		static unsigned vmCallId = 0;
		auto frameScope = interp_->newScope(fn->name + "__vm_" + std::to_string(++vmCallId), scope);

		// Bind args to locals (by position)
		for (size_t i = 0; i < args.size() && i < fn->localCount; ++i) {
			stack_.push_back(args[i]);
		}
		// Pad remaining locals with null
		for (size_t i = args.size(); i < fn->localCount; ++i) {
			stack_.push_back(interp_->makeValue());
		}

		size_t stopDepth = frames_.size();  // stop when this frame is popped
		frames_.push_back({fn, 0, stack_.size() - fn->localCount, frameScope, classs});
		ValuePtr result = execute(stopDepth);
		interp_->closeScope(frameScope);
		return result ? result : interp_->makeValue();
	}

	ValuePtr VM::resumeCoro(TaskRef task) {
		if (!task || task->state == TaskState::Completed || task->state == TaskState::Cancelled)
			return task ? task->result : interp_->makeValue();

		if (task->func->getBodyType() != FunctionBodyType::Bytecode) {
			// Fallback: AST coro handled by interpreter
			return interp_->callCoro(task);
		}

		auto byteFn = std::get<BytecodeFunctionRef>(task->func->body);
		size_t savedStackBase = stack_.size();
		// Restore local slots from task scope variables (simplified: push nulls)
		for (size_t i = 0; i < byteFn->localCount; ++i) {
			stack_.push_back(interp_->makeValue());
		}

		size_t stopDepth = frames_.size();
		frames_.push_back({byteFn,
			static_cast<size_t>(task->pc),
			savedStackBase,
			task->scope,
			task->classs});
		task->state = TaskState::Running;

		ValuePtr result;
		try {
			result = execute(stopDepth);
		}
		catch (...) {
			task->state = TaskState::Cancelled;
			while (stack_.size() > savedStackBase) stack_.pop_back();
			if (!frames_.empty()) frames_.pop_back();
			throw;
		}

		// After execute() the frame was popped; check final task state
		task->result = result;
		if (task->state != TaskState::Suspended) {
			task->state = TaskState::Completed;
		}
		return result;
	}

	// ---------------------------------------------------------------------------
	// Stack helpers
	// ---------------------------------------------------------------------------

	void VM::push(ValuePtr v) {
		stack_.push_back(v ? v : interp_->makeValue());
	}

	ValuePtr VM::pop() {
		if (stack_.empty()) return interp_->makeValue();
		auto v = std::move(stack_.back());
		stack_.pop_back();
		return v;
	}

	ValuePtr& VM::peek(size_t depth) {
		static ValuePtr null_ = std::make_shared<Value>();
		if (depth >= stack_.size()) return null_;
		return stack_[stack_.size() - 1 - depth];
	}

	// ---------------------------------------------------------------------------
	// Read helpers (consume bytes from the current frame)
	// ---------------------------------------------------------------------------

	uint8_t VM::readByte(CallFrame& f) {
		return f.fn->code[f.ip++];
	}

	uint16_t VM::readU16(CallFrame& f) {
		uint16_t lo = f.fn->code[f.ip++];
		uint16_t hi = f.fn->code[f.ip++];
		return static_cast<uint16_t>(lo | (hi << 8));
	}

	int16_t VM::readI16(CallFrame& f) {
		return static_cast<int16_t>(readU16(f));
	}

	// ---------------------------------------------------------------------------
	// Runtime helpers
	// ---------------------------------------------------------------------------

	bool VM::isTruthy(const ValuePtr& v) const {
		if (!v) return false;
		switch (v->getType()) {
			case Type::Bool:  return v->getBool();
			case Type::Int:   return v->getInt() != 0;
			case Type::Float: return v->getFloat() != 0.0;
			case Type::Null:  return false;
			case Type::String: return !v->getString().empty();
			default:          return true;
		}
	}

	void VM::defineVar(const std::string& name, ValuePtr val, bool isConst, ScopePtr scope) {
		val->typeDescriptor.isInit = true;
		val->typeDescriptor.isConst = isConst;
		if (val->typeDescriptor.type == Type::Null && val->typeDescriptor.isInit) {
			val->typeDescriptor.type = val->getType();
		}
		scope->variables[name] = val;
	}

	ValuePtr VM::callValue(ValuePtr callee, size_t argc, ScopePtr scope, Class* classs) {
		if (!callee || callee->getType() != Type::Function) {
			throw Exception("VM: cannot call non-function value");
		}
		auto fnc = callee->getFunction();
		List args;
		args.resize(argc);
		for (int i = static_cast<int>(argc) - 1; i >= 0; --i) {
			args[i] = pop();
		}
		return interp_->callFunction(fnc, scope, args, classs);
	}

	// ---------------------------------------------------------------------------
	// Main dispatch loop
	// ---------------------------------------------------------------------------

	ValuePtr VM::execute(size_t stopDepth) {
		if (frames_.empty()) return interp_->makeValue();

		while (frames_.size() > stopDepth) {
			CallFrame& frame = frames_.back();
			auto& fn = *frame.fn;
			auto& code = fn.code;
			auto& scope = frame.scope;

			if (frame.ip >= code.size()) {
				while (stack_.size() > frame.stackBase) stack_.pop_back();
				frames_.pop_back();
				if (frames_.size() <= stopDepth) return interp_->makeValue();
				push(interp_->makeValue());  // null return propagated to caller
				break;
			}

			auto op = static_cast<OpCode>(readByte(frame));
			switch (op) {

				// ---------------------------------------------------------------
			case OpCode::OP_PUSH_NULL:
			{
				push(interp_->makeValue());
				break;
			}
				case OpCode::OP_PUSH_CONST:
				{
					uint16_t idx = readU16(frame);
					push(fn.constants[idx]);
					break;
				}
				case OpCode::OP_POP:
				pop();
				break;
				case OpCode::OP_DUP:
				push(peek(0));
				break;

				// ---------------------------------------------------------------
				case OpCode::OP_GET_LOCAL:
				{
					uint16_t slot = readU16(frame);
					size_t absIdx = frame.stackBase + slot;
					push(absIdx < stack_.size() ? stack_[absIdx] : interp_->makeValue());
					break;
				}
				case OpCode::OP_SET_LOCAL:
				{
					uint16_t slot = readU16(frame);
					size_t absIdx = frame.stackBase + slot;
					if (absIdx < stack_.size()) stack_[absIdx] = pop();
					break;
				}

				// ---------------------------------------------------------------
				case OpCode::OP_GET_GLOBAL:
				{
					uint16_t nameIdx = readU16(frame);
					const auto& name = fn.names[nameIdx];
					auto& var = interp_->resolveVariable(name, frame.classs, scope);
					push(var);
					break;
				}
				case OpCode::OP_SET_GLOBAL:
				{
					uint16_t nameIdx = readU16(frame);
					const auto& name = fn.names[nameIdx];
					auto val = pop();
					auto& var = interp_->resolveVariableForWrite(name, frame.classs, scope);
					*var = *val;
					interp_->dependencyManager.onVariableChanged(VarSlot{var.get()});
					break;
				}
			case OpCode::OP_DEFINE_VAR:
			{
				uint16_t nameIdx = readU16(frame);
				const auto& name = fn.names[nameIdx];
				auto val = interp_->copyValue(*pop());
				defineVar(name, val, false, scope);
				break;
			}
			case OpCode::OP_DEFINE_CONST:
			{
				uint16_t nameIdx = readU16(frame);
				const auto& name = fn.names[nameIdx];
				auto val = interp_->copyValue(*pop());
				defineVar(name, val, true, scope);
				break;
			}

				// ---------------------------------------------------------------
				case OpCode::OP_GET_MEMBER:
				{
					uint16_t nameIdx = readU16(frame);
					const auto& memberName = fn.names[nameIdx];
					auto obj = pop();
					if (obj->getType() == Type::Class) {
						auto cls = obj->getClass();
						auto it = cls->variables.find(memberName);
						if (it != cls->variables.end()) {
							push(it->second); break;
						}
					}
					// Fall back to scope resolution for module / tuple .N access
					auto& var = interp_->resolveVariable(memberName, frame.classs, scope);
					push(var);
					break;
				}
				case OpCode::OP_SET_MEMBER:
				{
					uint16_t nameIdx = readU16(frame);
					const auto& memberName = fn.names[nameIdx];
					auto val = pop();
					auto obj = pop();
					if (obj->getType() == Type::Class) {
						auto cls = obj->getClass();
						auto it = cls->variables.find(memberName);
						if (it != cls->variables.end()) {
							*it->second = *val;
							interp_->dependencyManager.onVariableChanged(VarSlot{it->second.get()});
							break;
						}
					}
					break;
				}

				// ---------------------------------------------------------------
				case OpCode::OP_CALL:
				{
					uint8_t argc = readByte(frame);
					auto callee = pop();
					auto result = callValue(callee, argc, scope, frame.classs);
					push(result);
					break;
				}
				case OpCode::OP_CALL_BUILTIN:
				case OpCode::OP_CALL_OPERATOR:
				{
					uint16_t nameIdx = readU16(frame);
					uint8_t argc = readByte(frame);
					const auto& name = fn.names[nameIdx];
					List args(argc);
					for (int i = argc - 1; i >= 0; --i) args[i] = pop();
					auto fnc = interp_->resolveFunction(name, frame.classs, scope);
					if (!fnc) fnc = interp_->resolveOperator(name, scope, args);
					if (!fnc) throw Exception("VM: undefined function '" + name + "'");
					push(interp_->callFunction(fnc, scope, args, frame.classs));
					break;
				}
				case OpCode::OP_CALL_METHOD:
				{
					uint16_t nameIdx = readU16(frame);
					uint8_t argc = readByte(frame);
					const auto& name = fn.names[nameIdx];
					auto receiver = pop();
					List args(argc);
					for (int i = argc - 1; i >= 0; --i) args[i] = pop();
					auto cls = receiver->getType() == Type::Class ? receiver->getClass().get() : nullptr;
					auto fnc = interp_->resolveFunction(name, cls, scope);
					if (!fnc) throw Exception("VM: undefined method '" + name + "'");
					push(interp_->callFunction(fnc, scope, args, cls));
					break;
				}

				// ---------------------------------------------------------------
				case OpCode::OP_RETURN:
				{
					auto ret = pop();
					while (stack_.size() > frame.stackBase) stack_.pop_back();
					frames_.pop_back();
					if (frames_.size() <= stopDepth) return ret;
					push(ret);
					break;
				}
			case OpCode::OP_RETURN_NIL:
			{
				while (stack_.size() > frame.stackBase) stack_.pop_back();
				frames_.pop_back();
				if (frames_.size() <= stopDepth) return interp_->makeValue();
				push(interp_->makeValue());
				break;
			}

				// ---------------------------------------------------------------
				case OpCode::OP_ADD:
				{
					auto b = pop(); auto a = pop();
					List args = {a, b};
					auto fnc = interp_->resolveOperator("+", scope, args);
					if (!fnc) throw Exception("VM: '+' undefined for types");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}
				case OpCode::OP_SUB:
				{
					auto b = pop(); auto a = pop();
					List args = {a, b};
					auto fnc = interp_->resolveOperator("-", scope, args);
					if (!fnc) throw Exception("VM: '-' undefined");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}
				case OpCode::OP_MUL:
				{
					auto b = pop(); auto a = pop();
					List args = {a, b};
					auto fnc = interp_->resolveOperator("*", scope, args);
					if (!fnc) throw Exception("VM: '*' undefined");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}
				case OpCode::OP_DIV:
				{
					auto b = pop(); auto a = pop();
					List args = {a, b};
					auto fnc = interp_->resolveOperator("/", scope, args);
					if (!fnc) throw Exception("VM: '/' undefined");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}
				case OpCode::OP_MOD:
				{
					auto b = pop(); auto a = pop();
					List args = {a, b};
					auto fnc = interp_->resolveOperator("%", scope, args);
					if (!fnc) throw Exception("VM: '%' undefined");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}
				case OpCode::OP_NEG:
				{
					auto a = pop();
					List args = {a};
					auto fnc = interp_->resolveFunction("neg", scope);
					if (fnc) {
						push(interp_->callFunction(fnc, scope, args)); break;
					}
				// Fallback: inline negation
				if (a->getType() == Type::Int) {
					push(interp_->makeValue(-a->getInt())); break;
				}
				if (a->getType() == Type::Float) {
					push(interp_->makeValue(-a->getFloat())); break;
				}
					throw Exception("VM: unary '-' on non-numeric");
				}

				// ---------------------------------------------------------------
				case OpCode::OP_EQ: case OpCode::OP_NE:
				case OpCode::OP_LT: case OpCode::OP_LE:
				case OpCode::OP_GT: case OpCode::OP_GE:
				{
					auto b = pop(); auto a = pop();
					List args = {a, b};
					const char* opName =
						op == OpCode::OP_EQ ? "==" : op == OpCode::OP_NE ? "!=" :
						op == OpCode::OP_LT ? "<" : op == OpCode::OP_LE ? "<=" :
						op == OpCode::OP_GT ? ">" : ">=";
					auto fnc = interp_->resolveOperator(opName, scope, args);
					if (!fnc) throw Exception(std::string("VM: '") + opName + "' undefined");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}

				// ---------------------------------------------------------------
			case OpCode::OP_NOT:
			{
				auto a = pop();
				push(interp_->makeValue(!isTruthy(a)));
				break;
			}
				case OpCode::OP_AND_SHORT:
				{
					int16_t offset = readI16(frame);
					if (!isTruthy(peek(0))) {
						frame.ip += offset;
					}
					else {
						pop(); // discard left side; result will be the right side
					}
					break;
				}
				case OpCode::OP_OR_SHORT:
				{
					int16_t offset = readI16(frame);
					if (isTruthy(peek(0))) {
						frame.ip += offset;
					}
					else {
						pop();
					}
					break;
				}

				// ---------------------------------------------------------------
				case OpCode::OP_JUMP:
				{
					int16_t offset = readI16(frame);
					frame.ip += offset;
					break;
				}
				case OpCode::OP_JUMP_IF_FALSE:
				{
					int16_t offset = readI16(frame);
					auto cond = pop();
					if (!isTruthy(cond)) frame.ip += offset;
					break;
				}
				case OpCode::OP_JUMP_IF_TRUE:
				{
					int16_t offset = readI16(frame);
					auto cond = pop();
					if (isTruthy(cond)) frame.ip += offset;
					break;
				}

				// ---------------------------------------------------------------
			case OpCode::OP_MAKE_LIST:
			{
				uint16_t count = readU16(frame);
				List items(count);
				for (int i = count - 1; i >= 0; --i) items[i] = pop();
				push(interp_->makeValue(std::move(items)));
				break;
			}
			case OpCode::OP_MAKE_TUPLE:
			{
				uint16_t count = readU16(frame);
				List items(count);
				for (int i = count - 1; i >= 0; --i) items[i] = pop();
				push(interp_->makeValue(Value::makeTuple(std::move(items))));
				break;
			}
				case OpCode::OP_GET_INDEX:
				{
					auto idx = pop();
					auto container = pop();
					List args = {container, idx};
					auto fnc = interp_->resolveFunction("listindex", scope);
					if (!fnc) fnc = interp_->resolveFunction("__getindex__", scope);
					if (!fnc) throw Exception("VM: indexing not available");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}
				case OpCode::OP_SET_INDEX:
				{
					auto val = pop(); auto idx = pop(); auto container = pop();
					List args = {container, idx, val};
					auto fnc = interp_->resolveFunction("__setindex__", scope);
					if (!fnc) throw Exception("VM: index assignment not available");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}
				case OpCode::OP_MAKE_RANGE:
				{
					uint8_t inclusive = readByte(frame);
					auto end_ = pop(); auto start = pop();
					List args = {start, end_};
					const char* rangeOp = inclusive ? "..=" : "..";
					auto fnc = interp_->resolveOperator(rangeOp, scope, args);
					if (!fnc) throw Exception("VM: range operator undefined");
					push(interp_->callFunction(fnc, scope, args));
					break;
				}

				// ---------------------------------------------------------------
				case OpCode::OP_YELD:
				{
					auto payload = pop();
					// Suspend: save ip in the active Task.
					// The current frame's ip is already advanced past YELD.
					// The caller (resumeCoro / callCoro) checks task->state.
					// We can't easily get the TaskRef here, so we throw a special
					// sentinel exception that callCoro catches.
					while (stack_.size() > frame.stackBase) stack_.pop_back();
					frames_.pop_back();
					// Propagate yield via exception to let callCoro handle it.
					// This keeps the VM stateless regarding active tasks.
					throw YieldSignal{payload, static_cast<int>(frame.ip)};
				}
				case OpCode::OP_AWAIT:
				{
					auto taskVal = pop();
					if (taskVal->getType() != Type::Coro) {
						push(taskVal);
						break;
					}
					auto task = taskVal->getTask();
				if (task->state == TaskState::Completed) {
					push(task->result ? task->result : interp_->makeValue());
					break;
				}
				// Drive to completion via interpreter's callCoro
				while (task->state != TaskState::Completed && task->state != TaskState::Cancelled) {
					interp_->callCoro(task);
				}
				push(task->result ? task->result : interp_->makeValue());
					break;
				}
				case OpCode::OP_SPAWN:
				{
					uint8_t argc = readByte(frame);
					auto callee = pop();
					List args(argc);
					for (int i = argc - 1; i >= 0; --i) args[i] = pop();
					if (callee->getType() == Type::Function) {
						auto taskResult = interp_->callFunction(callee->getFunction(), scope, args);
						push(taskResult);
					}
				else {
					push(interp_->makeValue());
				}
				break;
			}

			// ---------------------------------------------------------------
			case OpCode::OP_ENTER_TX:
				{
					// Open a transaction scope (COW overlay).
					auto txScope = interp_->newScope("__tx__", scope);
					txScope->isTransactionScope = true;
					frame.scope = txScope;
					scope = txScope;
					break;
				}
				case OpCode::OP_EXIT_TX:
				{
					uint8_t producesValue = readByte(frame);
					bool success = true;
					ValuePtr result;
					if (producesValue && !stack_.empty()) result = pop();
					// Commit or rollback based on exception state (simplified: always commit).
					interp_->commitTransaction(scope);
					auto parentScope = scope->parent;
					interp_->closeScope(scope, false);
					if (parentScope) frame.scope = parentScope;
				if (producesValue) {
					push(result ? result : interp_->makeValue());
				}
				else {
					push(interp_->makeValue(true));
				}
					break;
				}

				// ---------------------------------------------------------------
				case OpCode::OP_CLOSE_SCOPE:
				{
					uint8_t runDeferred = readByte(frame);
					if (runDeferred) {
						interp_->runDefers(scope, frame.classs);
					}
					break;
				}
				case OpCode::OP_DEFER_REGISTER:
				{
					// For bytecode defers, the body is a compiled BytecodeFunction
					// stored as a constant.  We wrap it in a fake ExpressionPtr
					// (null here; deferred to future extension).
					uint16_t idx = readU16(frame);
					(void)idx;
					// Phase 5+: store fn.constants[idx] as defer body.
					break;
				}

				// ---------------------------------------------------------------
				default:
				throw Exception("VM: unknown opcode " +
					std::to_string(static_cast<int>(op)));
			}
		}

		return interp_->makeValue();
	}

} // namespace IkigaiScript
