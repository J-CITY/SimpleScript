#include "compiler.hpp"
#include "../ikigaiScript.h"
#include "../value.hpp"
#include <cassert>

namespace IkigaiScript {

	BytecodeCompiler::BytecodeCompiler(IkigaiScriptInterpreter* interp)
		: interpreter_(interp) {
	}

	// ---------------------------------------------------------------------------
	// Public API
	// ---------------------------------------------------------------------------

	BytecodeFunctionRef BytecodeCompiler::compileFunction(FunctionRef fnc) {
		if (fnc->getBodyType() != FunctionBodyType::Subexpressions) return nullptr;
		auto& stmts = std::get<std::vector<ExpressionPtr>>(fnc->body);

		current_ = std::make_shared<BytecodeFunction>();
		current_->name = fnc->name;
		current_->isCoro = fnc->isCoro;
		locals_.clear();
		localCount_ = 0;
		loopStack_.clear();

		// Bind parameters as local slots (slot 0 … n-1).
		for (auto& argName : fnc->argNames) {
			locals_.push_back({argName, localCount_++});
		}
		current_->localCount = localCount_;

		compileBlock(stmts);

		// Ensure every path ends with a return.
		if (current_->code.empty() ||
			static_cast<OpCode>(current_->code.back()) != OpCode::OP_RETURN) {
			current_->emit(OpCode::OP_RETURN_NIL);
		}
		return current_;
	}

	BytecodeFunctionRef BytecodeCompiler::compileStatements(
		const std::vector<ExpressionPtr>& stmts, const std::string& name) {
		current_ = std::make_shared<BytecodeFunction>();
		current_->name = name;
		locals_.clear();
		localCount_ = 0;
		loopStack_.clear();

		compileBlock(stmts);

		if (current_->code.empty() ||
			static_cast<OpCode>(current_->code.back()) != OpCode::OP_RETURN) {
			current_->emit(OpCode::OP_RETURN_NIL);
		}
		return current_;
	}

	// ---------------------------------------------------------------------------
	// Internal helpers
	// ---------------------------------------------------------------------------

	void BytecodeCompiler::compile(ExpressionPtr node) {
		if (!node) {
			current_->emit(OpCode::OP_PUSH_NULL, resolveLine(node));
			return;
		}
		node->accept(*this);
	}

	void BytecodeCompiler::compileBlock(const std::vector<ExpressionPtr>& stmts) {
		for (auto* stmt : stmts) {
			compile(stmt);
			// Discard statement results that weren't consumed (statement context).
			// FunctionDef and DefineVar produce side effects but no net stack value.
			if (stmt && stmt->type != ExpressionType::FunctionDef
				&& stmt->type != ExpressionType::DefineVar
				&& stmt->type != ExpressionType::Return
				&& stmt->type != ExpressionType::Yield
				&& stmt->type != ExpressionType::Continue
				&& stmt->type != ExpressionType::Break
				&& stmt->type != ExpressionType::Loop
				&& stmt->type != ExpressionType::ForEach
				&& stmt->type != ExpressionType::IfElse
				&& stmt->type != ExpressionType::Match
				&& stmt->type != ExpressionType::Defer
				&& stmt->type != ExpressionType::SafeBlock
				&& stmt->type != ExpressionType::LiveRebind
				&& stmt->type != ExpressionType::DestructuringAssign
				&& stmt->type != ExpressionType::SyncBlock
				&& stmt->type != ExpressionType::RaceBlock
				&& stmt->type != ExpressionType::BranchBlock) {
				current_->emit(OpCode::OP_POP);
			}
		}
	}

	int BytecodeCompiler::resolveLine(ExpressionPtr node) const {
		if (!node) return 0;
		return node->line;
	}

	uint16_t BytecodeCompiler::addConst(ValuePtr v) {
		return current_->addConstant(std::move(v));
	}

	uint16_t BytecodeCompiler::addName(const std::string& name) {
		return current_->addName(name);
	}

	int BytecodeCompiler::resolveLocal(const std::string& name) const {
		for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
			if (locals_[i].name == name) return locals_[i].slot;
		}
		return -1;
	}

	void BytecodeCompiler::pushLoopContext(size_t loopStart) {
		loopStack_.push_back({});
		loopStack_.back().loopStart = loopStart;
	}

	void BytecodeCompiler::popLoopContext() {
		loopStack_.pop_back();
	}

	void BytecodeCompiler::patchBreaks() {
		if (loopStack_.empty()) return;
		for (auto off : loopStack_.back().breakJumps) current_->patchJump(off);
		loopStack_.back().breakJumps.clear();
	}

	void BytecodeCompiler::patchContinues() {
		if (loopStack_.empty()) return;
		for (auto off : loopStack_.back().continueJumps) current_->patchJump(off);
		loopStack_.back().continueJumps.clear();
	}

	// ---------------------------------------------------------------------------
	// visit() implementations
	// ---------------------------------------------------------------------------

	void BytecodeCompiler::visit(ValueNode& n) {
		int ln = resolveLine(&n);
		if (!n.value) {
			current_->emit(OpCode::OP_PUSH_NULL, ln);
			return;
		}
		uint16_t idx = addConst(n.value);
		current_->emitU16(OpCode::OP_PUSH_CONST, idx, ln);
	}

	void BytecodeCompiler::visit(ResolveVar& n) {
		int ln = resolveLine(&n);
		int slot = resolveLocal(n.name);
		if (slot >= 0) {
			current_->emitU16(OpCode::OP_GET_LOCAL, static_cast<uint16_t>(slot), ln);
		}
		else {
			uint16_t idx = addName(n.name);
			current_->emitU16(OpCode::OP_GET_GLOBAL, idx, ln);
		}
	}

	void BytecodeCompiler::visit(DefineVar& n) {
		int ln = resolveLine(&n);
		if (n.defineExpression) {
			compile(n.defineExpression);
		}
		else {
			current_->emit(OpCode::OP_PUSH_NULL, ln);
		}
		// Store TypeDescriptor as a constant so the VM can apply type enforcement.
		auto tdVal = interpreter_->makeValue();
		tdVal->typeDescriptor = n.typeDescriptor;
		uint16_t tdIdx = addConst(tdVal);
		(void)tdIdx; // VM reads at name_idx+1 convention; we fold TD into name for now

		uint16_t nameIdx = addName(n.name.empty() ? "__tuple_destructure__" : n.name);
		OpCode op = n.typeDescriptor.isConst ? OpCode::OP_DEFINE_CONST : OpCode::OP_DEFINE_VAR;
		current_->emitU16(op, nameIdx, ln);
	}

	void BytecodeCompiler::visit(FunctionExpression& n) {
		if (n.type == ExpressionType::FunctionDef) {
			// Function definitions are handled by the interpreter at parse time;
			// in bytecode mode they were already registered in scope.  Nothing to emit.
			return;
		}
		// FunctionCall
		compileFunctionCall(n);
	}

	void BytecodeCompiler::compileFunctionCall(FunctionExpression& n) {
		int ln = resolveLine(&n);

		// Resolve callee function value.
		if (!n.function) {
			current_->emit(OpCode::OP_PUSH_NULL, ln);
			return;
		}

		auto fnVal = n.function;
		if (fnVal->getType() == Type::Function) {
			auto fnc = fnVal->getFunction();
			if (!fnc) {
				current_->emit(OpCode::OP_PUSH_NULL, ln); return;
			}
			const auto& name = fnc->name;

			// Short-circuit && / ||
			if (name == "&&" && n.subexpressions.size() == 2) {
				compile(n.subexpressions[0]);
				auto j = current_->emitJump(OpCode::OP_AND_SHORT, ln);
				compile(n.subexpressions[1]);
				current_->patchJump(j);
				return;
			}
			if (name == "||" && n.subexpressions.size() == 2) {
				compile(n.subexpressions[0]);
				auto j = current_->emitJump(OpCode::OP_OR_SHORT, ln);
				compile(n.subexpressions[1]);
				current_->patchJump(j);
				return;
			}

			// Compile args
			uint8_t argc = 0;
			for (auto* arg : n.subexpressions) {
				compile(arg);
				++argc;
			}

			// Choose opcode based on function type
			uint16_t nameIdx = addName(name);
			if (fnc->getBodyType() == FunctionBodyType::Lambda
				|| fnc->getBodyType() == FunctionBodyType::ScopedLambda
				|| fnc->getBodyType() == FunctionBodyType::ClassLambda) {
				// C++ builtin — dispatch by name
				current_->emitU16(OpCode::OP_CALL_BUILTIN, nameIdx, ln);
				current_->emitByte(argc, ln);
			}
			else if (fnc->opPrecedence != OperatorPrecedence::func) {
				// Operator
				current_->emitU16(OpCode::OP_CALL_OPERATOR, nameIdx, ln);
				current_->emitByte(argc, ln);
			}
			else {
				// Script-defined or bytecode function
				uint16_t constIdx = addConst(fnVal);
				current_->emitU16(OpCode::OP_PUSH_CONST, constIdx, ln);
				current_->emitU8(OpCode::OP_CALL, argc, ln);
			}
		}
		else {
			// Non-function value call (e.g. coro task invocation)
			uint16_t constIdx = addConst(fnVal);
			current_->emitU16(OpCode::OP_PUSH_CONST, constIdx, ln);
			uint8_t argc = 0;
			for (auto* arg : n.subexpressions) {
				compile(arg); ++argc;
			}
			current_->emitU8(OpCode::OP_CALL, argc, ln);
		}
	}

	void BytecodeCompiler::visit(Return& n) {
		int ln = resolveLine(&n);
		if (n.expression) {
			compile(n.expression);
			current_->emit(OpCode::OP_RETURN, ln);
		}
		else {
			current_->emit(OpCode::OP_RETURN_NIL, ln);
		}
	}

	void BytecodeCompiler::visit(Yield& n) {
		int ln = resolveLine(&n);
		if (n.expression) compile(n.expression);
		else current_->emit(OpCode::OP_PUSH_NULL, ln);
		current_->emit(OpCode::OP_YELD, ln);
	}

	void BytecodeCompiler::visit(Continue& n) {
		int ln = resolveLine(&n);
		if (!loopStack_.empty()) {
			// Jump back to the loop condition (will be patched to loopStart).
			auto j = current_->emitJump(OpCode::OP_JUMP, ln);
			loopStack_.back().continueJumps.push_back(j);
		}
	}

	void BytecodeCompiler::visit(Break& n) {
		int ln = resolveLine(&n);
		if (!loopStack_.empty()) {
			auto j = current_->emitJump(OpCode::OP_JUMP, ln);
			loopStack_.back().breakJumps.push_back(j);
		}
	}

	void BytecodeCompiler::visit(IfElse& n) {
		int ln = resolveLine(&n);
		std::vector<size_t> endJumps;
		for (size_t i = 0; i < n.states.size(); ++i) {
			auto& branch = n.states[i];
			size_t falseJump = SIZE_MAX;
			if (branch.testExpression) {
				compile(branch.testExpression);
				falseJump = current_->emitJump(OpCode::OP_JUMP_IF_FALSE, ln);
			}
			compileBlock(branch.subexpressions);
			if (i + 1 < n.states.size()) {
				endJumps.push_back(current_->emitJump(OpCode::OP_JUMP, ln));
			}
			if (falseJump != SIZE_MAX) current_->patchJump(falseJump);
		}
		for (auto j : endJumps) current_->patchJump(j);
	}

	void BytecodeCompiler::visit(Loop& n) {
		int ln = resolveLine(&n);

		if (n.initExpression) {
			compile(n.initExpression);
			current_->emit(OpCode::OP_POP, ln);
		}

		size_t loopStart = current_->currentOffset();
		pushLoopContext(loopStart);

		size_t exitJump = SIZE_MAX;
		if (n.testExpression) {
			compile(n.testExpression);
			exitJump = current_->emitJump(OpCode::OP_JUMP_IF_FALSE, ln);
		}

		compileBlock(n.subexpressions);

		// Patch continue jumps to here (before iterate expression)
		patchContinues();

		if (n.iterateExpression) {
			compile(n.iterateExpression);
			current_->emit(OpCode::OP_POP, ln);
		}

		current_->emitLoop(loopStart, OpCode::OP_JUMP, ln);

		if (exitJump != SIZE_MAX) current_->patchJump(exitJump);
		patchBreaks();
		popLoopContext();
	}

	void BytecodeCompiler::visit(Foreach& n) {
		int ln = resolveLine(&n);
		// Emit list expression, then call a runtime FOREACH helper.
		// The VM handles iteration natively for List/Range/Tuple types.
		compile(n.listExpression);

		// Push iterate variable names as constants so the VM knows what to bind.
		for (auto& name : n.iterateNames) {
			auto sv = interpreter_->makeValue(name);
			uint16_t idx = addConst(sv);
			current_->emitU16(OpCode::OP_PUSH_CONST, idx, ln);
		}
		uint16_t nameCount = static_cast<uint16_t>(n.iterateNames.size());
		current_->emitU16(OpCode::OP_MAKE_LIST, nameCount, ln);

		// Compile body as a nested BytecodeFunctionRef stored as a constant.
		BytecodeCompiler bodyCompiler(interpreter_);
		auto bodyFn = bodyCompiler.compileStatements(n.subexpressions, "__foreach_body__");
		auto bodyFnVal = interpreter_->makeValue(std::make_shared<Function>("__foreach_body__"));
		bodyFnVal->getFunction()->body = bodyFn;
		uint16_t bodyIdx = addConst(bodyFnVal);
		current_->emitU16(OpCode::OP_PUSH_CONST, bodyIdx, ln);

		// OP_CALL_BUILTIN "__foreach__" with 3 args: [iterable, names, body]
		uint16_t nameIdx = addName("__foreach__");
		current_->emitU16(OpCode::OP_CALL_BUILTIN, nameIdx, ln);
		current_->emitByte(3, ln);
	}

	void BytecodeCompiler::visit(MemberVariable& n) {
		int ln = resolveLine(&n);
		compile(n.object);
		uint16_t nameIdx = addName(n.name);
		current_->emitU16(OpCode::OP_GET_MEMBER, nameIdx, ln);
	}

	void BytecodeCompiler::visit(MemberFunctionCall& n) {
		int ln = resolveLine(&n);
		compile(n.object);
		for (auto* arg : n.subexpressions) compile(arg);
		uint16_t nameIdx = addName(n.functionName);
		current_->emitU16(OpCode::OP_CALL_METHOD, nameIdx, ln);
		current_->emitByte(static_cast<uint8_t>(n.subexpressions.size()), ln);
	}

	void BytecodeCompiler::visit(MatchExpression& n) {
		int ln = resolveLine(&n);
		compile(n.target);

		std::vector<size_t> endJumps;
		for (size_t i = 0; i < n.arms.size(); ++i) {
			auto& arm = n.arms[i];
			size_t falseJump = SIZE_MAX;
			if (arm.pattern) {
				current_->emit(OpCode::OP_DUP, ln);
				compile(arm.pattern);
				current_->emitU16(OpCode::OP_CALL_OPERATOR, addName("=="), ln);
				current_->emitByte(2, ln);
				falseJump = current_->emitJump(OpCode::OP_JUMP_IF_FALSE, ln);
			}
			// Pop the scrutinee before executing the arm body.
			current_->emit(OpCode::OP_POP, ln);
			compileBlock(arm.body);
			endJumps.push_back(current_->emitJump(OpCode::OP_JUMP, ln));
			if (falseJump != SIZE_MAX) current_->patchJump(falseJump);
		}
		// No arm matched — pop scrutinee, push null.
		current_->emit(OpCode::OP_POP, ln);
		current_->emit(OpCode::OP_PUSH_NULL, ln);
		for (auto j : endJumps) current_->patchJump(j);
	}

	void BytecodeCompiler::visit(TupleLiteralExpression& n) {
		int ln = resolveLine(&n);
		for (auto* elem : n.elements) compile(elem);
		current_->emitU16(OpCode::OP_MAKE_TUPLE, static_cast<uint16_t>(n.elements.size()), ln);
	}

	void BytecodeCompiler::visit(DestructuringAssign& n) {
		int ln = resolveLine(&n);
		compile(n.valueExpression);
		// Each name in pattern gets a SET_GLOBAL after extracting element.
		for (size_t i = 0; i < n.patternNames.size(); ++i) {
			if (n.patternNames[i] == "_") continue;
			current_->emit(OpCode::OP_DUP, ln);
			auto idxVal = interpreter_->makeValue(static_cast<Int>(i));
			uint16_t idxConst = addConst(idxVal);
			current_->emitU16(OpCode::OP_PUSH_CONST, idxConst, ln);
			current_->emit(OpCode::OP_GET_INDEX, ln);
			uint16_t nameIdx = addName(n.patternNames[i]);
			current_->emitU16(OpCode::OP_SET_GLOBAL, nameIdx, ln);
		}
		current_->emit(OpCode::OP_POP, ln); // pop the tuple
	}

	void BytecodeCompiler::visit(BlockExpression& n) {
		compileBlock(n.subexpressions);
	}

	void BytecodeCompiler::visit(SafeBlockExpression& n) {
		int ln = resolveLine(&n);
		current_->emit(OpCode::OP_ENTER_TX, ln);
		compileBlock(n.subexpressions);
		current_->emitU8(OpCode::OP_EXIT_TX,
			n.producesValue ? 1 : 0, ln);
	}

	void BytecodeCompiler::visit(DeferExpression& n) {
		int ln = resolveLine(&n);
		// Compile defer body as a nested function stored in the constant pool.
		BytecodeCompiler bodyCompiler(interpreter_);
		auto bodyFn = bodyCompiler.compileStatements(n.subexpressions, "__defer_body__");
		auto bodyFnVal = interpreter_->makeValue(std::make_shared<Function>("__defer_body__"));
		bodyFnVal->getFunction()->body = bodyFn;
		uint16_t idx = addConst(bodyFnVal);
		current_->emitU16(OpCode::OP_DEFER_REGISTER, idx, ln);
	}

	void BytecodeCompiler::visit(LiveRebind& n) {
		int ln = resolveLine(&n);
		if (n.guardExpr) compile(n.guardExpr);
		else current_->emit(OpCode::OP_PUSH_NULL, ln);
		uint16_t nameIdx = addName(n.targetName);
		// Live rebind dispatches through SET_GLOBAL with a special runtime flag;
		// for now we fall back to a named builtin call that the VM handles.
		current_->emitU16(OpCode::OP_CALL_BUILTIN, addName("__live_rebind__"), ln);
		current_->emitByte(2, ln); // [value, name]
		// Push name before the call.
		auto nameVal = interpreter_->makeValue(n.targetName);
		uint16_t nameConstIdx = addConst(nameVal);
		(void)nameConstIdx; // ordering handled by VM; simplified here
		current_->emitU16(OpCode::OP_PUSH_CONST, nameConstIdx, ln);
		current_->emitU16(OpCode::OP_SET_GLOBAL, nameIdx, ln);
	}

	void BytecodeCompiler::visit(AwaitExpression& n) {
		int ln = resolveLine(&n);
		compile(n.taskExpr);
		current_->emit(OpCode::OP_AWAIT, ln);
	}

	void BytecodeCompiler::visit(SpawnExpression& n) {
		int ln = resolveLine(&n);
		if (n.callExpr) {
			compile(n.callExpr);
			current_->emitU8(OpCode::OP_SPAWN, 0, ln);
		}
		else {
			// Block form — compile body as a nested function.
			BytecodeCompiler bodyCompiler(interpreter_);
			auto bodyFn = bodyCompiler.compileStatements(n.subexpressions, "__spawn_body__");
			auto bodyFnVal = interpreter_->makeValue(std::make_shared<Function>("__spawn_body__"));
			bodyFnVal->getFunction()->body = bodyFn;
			uint16_t idx = addConst(bodyFnVal);
			current_->emitU16(OpCode::OP_PUSH_CONST, idx, ln);
			current_->emitU8(OpCode::OP_SPAWN, 0, ln);
		}
	}

	void BytecodeCompiler::visit(SyncBlockExpression& n) {
		int ln = resolveLine(&n);
		// Compile each statement; collect task results into tuple.
		for (auto* stmt : n.subexpressions) compile(stmt);
		current_->emitU16(OpCode::OP_MAKE_TUPLE,
			static_cast<uint16_t>(n.subexpressions.size()), ln);
		uint16_t nameIdx = addName("__sync__");
		current_->emitU16(OpCode::OP_CALL_BUILTIN, nameIdx, ln);
		current_->emitByte(1, ln);
	}

	void BytecodeCompiler::visit(RaceBlockExpression& n) {
		int ln = resolveLine(&n);
		for (auto* stmt : n.subexpressions) compile(stmt);
		current_->emitU16(OpCode::OP_MAKE_TUPLE,
			static_cast<uint16_t>(n.subexpressions.size()), ln);
		uint16_t nameIdx = addName("__race__");
		current_->emitU16(OpCode::OP_CALL_BUILTIN, nameIdx, ln);
		current_->emitByte(1, ln);
	}

	void BytecodeCompiler::visit(BranchBlockExpression& n) {
		int ln = resolveLine(&n);
		BytecodeCompiler bodyCompiler(interpreter_);
		auto bodyFn = bodyCompiler.compileStatements(n.subexpressions, "__branch_body__");
		auto bodyFnVal = interpreter_->makeValue(std::make_shared<Function>("__branch_body__"));
		bodyFnVal->getFunction()->body = bodyFn;
		uint16_t idx = addConst(bodyFnVal);
		current_->emitU16(OpCode::OP_PUSH_CONST, idx, ln);
		current_->emitU8(OpCode::OP_SPAWN, 0, ln);
		current_->emit(OpCode::OP_POP, ln); // fire-and-forget
	}

	void BytecodeCompiler::visit(NamedArgumentExpression& n) {
		// Named args aren't directly compiled; they're unpacked by compileFunctionCall.
		compile(n.expression);
	}

} // namespace IkigaiScript
