#include "codegen.hpp"

CodeGen::CodeGen()
	: mCurrentScope("main") {
}

std::string CodeGen::emit(const ExprPtr& ast) {
	mGeneratedCode =
			"extern _lrt_print_int\n"
			"extern _lrt_print_double\n"
			"section .text\n"
#if defined(__APPLE__) || defined(__MACH__)
			"\tglobal _main\n"
			"_main:\n";
#elif defined(__linux__)
	"\tglobal main\n"
			"main:\n";
#else
	throw std::runtime_error("Unsupported Operating System");
#endif
	push("rbp")
	mov("rbp", "rsp");

	auto next = ast;
	while (next != nullptr) {
		Register* reg = emitAST(next);
		regFree(reg)
		next = next->child;
	}

	emitInstr2op("xor", "rax", "rax");
	pop("rbp")
	ret();

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
	} else if (const auto print = cast::toPrint(ast)) {
		emitPrint(*print);
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
		case TokenType::plus:
			return emitExpr(binop.lhs, binop.rhs, {.op = "add", .opSSE = "addsd"});
		case TokenType::minus:
			return emitExpr(binop.lhs, binop.rhs, {.op = "sub", .opSSE = "subsd"});
		case TokenType::div:
			return emitExpr(binop.lhs, binop.rhs, {.op = "idiv", .opSSE = "divsd"});
		case TokenType::mul:
			return emitExpr(binop.lhs, binop.rhs, {.op = "imul", .opSSE = "mulsd"});
		case TokenType::logand:
			return emitExpr(binop.lhs, binop.rhs, {.op = "and", .opSSE = ""});
		case TokenType::logior:
			return emitExpr(binop.lhs, binop.rhs, {.op = "or", .opSSE = ""});
		case TokenType::logxor:
			return emitExpr(binop.lhs, binop.rhs, {.op = "xor", .opSSE = ""});
		case TokenType::lognor: {
			const ExprPtr negOne = std::make_shared<IntExpr>(-1);
			// Bitwise NOT seperately
			Register* regLhs = emitExpr(binop.lhs, negOne, {.op = "xor", .opSSE = ""});
			Register* regRhs = emitExpr(binop.rhs, negOne, {.op = "xor", .opSSE = ""});
			emitInstr2op("and", mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64),
			             mRegisterAllocator.nameFromReg(regRhs, RegisterSize::reg64));
			regFree(regRhs)
			return regLhs;
		}
		case TokenType::not_:
			return emitCmpZero(binop.lhs);
		case TokenType::equal:
		case TokenType::nequal:
		case TokenType::greaterThen:
		case TokenType::lessThen:
		case TokenType::greaterThenEq:
		case TokenType::lessThenEq:
		case TokenType::and_:
		case TokenType::or_:
			return emitExpr(binop.lhs, binop.rhs, {.op = "cmp", .opSSE = "ucomisd"});
		default:
			return nullptr;
	}
}

Register* CodeGen::emitDotimes(const DotimesExpr& dotimes) {
	const auto iterVar = cast::toVar(dotimes.iterationCount);
	const std::string_view iterVarName = cast::toString(iterVar->name)->data;
	// Labels
	const std::string loopLabel = createLabel();
	const std::string doneLabel = createLabel();
	// Loop condition
	ExprPtr name = iterVar->name;
	ExprPtr value = std::make_shared<IntExpr>(0);
	ExprPtr lhs = std::make_shared<VarExpr>(name, value, SymbolType::local);
	cast::toVar(lhs)->vType = iterVar->vType;

	ExprPtr rhs = iterVar->value;
	auto token = Token{TokenType::lessThen};
	ExprPtr test = std::make_shared<BinOpExpr>(lhs, rhs, token);
	// Address of iter var
	stackAlloc(mMemorySizeInBytes[std::to_underlying(RegisterSize::reg64)])
	std::string iterVarAddr = getAddr(iterVarName, SymbolType::local, RegisterSize::reg64);
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
	auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

	mov(regStr, iterVarAddr);
	emitInstr2op("add", regStr, 1);
	mov(iterVarAddr, regStr);

	regFree(reg)

	emitJump("jmp", loopLabel);
	emitLabel(doneLabel);

	stackDealloc(mMemorySizeInBytes[std::to_underlying(RegisterSize::reg64)])

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
		const int size = mMemorySizeInBytes[std::to_underlying(getMemSize(var))];
		requiredStackMem += size;
	}

	stackAlloc(requiredStackMem)

	for (const auto& var: let.bindings) {
		const RegisterSize memSize = getMemSize(var);
		handleAssignment(var, memSize);
	}

	for (const auto& sexpr: let.body) {
		reg = emitAST(sexpr);
		regFree(reg)
	}

	stackDealloc(requiredStackMem)

	return reg;
}

void CodeGen::emitSetq(const SetqExpr& setq) {
	const RegisterSize memSize = getMemSize(setq.pair);
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
		const std::string_view paramName = cast::toString(param->name)->data;

		if (param->vType == VarType::int_) {
			if (scratchIdx > 5)
				continue;
			scratchIdx++;
		} else if (param->vType == VarType::double_) {
			if (sseIdx > 7)
				continue;
			sseIdx++;
		}

		stackSize += mMemorySizeInBytes[std::to_underlying(getMemSize(arg))];
		mStackAllocator.pushStackFrame(mCurrentScope, paramName, param->sType);
	}

	stackAlloc(stackSize)

	scratchIdx = 0, sseIdx = 0;
	for (const auto& arg: defun.args) {
		const auto param = cast::toVar(arg);
		const std::string_view paramName = cast::toString(param->name)->data;

		if ((param->vType == VarType::int_ && scratchIdx > 5) || (param->vType == VarType::double_ && sseIdx > 7)) {
			continue;
		}

		mov(getAddr(paramName, param->sType, RegisterSize::reg64),
		    mRegisterAllocator.nameFromID(param->vType == VarType::int_
			    ? mParamRegisters[scratchIdx++]
			    : mParamRegistersSSE[sseIdx++], RegisterSize::reg64));
	}

	Register* reg = nullptr;
	for (const auto& form: defun.forms) {
		if (form == defun.forms.back() && cast::toBinop(form)) {
			reg = emitSet(form);
		} else {
			reg = emitAST(form);
		}
	}

	if (reg && reg->isSSE()) {
		movsd("xmm0", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64));
	} else if (reg && !reg->isSSE()) {
		mov("rax", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64));
	}

	regFree(reg)
	stackDealloc(stackSize)
	pop("rbp")
	ret();
}

void CodeGen::emitPrint(const PrintExpr& print) {
	ExprPtr funcName;

	if (const auto var = cast::toVar(print.arg); var && var->vType == VarType::int_) {
		ExprPtr name = std::make_shared<StringExpr>("_lrt_print_int");
		ExprPtr value = std::make_shared<Uninitialized>();
		funcName = std::make_shared<VarExpr>(name, value);
	} else if (var && var->vType == VarType::double_) {
		ExprPtr name = std::make_shared<StringExpr>("_lrt_print_double");
		ExprPtr value = std::make_shared<Uninitialized>();
		funcName = std::make_shared<VarExpr>(name, value);
	}

	const FuncCallExpr printFunc(funcName, {print.arg});
	emitFuncCall(printFunc);
}

Register* CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {
	const auto func = cast::toVar(funcCall.name);
	const std::string_view funcName = cast::toString(func->name)->data;

	// Calculate the proper stack size before function call
	uint32_t stackAlignedSize = mStackAllocator.calculateRequiredStackSize(funcCall.args);
	stackAlloc(stackAlignedSize)

	Register* reg;
	int32_t scratchIdx{0};
	int32_t sseIdx{0};
	int32_t stackIdx{0};
	for (const auto& arg: funcCall.args) {
		if (const auto param = cast::toVar(arg)) {
			// If scratch param size > 5 or sse param size > 7, push the params onto stack
			if ((scratchIdx > 5 && param->vType == VarType::int_) || (sseIdx > 7 && param->vType == VarType::double_)) {
				pushParamOntoStack(funcName, *param, stackIdx);
				continue;
			}
			// Push parameter to the appropriate register
			if (const auto innerVar = cast::toVar(param->value)) {
				const std::string_view paramName = cast::toString(innerVar->name)->data;
				pushParamToRegister(param->vType == VarType::int_
					                    ? mParamRegisters[scratchIdx++]
					                    : mParamRegistersSSE[sseIdx++],
				                    getAddr(paramName, innerVar->sType, RegisterSize::reg64).c_str());
			} else if (const auto binop = cast::toBinop(param->value)) {
				reg = emitBinop(*binop);
				pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
				                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64).data());
				regFree(reg)
			} else if (const auto fc = cast::toFuncCall(param->value)) {
				reg = emitFuncCall(*fc);

				pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
				                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64).data());
				regFree(reg)
			} else {
				const std::string_view paramName = cast::toString(param->name)->data;

				if (param->vType == VarType::int_) {
					if (param->sType == SymbolType::unknown) {
						pushParamToRegister(mParamRegisters[scratchIdx++], cast::toInt(param->value)->n);
					} else {
						pushParamToRegister(mParamRegisters[scratchIdx++], getAddr(paramName, param->sType, RegisterSize::reg64).c_str());
					}
				} else if (param->vType == VarType::double_) {
					if (param->sType == SymbolType::unknown) {
						pushParamToRegister(mParamRegistersSSE[sseIdx++], cast::toDouble(param->value)->n);
					} else {
						pushParamToRegister(mParamRegistersSSE[sseIdx++], getAddr(paramName, param->sType, RegisterSize::reg64).c_str());
					}
				}
			}
		} else if (const auto binop = cast::toBinop(arg)) {
			reg = emitBinop(*binop);
			pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
			                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64));
			regFree(reg)
		} else if (const auto fc = cast::toFuncCall(arg)) {
			reg = emitFuncCall(*fc);

			pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
			                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64));
			regFree(reg)
		}
	}

	emitInstr1op("call", funcName);

	if (cast::toDouble(funcCall.returnType)) {
		reg = mRegisterAllocator.alloc(RegisterType::sse);
		movsd(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), "xmm0");
	} else {
		reg = regAlloc();
		mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), "rax");
	}

	stackDealloc(stackAlignedSize)

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
		const std::string_view varName = cast::toString(var->name)->data;

		Register* reg = regAlloc();
		mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64),
		    getAddr(varName, var->sType, RegisterSize::reg64));

		return reg;
	}

	return nullptr;
}

Register* CodeGen::emitInt(const IntExpr& int_) {
	Register* reg = regAlloc();
	mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), int_.n);
	return reg;
}

Register* CodeGen::emitDouble(const DoubleExpr& double_) {
	Register* reg = regAlloc();
	auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

	Register* regSSE = mRegisterAllocator.alloc(RegisterType::sse);

	uint64_t hex = *reinterpret_cast<const uint64_t*>(&double_.n);

	mov(regStr, emitHex(hex));
	movq(mRegisterAllocator.nameFromReg(regSSE, RegisterSize::reg64), regStr);

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
	return emitLoadRegFromMem(*var, RegisterSize::reg64);
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

	if (regLhs->isSSE() && !regRhs->isSSE()) {
		Register* newReg = mRegisterAllocator.alloc(RegisterType::sse);
		auto newRegStr = mRegisterAllocator.nameFromReg(newReg, RegisterSize::reg64);

		emitInstr2op("cvtsi2sd", newRegStr, mRegisterAllocator.nameFromReg(regRhs, RegisterSize::reg64));
		regFree(regRhs)

		emitInstr2op(opcode.opSSE, mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64), newRegStr);
		regFree(newReg);

		return regLhs;
	}

	if (!regLhs->isSSE() && regRhs->isSSE()) {
		Register* newReg = mRegisterAllocator.alloc(RegisterType::sse);
		auto newRegStr = mRegisterAllocator.nameFromReg(newReg, RegisterSize::reg64);
		auto regRhsStr = mRegisterAllocator.nameFromReg(regRhs, RegisterSize::reg64);

		emitInstr2op("cvtsi2sd", newRegStr, mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64));
		regFree(regLhs)

		emitInstr2op(opcode.opSSE, newRegStr, regRhsStr);
		movsd(regRhsStr, newRegStr);
		regFree(newReg);
		return regRhs;
	}

	if (regLhs->isSSE() && regRhs->isSSE()) {
		emitInstr2op(opcode.opSSE, mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64),
		             mRegisterAllocator.nameFromReg(regRhs, RegisterSize::reg64));
		regFree(regRhs);
		return regLhs;
	}

	// rax -> dividend
	// idiv divisor[register/memory]
	if (opcode.op == "idiv") {
		mov("rax", mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64));
		cqo();
		emitInstr1op("idiv", mRegisterAllocator.nameFromReg(regRhs, RegisterSize::reg64));
		mov(mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64), "rax");
	} else {
		emitInstr2op(opcode.op, mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64),
		             mRegisterAllocator.nameFromReg(regRhs, RegisterSize::reg64));
	}

	regFree(regRhs);
	return regLhs;
}

void CodeGen::emitSection(const ExprPtr& var, const bool isConstant) {
	const auto var_ = cast::toVar(var);

	if (cast::toBinop(var_->value) || cast::toFuncCall(var_->value)) {
		updateSections("\nsection .bss\n", {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeUninitialized[std::to_underlying(RegisterSize::reg64)], 1)
		               });

		handleAssignment(var, RegisterSize::reg64);
	} else if (cast::toUninitialized(var_->value)) {
		updateSections("\nsection .bss\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeUninitialized[std::to_underlying(RegisterSize::reg64)], 1)
		               });
	} else if (cast::toNIL(var_->value)) {
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeInitialized[std::to_underlying(RegisterSize::reg8l)], 0)
		               });
	} else if (cast::toT(var_->value)) {
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeInitialized[std::to_underlying(RegisterSize::reg8l)], 1)
		               });
	} else if (const auto int_ = cast::toInt(var_->value)) {
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeInitialized[std::to_underlying(RegisterSize::reg64)], int_->n)
		               });
	} else if (const auto double_ = cast::toDouble(var_->value)) {
		uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeInitialized[std::to_underlying(RegisterSize::reg64)],
			                                    emitHex(hex))
		               });
	} else if (cast::toVar(var_->value)) {
		const RegisterSize memSize = getMemSize(var_);

		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeInitialized[std::to_underlying(memSize)], 0)
		               });
		handleAssignment(var, memSize);
	} else if (const auto str = cast::toString(var_->value)) {
		updateSections("\nsection .rodata\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = strDirective(str->data)
		               });
	}
}

void CodeGen::emitTest(const ExprPtr& test, std::string_view trueLabel, std::string_view elseLabel) {
	Register* reg;

	if (const auto binop = cast::toBinop(test)) {
		switch (binop->opToken.type) {
			case TokenType::plus:
			case TokenType::minus:
			case TokenType::div:
			case TokenType::mul:
			case TokenType::logand:
			case TokenType::logior:
			case TokenType::logxor:
			case TokenType::lognor: {
				reg = emitBinop(*binop);
				emitInstr2op(reg->isSSE() ? "ucomisd" : "cmp", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64),
				             0);
				emitJump("je", elseLabel);
				regFree(reg)
				break;
			}
			case TokenType::equal:
			case TokenType::not_:
				reg = emitBinop(*binop);
				emitJump("jne", elseLabel);
				regFree(reg)
				break;
			case TokenType::nequal:
				reg = emitBinop(*binop);
				emitJump("je", elseLabel);
				regFree(reg)
				break;
			case TokenType::greaterThen:
				reg = emitBinop(*binop);
				emitJump("jle", elseLabel);
				regFree(reg)
				break;
			case TokenType::lessThen:
				reg = emitBinop(*binop);
				emitJump("jge", elseLabel);
				regFree(reg)
				break;
			case TokenType::greaterThenEq:
				reg = emitBinop(*binop);
				emitJump("jl", elseLabel);
				regFree(reg)
				break;
			case TokenType::lessThenEq:
				reg = emitBinop(*binop);
				emitJump("jg", elseLabel);
				regFree(reg)
				break;
			case TokenType::and_: {
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
			case TokenType::or_: {
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
		emitInstr2op(reg->isSSE() ? "ucomisd" : "cmp", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), 0);
		emitJump("je", elseLabel);
		regFree(reg)
	} else if (const auto var = cast::toVar(test)) {
		reg = emitLoadRegFromMem(*var, RegisterSize::reg64);
		emitInstr2op(reg->isSSE() ? "ucomisd" : "cmp", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), 0);
		emitJump("je", elseLabel);
		regFree(reg)
	} else if (cast::toNIL(test)) {
		emitJump("jmp", elseLabel);
	} else if (cast::toT(test)) {
		emitJump("jmp", trueLabel);
		emitLabel(trueLabel);
	}
}

void CodeGen::emitJmpTrueLabel(const Register* reg, const TokenType type, std::string_view label) {
	switch (type) {
		case TokenType::plus:
		case TokenType::minus:
		case TokenType::div:
		case TokenType::mul:
		case TokenType::logand:
		case TokenType::logior:
		case TokenType::logxor:
		case TokenType::lognor: {
			emitInstr2op(reg->isSSE() ? "ucomisd" : "cmp", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), 0);
			emitJump("jne", label);
			break;
		}
		case TokenType::equal:
		case TokenType::not_:
			emitJump("je", label);
			break;
		case TokenType::nequal:
			emitJump("jne", label);
			break;
		case TokenType::greaterThen:
			emitJump("jg", label);
			break;
		case TokenType::lessThen:
			emitJump("jl", label);
			break;
		case TokenType::greaterThenEq:
			emitJump("jge", label);
			break;
		case TokenType::lessThenEq:
			emitJump("jle", label);
			break;
		default: break;
	}
}

Register* CodeGen::emitSet(const ExprPtr& set) {
	Register* setReg = nullptr;

	if (const auto binop = cast::toBinop(set)) {
		switch (binop->opToken.type) {
			case TokenType::plus:
			case TokenType::minus:
			case TokenType::div:
			case TokenType::mul:
			case TokenType::logand:
			case TokenType::logior:
			case TokenType::logxor:
			case TokenType::lognor:
				setReg = emitBinop(*binop);
				break;
			case TokenType::equal:
			case TokenType::not_:
				setReg = emitSetReg(*binop);
				emitSet8L("sete", setReg)
				break;
			case TokenType::nequal:
				setReg = emitSetReg(*binop);
				emitSet8L("setne", setReg)
				break;
			case TokenType::greaterThen:
				setReg = emitSetReg(*binop);
				emitSet8L("setg", setReg)
				break;
			case TokenType::lessThen:
				setReg = emitSetReg(*binop);
				emitSet8L("setl", setReg)
				break;
			case TokenType::greaterThenEq:
				setReg = emitSetReg(*binop);
				emitSet8L("setge", setReg)
				break;
			case TokenType::lessThenEq:
				setReg = emitSetReg(*binop);
				emitSet8L("setle", setReg)
				break;
			case TokenType::and_:
				return emitLogOp(*binop, "and");
			case TokenType::or_:
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
		regInfo.setReg = regInfo.reg->isSSE() ? regAlloc() : regInfo.reg;

		regInfo.setRegStr = mRegisterAllocator.nameFromReg(regInfo.setReg, RegisterSize::reg64);
		regInfo.setReg8LStr = mRegisterAllocator.nameFromReg(regInfo.setReg, RegisterSize::reg8l);

		emitInstr2op("xor", regInfo.setRegStr, regInfo.setRegStr);
		emitInstr1op("setne", regInfo.setReg8LStr);
	};

	RegisterInfo lhs;
	prepareRegister(binop.lhs, lhs);

	RegisterInfo rhs;
	prepareRegister(binop.rhs, rhs);

	emitInstr2op(op, lhs.setReg8LStr, rhs.setReg8LStr);
	movzx(lhs.setRegStr, lhs.setReg8LStr);

	if (lhs.reg->isSSE()) {
		emitInstr2op("cvtsi2sd", mRegisterAllocator.nameFromReg(lhs.reg, RegisterSize::reg64), lhs.setRegStr);
		regFree(lhs.setReg)
	}

	if (rhs.reg->isSSE()) {
		regFree(rhs.setReg)
	}

	regFree(rhs.reg)
	return lhs.reg;
}

Register* CodeGen::emitSetReg(const BinOpExpr& binop) {
	const auto reg = emitBinop(binop);

	if (reg->isSSE()) {
		regFree(reg)
		return regAlloc();
	}

	return reg;
}

Register* CodeGen::emitCmpZero(const ExprPtr& node) {
	const ExprPtr zero = std::make_shared<IntExpr>(0);
	return emitExpr(node, zero, {.op = "cmp", .opSSE = "ucomisd"});
}

void CodeGen::handleAssignment(const ExprPtr& var, const RegisterSize size) {
	const auto var_ = cast::toVar(var);
	const std::string_view varName = cast::toString(var_->name)->data;

	if (const auto int_ = cast::toInt(var_->value)) {
		mov(getAddr(varName, var_->sType, RegisterSize::reg64), int_->n);
	} else if (const auto double_ = cast::toDouble(var_->value)) {
		Register* reg = regAlloc();
		auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

		uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);

		mov(regStr, emitHex(hex));
		mov(getAddr(varName, var_->sType, RegisterSize::reg64), regStr);
		regFree(reg)
	} else if (cast::toVar(var_->value)) {
		handleVariable(*var_, size);
	} else if (cast::toNIL(var_->value)) {
		mov(getAddr(varName, var_->sType, RegisterSize::reg64), 0);
	} else if (cast::toT(var_->value)) {
		mov(getAddr(varName, var_->sType, RegisterSize::reg64), 1);
	} else if (cast::toUninitialized(var_->value) && var_->sType == SymbolType::local) {
		getAddr(varName, var_->sType, RegisterSize::reg64);
	} else if (const auto str = cast::toString(var_->value)) {
		std::string label = ".L.";
		label += varName;
		std::string labelAddr = getAddr(label, var_->sType, size);
		std::string varAddr = getAddr(varName, var_->sType, size);

		updateSections("\nsection .data\n", {.name = label, .data = strDirective(str->data)});

		Register* reg = regAlloc();
		auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

		emitInstr2op("lea", regStr, labelAddr);
		mov(varAddr, regStr);
		regFree(reg)
	} else {
		Register* reg = emitSet(var_->value);
		emitStoreMemFromReg(varName, var_->sType, reg, RegisterSize::reg64);
		regFree(reg)
	}
}

void CodeGen::handleVariable(const VarExpr& var, const RegisterSize size) {
	const std::string_view varName = cast::toString(var.name)->data;
	const auto value = cast::toVar(var.value);

	if (Register* reg = emitLoadRegFromMem(*value, size)) {
		emitStoreMemFromReg(varName, var.sType, reg, size);
		regFree(reg)
	}
}

Register* CodeGen::emitLoadRegFromMem(const VarExpr& var, const RegisterSize size) {
	Register* reg = nullptr;
	const std::string_view varName = cast::toString(var.name)->data;

	switch (var.sType) {
		case SymbolType::param: {
			reg = regAlloc();
			mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), getAddr(varName, var.sType, size));
			break;
		}
		case SymbolType::local:
		case SymbolType::global: {
			if (var.vType == VarType::int_) {
				reg = regAlloc();
				mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), getAddr(varName, var.sType, size));
			} else if (var.vType == VarType::double_) {
				reg = mRegisterAllocator.alloc(RegisterType::sse);
				movsd(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), getAddr(varName, var.sType, size));
			} else if (cast::toString(var.value)) {
				reg = regAlloc();
				emitInstr2op("lea", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64),
				             getAddr(varName, var.sType, size));
			} else if (cast::toNIL(var.value) || cast::toT(var.value)) {
				reg = regAlloc();
				movzx(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), getAddr(varName, var.sType, size));
			}
			break;
		}
		default:
			break;
	}

	return reg;
}

void CodeGen::emitStoreMemFromReg(const std::string_view varName,
                                  const SymbolType stype,
                                  const Register* reg,
                                  const RegisterSize size) {
	auto regStr = mRegisterAllocator.nameFromReg(reg, size);

	if (reg->isSSE()) {
		movsd(getAddr(varName, stype, size), regStr);
	} else {
		mov(getAddr(varName, stype, size), regStr);
	}
}

std::string CodeGen::getAddr(const std::string_view varName, const SymbolType stype, const RegisterSize size) {
	switch (stype) {
		case SymbolType::global:
			return std::format("{} [rel {}]", mMemorySize[std::to_underlying(size)], varName);
		case SymbolType::local:
			return std::format("{} [rbp - {}]",
			                   mMemorySize[std::to_underlying(size)],
			                   mStackAllocator.pushStackFrame(mCurrentScope, varName, stype));
		case SymbolType::param:
			return std::format("{} [rbp + {}]",
			                   mMemorySize[std::to_underlying(size)],
			                   mStackAllocator.pushStackFrame(mCurrentScope, varName, stype));
		default:
			throw std::runtime_error("Unknown SymbolType.");
	}
}

RegisterSize CodeGen::getMemSize(const ExprPtr& var) {
	auto var_ = cast::toVar(var);

	do {
		if (cast::toNIL(var_->value) || cast::toT(var_->value)) {
			return RegisterSize::reg8l;
		}

		if (cast::toInt(var_->value) || cast::toDouble(var_->value)) {
			return RegisterSize::reg64;
		}

		var_ = cast::toVar(var_->value);
	} while (var_);

	return RegisterSize::zero;
}

void CodeGen::pushParamOntoStack(const std::string_view funcName, const VarExpr& param, int32_t& stackIdx) {
	const std::string paramName = cast::toString(param.name)->data;

	mStackAllocator.pushStackFrame(funcName, paramName, SymbolType::param);

	const std::string addr = stackIdx ? std::format("qword [rsp + {}]", stackIdx) : "qword [rsp]";

	if (const auto int_ = cast::toInt(param.value)) {
		mov(addr, int_->n);
	} else if (const auto double_ = cast::toDouble(param.value)) {
		Register* regScr = regAlloc();
		auto regScrStr = mRegisterAllocator.nameFromReg(regScr, RegisterSize::reg64);

		uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);

		mov(regScrStr, emitHex(hex));
		mov(addr, regScrStr);

		regFree(regScr)
	}

	stackIdx += 8;
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
