#include "codegen.hpp"
#include <format>

#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) mGeneratedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) mGeneratedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) mGeneratedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) emitInstr1op(jmp, label)
#define ret() mGeneratedCode += "\tret\n"
#define cqo() mGeneratedCode += "\tcqo\n"
#define syscall() mGeneratedCode += "\tsyscall\n"
#define mov(d, s) emitInstr2op("mov", d, s)
#define movq(d, s) emitInstr2op("movq", d, s)
#define movsd(d, s) emitInstr2op("movsd", d, s)
#define movzx(d, s) emitInstr2op("movzx", d, s)
#define strDirective(s) std::format("db \"{}\", 10", s)
#define memDirective(d, n) std::format("{} {}", d, n)

#define stack_alloc(size) \
    if (size > 0) { \
        emitInstr2op("sub", "rsp", size); \
        mStackAllocator.alloc(size); \
    }

#define stack_dealloc(size) \
    if (size > 0) { \
        emitInstr2op("add", "rsp", size); \
        mStackAllocator.dealloc(size); \
    }

#define push(v) \
    emitInstr1op("push", v); \
    mStackAllocator.alloc(8);

#define pop(rn) \
    emitInstr1op("pop", rn); \
    mStackAllocator.dealloc(8);

#define emitSet8L(op, reg) \
    emitInstr1op(op, getRegName(reg, REG8L)); \
    movzx(getRegName(reg, REG64), getRegName(reg, REG8L));

#define regAlloc() ([&]() { \
    auto* reg = mRegisterAllocator.alloc(); \
    if (reg && isPRESERVED(reg->rType)) { \
        push(getRegName(reg, REG64)) \
    } \
    return reg; \
    }())

#define regFree(reg) \
    if (reg) { \
        mRegisterAllocator.free(reg); \
        if (isPRESERVED(reg->rType)) { \
            pop(getRegName(reg, REG64)) \
        } \
    }

CodeGen::CodeGen()
	: mCurrentScope("main") {
}

std::string CodeGen::emit(const ExprPtr& ast) {
	mGeneratedCode =
			"[bits 64]\n"
			"section .text\n"
			"\tglobal _start\n"
			"_start:\n";

	push("rbp")
	mov("rbp", "rsp");

	auto next = ast;
	while (next != nullptr) {
		auto* reg = emitAST(next);
		regFree(reg)
		next = next->child;
	}

	pop("rbp")

#if defined(__APPLE__) || defined(__MACH__)
	mov("rax", emitHex(0x2000001));
#elif defined(__linux__)
	mov("rax", 60);
#else
	throw std::runtime_error("Unsupported Operating System");
#endif

	emitInstr2op("xor", "rdi", "rdi");
	syscall();

	// Function definitions
	for (const auto& [func, defun]: mFunctions) {
		(this->*func)(defun);
	}
	// Sections
	for (const auto& [section, data]: mSections) {
		mGeneratedCode += section;

		for (const auto& [name, size]: data) {
			mGeneratedCode += std::format("{}: {}\n", name, size);
		}
	}

	return mGeneratedCode;
}

Register* CodeGen::emitAST(const ExprPtr& ast) {
	if (const auto binop = cast::toBinop(ast)) {
		return emitBinop(*binop);
	}
	if (const auto dotimes = cast::toDotimes(ast)) {
		return emitDotimes(*dotimes);
	}
	if (const auto loop = cast::toLoop(ast)) {
		return emitLoop(*loop);
	}
	if (const auto let = cast::toLet(ast)) {
		return emitLet(*let);
	}
	if (const auto setq = cast::toSetq(ast)) {
		emitSetq(*setq);
	} else if (const auto defvar = cast::toDefvar(ast)) {
		emitDefvar(*defvar);
	} else if (const auto defconst = cast::toDefconstant(ast)) {
		emitDefconst(*defconst);
	} else if (const auto defun = cast::toDefun(ast)) {
		mFunctions.emplace_back(&CodeGen::emitDefun, *defun);
	} else if (const auto funcCall = cast::toFuncCall(ast)) {
		return emitFuncCall(*funcCall);
	} else if (const auto if_ = cast::toIf(ast)) {
		return emitIf(*if_);
	} else if (const auto when = cast::toWhen(ast)) {
		return emitWhen(*when);
	} else if (const auto cond = cast::toCond(ast)) {
		return emitCond(*cond);
	} else if (cast::toInt(ast) || cast::toDouble(ast) || cast::toVar(ast)) {
		return emitPrimitive(ast);
	}

	return nullptr;
}

Register* CodeGen::emitBinop(const BinOpExpr& binop) {
	switch (binop.opToken.type) {
		case TokenType::PLUS:
			return emitExpr(binop.lhs, binop.rhs, {.op = "add", .opSSE = "addsd"});
		case TokenType::MINUS:
			return emitExpr(binop.lhs, binop.rhs, {.op = "sub", .opSSE = "subsd"});
		case TokenType::DIV:
			return emitExpr(binop.lhs, binop.rhs, {.op = "idiv", .opSSE = "divsd"});
		case TokenType::MUL:
			return emitExpr(binop.lhs, binop.rhs, {.op = "imul", .opSSE = "mulsd"});
		case TokenType::LOGAND:
			return emitExpr(binop.lhs, binop.rhs, {.op = "and", .opSSE = ""});
		case TokenType::LOGIOR:
			return emitExpr(binop.lhs, binop.rhs, {.op = "or", .opSSE = ""});
		case TokenType::LOGXOR:
			return emitExpr(binop.lhs, binop.rhs, {.op = "xor", .opSSE = ""});
		case TokenType::LOGNOR: {
			const ExprPtr negOne = std::make_shared<IntExpr>(-1);
			// Bitwise NOT seperately
			Register* regLhs = emitExpr(binop.lhs, negOne, {.op = "xor", .opSSE = ""});
			Register* regRhs = emitExpr(binop.rhs, negOne, {.op = "xor", .opSSE = ""});
			emitInstr2op("and", getRegName(regLhs, REG64), getRegName(regRhs, REG64));
			regFree(regRhs)
			return regLhs;
		}
		case TokenType::NOT:
			return emitCmpZero(binop.lhs);
		case TokenType::EQUAL:
		case TokenType::NEQUAL:
		case TokenType::GREATER_THEN:
		case TokenType::LESS_THEN:
		case TokenType::GREATER_THEN_EQ:
		case TokenType::LESS_THEN_EQ:
		case TokenType::AND:
		case TokenType::OR:
			return emitExpr(binop.lhs, binop.rhs, {.op = "cmp", .opSSE = "ucomisd"});
		default:
			return nullptr;
	}
}

Register* CodeGen::emitDotimes(const DotimesExpr& dotimes) {
	const auto iterVar = cast::toVar(dotimes.iterationCount);
	const std::string iterVarName = cast::toString(iterVar->name)->data;
	// Labels
	const std::string loopLabel = createLabel();
	const std::string doneLabel = createLabel();
	// Loop condition
	ExprPtr name = iterVar->name;
	ExprPtr value = std::make_shared<IntExpr>(0);
	ExprPtr lhs = std::make_shared<VarExpr>(name, value, SymbolType::LOCAL);
	cast::toVar(lhs)->vType = iterVar->vType;

	ExprPtr rhs = iterVar->value;
	auto token = Token{TokenType::LESS_THEN};
	ExprPtr test = std::make_shared<BinOpExpr>(lhs, rhs, token);
	// Address of iter var
	stack_alloc(mMemorySizeInBytes[REG64])
	std::string iterVarAddr = getAddr(iterVarName, SymbolType::LOCAL, REG64);
	// Set 0 to iter var
	mov(iterVarAddr, 0);
	// Loop label
	emitLabel(loopLabel);
	emitTest(test, std::string(), doneLabel);
	// Emit statements
	Register* reg = nullptr;
	for (const auto& statement: dotimes.statements) {
		reg = emitAST(statement);
		regFree(reg)
	}
	// Increment iteration count
	reg = regAlloc();
	auto regStr = getRegName(reg, REG64);

	mov(regStr, iterVarAddr);
	emitInstr2op("add", regStr, 1);
	mov(iterVarAddr, regStr);

	regFree(reg)

	emitJump("jmp", loopLabel);
	emitLabel(doneLabel);

	stack_dealloc(mMemorySizeInBytes[REG64])

	return reg;
}

Register* CodeGen::emitLoop(const LoopExpr& loop) {
	Register* reg = nullptr;
	// Labels
	std::string loopLabel = createLabel();
	std::string doneLabel = createLabel();

	emitLabel(loopLabel);

	bool hasReturn{false};
	for (auto& sexpr: loop.sexprs) {
		const auto when = cast::toWhen(sexpr);
		if (!when) {
			reg = emitAST(sexpr);
			regFree(reg)
			continue;
		}

		for (auto& form: when->then) {
			if (const auto return_ = cast::toReturn(form); !return_) {
				reg = emitAST(form);
				regFree(reg)
				continue;
			}

			emitTest(when->test, "", loopLabel);
			emitJump("jmp", doneLabel);
			hasReturn = true;
			break;
		}

		if (!hasReturn)
			emitJump("jmp", loopLabel);
	}
	emitLabel(doneLabel);

	return reg;
}

Register* CodeGen::emitLet(const LetExpr& let) {
	Register* reg = nullptr;
	uint32_t requiredStackMem = 0;

	for (const auto& var: let.bindings) {
		const int size = mMemorySizeInBytes[getMemSize(var)];
		requiredStackMem += size;
	}

	stack_alloc(requiredStackMem)

	for (const auto& var: let.bindings) {
		const uint32_t memSize = getMemSize(var);
		handleAssignment(var, memSize);
	}

	for (const auto& sexpr: let.body) {
		reg = emitAST(sexpr);
		regFree(reg)
	}

	stack_dealloc(requiredStackMem)

	return reg;
}

void CodeGen::emitSetq(const SetqExpr& setq) {
	const uint32_t memSize = getMemSize(setq.pair);
	handleAssignment(setq.pair, memSize);
}

void CodeGen::emitDefvar(const DefvarExpr& defvar) {
	emitSection(defvar.pair);
}

void CodeGen::emitDefconst(const DefconstExpr& defconst) {
	emitSection(defconst.pair, true);
}

void CodeGen::emitDefun(const DefunExpr& defun) {
	const auto func = cast::toVar(defun.name);
	mCurrentScope = cast::toString(func->name)->data;

	emitLabel("\n" + mCurrentScope);
	push("rbp")
	mov("rbp", "rsp");

	uint32_t stackSize{0};
	int32_t scratchIdx{0};
	int32_t sseIdx{0};
	for (auto& arg: defun.args) {
		const auto param = cast::toVar(arg);
		const std::string paramName = cast::toString(param->name)->data;

		if (param->vType == VarType::INT) {
			if (scratchIdx > 5)
				continue;
			scratchIdx++;
		} else if (param->vType == VarType::DOUBLE) {
			if (sseIdx > 7)
				continue;
			sseIdx++;
		}

		stackSize += mMemorySizeInBytes[getMemSize(arg)];
		mStackAllocator.pushStackFrame(mCurrentScope, paramName, param->sType);
	}

	stack_alloc(stackSize)

	scratchIdx = 0, sseIdx = 0;
	for (const auto& arg: defun.args) {
		const auto param = cast::toVar(arg);
		const std::string paramName = cast::toString(param->name)->data;

		if ((param->vType == VarType::INT && scratchIdx > 5) || (param->vType == VarType::DOUBLE && sseIdx > 7)) {
			continue;
		}

		mov(getAddr(paramName, param->sType, REG64),
		    getRegNameByID(param->vType == VarType::INT
			    ? mParamRegisters[scratchIdx++]
			    : mParamRegistersSSE[sseIdx++], REG64));
	}

	Register* reg = nullptr;
	for (const auto& form: defun.forms) {
		if (form == defun.forms.back() && cast::toBinop(form)) {
			reg = emitSet(form);
		} else {
			reg = emitAST(form);
		}
	}

	if (reg && isSSE(reg->rType)) {
		movsd("xmm0", getRegName(reg, REG64));
	} else if (reg && !isSSE(reg->rType)) {
		mov("rax", getRegName(reg, REG64));
	}

	regFree(reg)
	stack_dealloc(stackSize)
	pop("rbp")
	ret();
}

Register* CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {
	const auto func = cast::toVar(funcCall.name);
	const std::string funcName = cast::toString(func->name)->data;

	// Calculate the proper stack size before function call
	uint32_t stackAlignedSize = mStackAllocator.calculateRequiredStackSize(funcCall.args);
	stack_alloc(stackAlignedSize)

	Register* reg;
	int32_t scratchIdx{0};
	int32_t sseIdx{0};
	int32_t stackIdx{0};
	for (const auto& arg: funcCall.args) {
		const auto param = cast::toVar(arg);

		// If scratch param size > 5 or sse param size > 7, push the params onto stack
		if ((scratchIdx > 5 && param->vType == VarType::INT) || (sseIdx > 7 && param->vType == VarType::DOUBLE)) {
			pushParamOntoStack(funcName, *param, stackIdx);
			continue;
		}
		// Push parameter to the appropriate register
		if (const auto innerVar = cast::toVar(param->value)) {
			const std::string paramName = cast::toString(innerVar->name)->data;
			pushParamToRegister(param->vType == VarType::INT
				                    ? mParamRegisters[scratchIdx++]
				                    : mParamRegistersSSE[sseIdx++],
			                    getAddr(paramName, innerVar->sType, REG64).c_str());
		} else if (const auto binop = cast::toBinop(param->value)) {
			reg = emitBinop(*binop);
			pushParamToRegister(isSSE(reg->rType)
				                    ? mParamRegistersSSE[sseIdx++]
				                    : mParamRegisters[scratchIdx++],
			                    getRegName(reg, REG64));
			regFree(reg)
		} else if (const auto fc = cast::toFuncCall(param->value)) {
			reg = emitFuncCall(*fc);

			pushParamToRegister(isSSE(reg->rType)
				                    ? mParamRegistersSSE[sseIdx++]
				                    : mParamRegisters[scratchIdx++],
			                    getRegName(reg, REG64));
			regFree(reg)
		} else {
			if (param->vType == VarType::INT) {
				pushParamToRegister(mParamRegisters[scratchIdx++], cast::toInt(param->value)->n);
			} else if (param->vType == VarType::DOUBLE) {
				pushParamToRegister(mParamRegistersSSE[sseIdx++], cast::toDouble(param->value)->n);
			}
		}
	}

	emitInstr1op("call", funcName);

	if (cast::toDouble(funcCall.returnType)) {
		reg = mRegisterAllocator.alloc(SSE);
		movsd(getRegName(reg, REG64), "xmm0");
	} else {
		reg = regAlloc();
		mov(getRegName(reg, REG64), "rax");
	}

	stack_dealloc(stackAlignedSize)

	return reg;
}

Register* CodeGen::emitIf(const IfExpr& if_) {
	const std::string trueLabel = createLabel();
	const std::string elseLabel = createLabel();
	// Emit test
	emitTest(if_.test, trueLabel, elseLabel);
	// Emit then
	Register* reg = nullptr;
	reg = emitAST(if_.then);
	// Emit else
	if (!cast::toUninitialized(if_.else_)) {
		std::string done = createLabel();
		emitJump("jmp", done);
		emitLabel(elseLabel);

		regFree(reg)
		reg = emitAST(if_.else_);
		emitLabel(done);
	} else {
		emitLabel(elseLabel);
	}

	regFree(reg)
	return reg;
}

Register* CodeGen::emitWhen(const WhenExpr& when) {
	const std::string doneLabel = createLabel();
	// Emit test
	emitTest(when.test, std::string(), doneLabel);
	// Emit then
	Register* reg = nullptr;
	for (const auto& form: when.then) {
		reg = emitAST(form);
		regFree(reg)
	}
	emitLabel(doneLabel);

	return reg;
}

Register* CodeGen::emitCond(const CondExpr& cond) {
	const std::string done = createLabel();

	Register* reg = nullptr;
	for (const auto& [test, forms]: cond.variants) {
		const std::string elseLabel = createLabel();
		emitTest(test, std::string(), elseLabel);

		for (const auto& form: forms) {
			reg = emitAST(form);
			regFree(reg)
		}

		emitJump("jmp", done);
		emitLabel(elseLabel);
	}
	emitLabel(done);

	return reg;
}

Register* CodeGen::emitPrimitive(const ExprPtr& prim) {
	if (const auto int_ = cast::toInt(prim)) {
		return emitInt(*int_);
	}

	if (const auto double_ = cast::toDouble(prim)) {
		return emitDouble(*double_);
	}

	if (const auto var = cast::toVar(prim)) {
		const std::string varName = cast::toString(var->name)->data;

		Register* reg = regAlloc();
		mov(getRegName(reg, REG64), getAddr(varName, var->sType, REG64));

		return reg;
	}

	return nullptr;
}

Register* CodeGen::emitInt(const IntExpr& int_) {
	Register* reg = regAlloc();
	mov(getRegName(reg, REG64), int_.n);
	return reg;
}

Register* CodeGen::emitDouble(const DoubleExpr& double_) {
	Register* reg = regAlloc();
	auto regStr = getRegName(reg, REG64);

	Register* regSSE = mRegisterAllocator.alloc(SSE);

	uint64_t hex = *reinterpret_cast<const uint64_t*>(&double_.n);

	mov(regStr, emitHex(hex));
	movq(getRegName(regSSE, REG64), regStr);

	regFree(reg)

	return regSSE;
}

Register* CodeGen::emitNumb(const ExprPtr& n) {
	if (const auto int_ = cast::toInt(n)) {
		return emitInt(*int_);
	}

	if (const auto double_ = cast::toDouble(n)) {
		return emitDouble(*double_);
	}

	const auto var = cast::toVar(n);
	return emitLoadRegFromMem(*var, REG64);
}

Register* CodeGen::emitNode(const ExprPtr& node) {
	if (const auto binOp = cast::toBinop(node)) {
		return emitBinop(*binOp);
	}

	if (const auto funcCall = cast::toFuncCall(node)) {
		return emitFuncCall(*funcCall);
	}

	return emitNumb(node);
}

Register* CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, OpcodePair opcode) {
	Register* regLhs = emitNode(lhs);
	Register* regRhs = emitNode(rhs);

	if (isSSE(regLhs->rType) && !isSSE(regRhs->rType)) {
		Register* newReg = mRegisterAllocator.alloc(SSE);
		auto newRegStr = getRegName(newReg, REG64);

		emitInstr2op("cvtsi2sd", newRegStr, getRegName(regRhs, REG64));
		regFree(regRhs)

		emitInstr2op(opcode.opSSE, getRegName(regLhs, REG64), newRegStr);
		regFree(newReg);

		return regLhs;
	}

	if (!isSSE(regLhs->rType) && isSSE(regRhs->rType)) {
		Register* newReg = mRegisterAllocator.alloc(SSE);
		auto newRegStr = getRegName(newReg, REG64);
		auto regRhsStr = getRegName(regRhs, REG64);

		emitInstr2op("cvtsi2sd", newRegStr, getRegName(regLhs, REG64));
		regFree(regLhs)

		emitInstr2op(opcode.opSSE, newRegStr, regRhsStr);
		movsd(regRhsStr, newRegStr);
		regFree(newReg);
		return regRhs;
	}

	if (isSSE(regLhs->rType) && isSSE(regRhs->rType)) {
		emitInstr2op(opcode.opSSE, getRegName(regLhs, REG64), getRegName(regRhs, REG64));
		regFree(regRhs);
		return regLhs;
	}

	// rax -> dividend
	// idiv divisor[register/memory]
	if (opcode.op == "idiv") {
		mov("rax", getRegName(regLhs, REG64));
		cqo();
		emitInstr1op("idiv", getRegName(regRhs, REG64));
		mov(getRegName(regLhs, REG64), "rax");
	} else {
		emitInstr2op(opcode.op, getRegName(regLhs, REG64), getRegName(regRhs, REG64));
	}

	regFree(regRhs);
	return regLhs;
}

void CodeGen::emitSection(const ExprPtr& var, const bool isConstant) {
	const auto var_ = cast::toVar(var);

	if (cast::toBinop(var_->value) || cast::toFuncCall(var_->value)) {
		updateSections("\nsection .bss\n", {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeUninitialized[REG64], 1)});

		handleAssignment(var, REG64);
	} else if (cast::toUninitialized(var_->value)) {
		updateSections("\nsection .bss\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeUninitialized[REG64], 1)});
	} else if (cast::toNIL(var_->value)) {
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {.name = cast::toString(var_->name)->data,
		                              .data = memDirective(mDataSizeInitialized[REG8L], 0)});
	} else if (cast::toT(var_->value)) {
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {.name = cast::toString(var_->name)->data,
		                              .data = memDirective(mDataSizeInitialized[REG8L], 1)});
	} else if (const auto int_ = cast::toInt(var_->value)) {
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {.name = cast::toString(var_->name)->data,
		                              .data = memDirective(mDataSizeInitialized[REG64], int_->n)});
	} else if (const auto double_ = cast::toDouble(var_->value)) {
		uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {.name = cast::toString(var_->name)->data,
		                              .data = memDirective(mDataSizeInitialized[REG64], emitHex(hex))});
	} else if (cast::toVar(var_->value)) {
		const uint32_t memSize = getMemSize(var_);

		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {.name = cast::toString(var_->name)->data,
		                              .data = memDirective(mDataSizeInitialized[memSize], 0)});
		handleAssignment(var, memSize);
	} else if (const auto str = cast::toString(var_->value)) {
		updateSections("\nsection .rodata\n",
		               {.name = cast::toString(var_->name)->data,
		                              .data = strDirective(str->data)});
	}
}

void CodeGen::emitTest(const ExprPtr& test, const std::string& trueLabel, const std::string& elseLabel) {
	Register* reg;

	if (const auto binop = cast::toBinop(test)) {
		switch (binop->opToken.type) {
			case TokenType::PLUS:
			case TokenType::MINUS:
			case TokenType::DIV:
			case TokenType::MUL:
			case TokenType::LOGAND:
			case TokenType::LOGIOR:
			case TokenType::LOGXOR:
			case TokenType::LOGNOR: {
				reg = emitBinop(*binop);
				emitInstr2op((isSSE(reg->rType) ? "ucomisd" : "cmp"), getRegName(reg, REG64), 0);
				emitJump("je", elseLabel);
				regFree(reg)
				break;
			}
			case TokenType::EQUAL:
			case TokenType::NOT:
				reg = emitBinop(*binop);
				emitJump("jne", elseLabel);
				regFree(reg)
				break;
			case TokenType::NEQUAL:
				reg = emitBinop(*binop);
				emitJump("je", elseLabel);
				regFree(reg)
				break;
			case TokenType::GREATER_THEN:
				reg = emitBinop(*binop);
				emitJump("jle", elseLabel);
				regFree(reg)
				break;
			case TokenType::LESS_THEN:
				reg = emitBinop(*binop);
				emitJump("jge", elseLabel);
				regFree(reg)
				break;
			case TokenType::GREATER_THEN_EQ:
				reg = emitBinop(*binop);
				emitJump("jl", elseLabel);
				regFree(reg)
				break;
			case TokenType::LESS_THEN_EQ:
				reg = emitBinop(*binop);
				emitJump("jg", elseLabel);
				regFree(reg)
				break;
			case TokenType::AND: {
				auto andComp = [&](const ExprPtr& node) {
					if (isPrimitive(node)) {
						Register* regLhs = emitCmpZero(node);
						emitJump("je", elseLabel);
						regFree(regLhs)
					} else {
						emitTest(node, trueLabel, elseLabel);
					}
				};

				andComp(binop->lhs);
				andComp(binop->rhs);
				break;
			}
			case TokenType::OR: {
				if (isPrimitive(binop->lhs)) {
					Register* regLhs = emitCmpZero(binop->lhs);
					emitJump("jne", trueLabel);
					regFree(regLhs)
				} else if (const auto bop = cast::toBinop(binop->lhs)) {
					reg = emitBinop(*bop);
					emitJmpTrueLabel(reg, bop->opToken.type, trueLabel);
					regFree(reg)
				} else {
					emitTest(binop->lhs, trueLabel, elseLabel);
				}

				if (isPrimitive(binop->rhs)) {
					Register* regRhs = emitCmpZero(binop->rhs);
					emitJump("je", elseLabel);
					regFree(regRhs)
				} else {
					emitTest(binop->rhs, trueLabel, elseLabel);
				}

				emitLabel(trueLabel);
				break;
			}
			default:
				break;
		}
	} else if (const auto funcCall = cast::toFuncCall(test)) {
		reg = emitFuncCall(*funcCall);
		emitInstr2op((isSSE(reg->rType) ? "ucomisd" : "cmp"), getRegName(reg, REG64), 0);
		emitJump("je", elseLabel);
		regFree(reg)
	} else if (const auto var = cast::toVar(test)) {
		reg = emitLoadRegFromMem(*var, REG64);
		emitInstr2op((isSSE(reg->rType) ? "ucomisd" : "cmp"), getRegName(reg, REG64), 0);
		emitJump("je", elseLabel);
		regFree(reg)
	} else if (cast::toNIL(test)) {
		emitJump("jmp", elseLabel);
	} else if (cast::toT(test)) {
		emitJump("jmp", trueLabel);
		emitLabel(trueLabel);
	}
}

void CodeGen::emitJmpTrueLabel(const Register* reg, const TokenType type, const std::string& label) {
	switch (type) {
		case TokenType::PLUS:
		case TokenType::MINUS:
		case TokenType::DIV:
		case TokenType::MUL:
		case TokenType::LOGAND:
		case TokenType::LOGIOR:
		case TokenType::LOGXOR:
		case TokenType::LOGNOR: {
			emitInstr2op((isSSE(reg->rType) ? "ucomisd" : "cmp"), getRegName(reg, REG64), 0);
			emitJump("jne", label);
			break;
		}
		case TokenType::EQUAL:
		case TokenType::NOT:
			emitJump("je", label);
			break;
		case TokenType::NEQUAL:
			emitJump("jne", label);
			break;
		case TokenType::GREATER_THEN:
			emitJump("jg", label);
			break;
		case TokenType::LESS_THEN:
			emitJump("jl", label);
			break;
		case TokenType::GREATER_THEN_EQ:
			emitJump("jge", label);
			break;
		case TokenType::LESS_THEN_EQ:
			emitJump("jle", label);
			break;
		default: break;
	}
}

Register* CodeGen::emitSet(const ExprPtr& set) {
	Register* setReg = nullptr;

	if (const auto binop = cast::toBinop(set)) {
		switch (binop->opToken.type) {
			case TokenType::PLUS:
			case TokenType::MINUS:
			case TokenType::DIV:
			case TokenType::MUL:
			case TokenType::LOGAND:
			case TokenType::LOGIOR:
			case TokenType::LOGXOR:
			case TokenType::LOGNOR:
				setReg = emitBinop(*binop);
				break;
			case TokenType::EQUAL:
			case TokenType::NOT:
				setReg = emitSetReg(*binop);
				emitSet8L("sete", setReg)
				break;
			case TokenType::NEQUAL:
				setReg = emitSetReg(*binop);
				emitSet8L("setne", setReg)
				break;
			case TokenType::GREATER_THEN:
				setReg = emitSetReg(*binop);
				emitSet8L("setg", setReg)
				break;
			case TokenType::LESS_THEN:
				setReg = emitSetReg(*binop);
				emitSet8L("setl", setReg)
				break;
			case TokenType::GREATER_THEN_EQ:
				setReg = emitSetReg(*binop);
				emitSet8L("setge", setReg)
				break;
			case TokenType::LESS_THEN_EQ:
				setReg = emitSetReg(*binop);
				emitSet8L("setle", setReg)
				break;
			case TokenType::AND:
				return emitLogOp(*binop, "and");
			case TokenType::OR:
				return emitLogOp(*binop, "or");
			default:
				break;
		}
	} else if (const auto funcCall = cast::toFuncCall(set)) {
		return emitFuncCall(*funcCall);
	}

	return setReg;
}

Register* CodeGen::emitLogOp(const BinOpExpr& binop, std::string_view op) {
	struct RegisterInfo {
		Register* reg{nullptr};
		Register* setReg{nullptr};
		std::string_view setRegStr{};
		std::string_view setReg8LStr{};
	};

	auto prepareRegister = [&](const ExprPtr& node, RegisterInfo& regInfo) {
		regInfo.reg = emitCmpZero(node);
		regInfo.setReg = isSSE(regInfo.reg->rType) ? regAlloc() : regInfo.reg;

		regInfo.setRegStr = getRegName(regInfo.setReg, REG64);
		regInfo.setReg8LStr = getRegName(regInfo.setReg, REG8L);

		emitInstr2op("xor", regInfo.setRegStr, regInfo.setRegStr);
		emitInstr1op("setne", regInfo.setReg8LStr);
	};

	RegisterInfo lhs;
	prepareRegister(binop.lhs, lhs);

	RegisterInfo rhs;
	prepareRegister(binop.rhs, rhs);

	emitInstr2op(op, lhs.setReg8LStr, rhs.setReg8LStr);
	movzx(lhs.setRegStr, lhs.setReg8LStr);

	if (isSSE(lhs.reg->rType)) {
		emitInstr2op("cvtsi2sd", getRegName(lhs.reg, REG64), lhs.setRegStr);
		regFree(lhs.setReg)
	}

	if (isSSE(rhs.reg->rType)) {
		regFree(rhs.setReg)
	}

	regFree(rhs.reg)
	return lhs.reg;
}

Register* CodeGen::emitSetReg(const BinOpExpr& binop) {
	const auto reg = emitBinop(binop);

	if (isSSE(reg->rType)) {
		regFree(reg)
		return regAlloc();
	}

	return reg;
}

Register* CodeGen::emitCmpZero(const ExprPtr& node) {
	const ExprPtr zero = std::make_shared<IntExpr>(0);
	return emitExpr(node, zero, {.op = "cmp", .opSSE = "ucomisd"});
}

void CodeGen::handleAssignment(const ExprPtr& var, const uint32_t size) {
	const auto var_ = cast::toVar(var);
	const std::string varName = cast::toString(var_->name)->data;

	if (const auto int_ = cast::toInt(var_->value)) {
		mov(getAddr(varName, var_->sType, REG64), int_->n);
	} else if (const auto double_ = cast::toDouble(var_->value)) {
		Register* reg = regAlloc();
		auto regStr = getRegName(reg, REG64);

		uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);

		mov(regStr, emitHex(hex));
		mov(getAddr(varName, var_->sType, REG64), regStr);
		regFree(reg)
	} else if (cast::toVar(var_->value)) {
		handleVariable(*var_, size);
	} else if (cast::toNIL(var_->value)) {
		mov(getAddr(varName, var_->sType, REG64), 0);
	} else if (cast::toT(var_->value)) {
		mov(getAddr(varName, var_->sType, REG64), 1);
	} else if (cast::toUninitialized(var_->value) && var_->sType == SymbolType::LOCAL) {
		getAddr(varName, var_->sType, REG64);
	} else if (const auto str = cast::toString(var_->value)) {
		const std::string label = ".L." + varName;
		std::string labelAddr = getAddr(label, var_->sType, size);
		std::string varAddr = getAddr(varName, var_->sType, size);

		updateSections("\nsection .data\n", {.name = label, .data = strDirective(str->data)});

		Register* reg = regAlloc();
		auto regStr = getRegName(reg, REG64);

		emitInstr2op("lea", regStr, labelAddr);
		mov(varAddr, regStr);
		regFree(reg)
	} else {
		Register* reg = emitSet(var_->value);
		emitStoreMemFromReg(varName, var_->sType, reg, REG64);
		regFree(reg)
	}
}

void CodeGen::handleVariable(const VarExpr& var, const uint32_t size) {
	const std::string varName = cast::toString(var.name)->data;
	const auto value = cast::toVar(var.value);

	if (Register* reg = emitLoadRegFromMem(*value, size)) {
		emitStoreMemFromReg(varName, var.sType, reg, size);
		regFree(reg)
	}
}

Register* CodeGen::emitLoadRegFromMem(const VarExpr& var, const uint32_t size) {
	Register* reg = nullptr;
	const std::string varName = cast::toString(var.name)->data;

	switch (var.sType) {
		case SymbolType::PARAM: {
			reg = regAlloc();
			mov(getRegName(reg, REG64), getAddr(varName, var.sType, size));
			break;
		}
		case SymbolType::LOCAL:
		case SymbolType::GLOBAL: {
			if (var.vType == VarType::INT) {
				reg = regAlloc();
				mov(getRegName(reg, REG64), getAddr(varName, var.sType, size));
			} else if (var.vType == VarType::DOUBLE) {
				reg = mRegisterAllocator.alloc(SSE);
				movsd(getRegName(reg, REG64), getAddr(varName, var.sType, size));
			} else if (cast::toString(var.value)) {
				reg = regAlloc();
				emitInstr2op("lea", getRegName(reg, REG64), getAddr(varName, var.sType, size));
			} else if (cast::toNIL(var.value) || cast::toT(var.value)) {
				reg = regAlloc();
				movzx(getRegName(reg, REG64), getAddr(varName, var.sType, size));
			}
			break;
		}
		default:
			break;
	}

	return reg;
}

void CodeGen::emitStoreMemFromReg(const std::string& varName,
                                  const SymbolType stype,
                                  const Register* reg,
                                  const uint32_t size) {
	auto regStr = getRegName(reg, size);

	if (isSSE(reg->rType)) {
		movsd(getAddr(varName, stype, size), regStr);
	} else {
		mov(getAddr(varName, stype, size), regStr);
	}
}

std::string CodeGen::getAddr(const std::string& varName, const SymbolType stype, const uint32_t size) {
	switch (stype) {
		case SymbolType::GLOBAL:
			return std::format("{} [rel {}]", mMemorySize[size], varName);
		case SymbolType::LOCAL:
			return std::format("{} [rbp - {}]",
			                   mMemorySize[size],
			                   mStackAllocator.pushStackFrame(mCurrentScope, varName, stype));
		case SymbolType::PARAM:
			return std::format("{} [rbp + {}]",
			                   mMemorySize[size],
			                   mStackAllocator.pushStackFrame(mCurrentScope, varName, stype));
		default:
			throw std::runtime_error("Unknown SymbolType.");
	}
}

uint32_t CodeGen::getMemSize(const ExprPtr& var) {
	auto var_ = cast::toVar(var);

	do {
		if (cast::toNIL(var_->value) || cast::toT(var_->value)) {
			return REG8L;
		}

		if (cast::toInt(var_->value) || cast::toDouble(var_->value)) {
			return REG64;
		}

		var_ = cast::toVar(var_->value);
	} while (var_);

	return 0;
}

void CodeGen::pushParamToRegister(const uint32_t rid, const std::any& value) {
	const Register* reg = mRegisterAllocator.regFromID(rid);
	auto regStr = getRegName(reg, REG64);

	if (isSSE(reg->rType)) {
		try {
			auto n = std::any_cast<double>(value);
			uint64_t hex = *reinterpret_cast<uint64_t*>(&n);

			Register* regScr = regAlloc();
			auto regScrStr = getRegName(regScr, REG64);

			mov(regScrStr, emitHex(hex));
			movq(regStr, regScrStr);
			regFree(regScr)
		} catch ([[maybe_unused]] const std::bad_any_cast& e) {
			movsd(regStr, std::any_cast<const char*>(value));
		}
	} else {
		try {
			mov(regStr, std::any_cast<int>(value));
		} catch ([[maybe_unused]] const std::bad_any_cast& e) {
			mov(regStr, std::any_cast<const char*>(value));
		}
	}
}

void CodeGen::pushParamOntoStack(const std::string& funcName, const VarExpr& param, int32_t& stackIdx) {
	const std::string paramName = cast::toString(param.name)->data;

	mStackAllocator.pushStackFrame(funcName, paramName, SymbolType::PARAM);

	const std::string addr = stackIdx ? std::format("qword [rsp + {}]", stackIdx) : "qword [rsp]";

	if (const auto int_ = cast::toInt(param.value)) {
		mov(addr, int_->n);
	} else if (const auto double_ = cast::toDouble(param.value)) {
		Register* regScr = regAlloc();
		auto regScrStr = getRegName(regScr, REG64);

		uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);

		mov(regScrStr, emitHex(hex));
		mov(addr, regScrStr);

		regFree(regScr)
	}

	stackIdx += 8;
}

const char* CodeGen::getRegName(const Register* reg, const uint32_t size) {
	return mRegisterAllocator.nameFromReg(reg, size);
}

const char* CodeGen::getRegNameByID(const uint32_t id, const uint32_t size) {
	return mRegisterAllocator.nameFromID(id, size);
}

std::string CodeGen::createLabel() {
	return ".L" + std::to_string(mCurrentLabelCount++);
}

void CodeGen::updateSections(const std::string_view sectionName, Section section) {
	mSections[std::string(sectionName)].push_back(std::move(section));
}

bool CodeGen::isPrimitive(const ExprPtr& var) {
	return cast::toInt(var) ||
	       cast::toDouble(var) ||
	       cast::toNIL(var) ||
	       cast::toT(var) ||
	       cast::toString(var) ||
	       cast::toVar(var);
}
