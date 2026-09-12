#include "codegen.hpp"

CodeGen::CodeGen()
	: mCurrentScope("main") {
}

std::string CodeGen::emit(const ExprPtr& ast) {
	mGeneratedCode =
			"extern _lrt_print_int\n"
			"extern _lrt_print_double\n"
			"extern _lrt_print_str\n"
			"extern _lrt_read_int\n"
			"extern _lrt_read_double\n"
			"extern _lrt_read_str\n"
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
	push("rbp");
	mov("rbp", "rsp");

	auto next = ast;
	while (next != nullptr) {
		Register* reg = emitAST(next, true);
		regFree(reg);
		next = next->child;
	}

	emitInstr2op("xor", "eax", "eax");
	leave();
	ret();

	// Function definitions
	for (const auto& [func, defun]: mFunctions) {
		(this->*func)(defun);
	}

	// Sections
	for (const auto& [section, data]: mSections) {
		mGeneratedCode += section;

		for (const auto& [name, size]: data) {
			mGeneratedCode += std::format("{}: \n\t{}\n", name, size);
		}
	}

	return mGeneratedCode;
}

Register* CodeGen::emitAST(const ExprPtr& ast, const bool discardResult) {
	if (const auto binop = cast::toBinop(ast)) {
		return emitBinop(*binop);
	}
	if (const auto dotimes = cast::toDotimes(ast)) {
		return emitDotimes(*dotimes);
	}
	if (const auto loop = cast::toLoop(ast)) {
		return emitLoop(*loop, discardResult);
	}
	if (const auto let = cast::toLet(ast)) {
		return emitLet(*let, discardResult);
	}
	if (const auto setq = cast::toSetq(ast)) {
		return emitSetq(*setq, discardResult);
	}
	if (const auto defvar = cast::toDefvar(ast)) {
		emitDefvar(*defvar);
	} else if (const auto defconst = cast::toDefconstant(ast)) {
		emitDefconst(*defconst);
	} else if (const auto defun = cast::toDefun(ast)) {
		mFunctions.emplace_back(&CodeGen::emitDefun, *defun);
	} else if (const auto print = cast::toPrint(ast)) {
		return emitPrint(*print);
	} else if (const auto funcCall = cast::toFuncCall(ast)) {
		return emitFuncCall(*funcCall);
	} else if (const auto if_ = cast::toIf(ast)) {
		return emitIf(*if_, discardResult);
	} else if (const auto when = cast::toWhen(ast)) {
		return emitWhen(*when, discardResult);
	} else if (const auto cond = cast::toCond(ast)) {
		return emitCond(*cond, discardResult);
	} else if (cast::toInt(ast) || cast::toDouble(ast)) {
		return emitPrimitive(ast);
	} else if (const auto var = cast::toVar(ast)) {
		return emitLoadRegFromMem(*var, RegisterSize::reg64);
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
			regFree(regRhs);
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
	const auto iterVar = cast::toVar(dotimes.countForm);
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
	stackAlloc(mMemorySizeInBytes[std::to_underlying(RegisterSize::reg64)]);
	std::string iterVarAddr = getAddr(iterVarName, iterVar->vType, SymbolType::local, RegisterSize::reg64);
	// Set 0 to iter var
	mov(iterVarAddr, 0);
	// Loop label
	emitLabel(loopLabel);
	emitTest(test, "", doneLabel);
	// Emit statements
	Register* reg = nullptr;
	for (const auto& statement: dotimes.statements) {
		reg = emitAST(statement, true);
		regFree(reg);
	}
	// Increment iteration count
	reg = regAlloc();
	auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

	mov(regStr, iterVarAddr);
	emitInstr2op("add", regStr, 1);
	mov(iterVarAddr, regStr);

	emitJump("jmp", loopLabel);
	emitLabel(doneLabel);

	regFree(reg);
	stackDealloc(mMemorySizeInBytes[std::to_underlying(RegisterSize::reg64)]);

	if (!dotimes.resultForm) {
		return emitInt(*std::make_shared<IntExpr>(0));
	}

	if (cast::toInt(dotimes.resultForm) ||
	    cast::toDouble(dotimes.resultForm) ||
	    cast::toNIL(dotimes.resultForm) ||
	    cast::toT(dotimes.resultForm)) {
		return emitPrimitive(dotimes.resultForm);
	}

	if (const auto var = cast::toVar(dotimes.resultForm)) {
		return emitLoadRegFromMem(*var, RegisterSize::reg64);
	}

	return nullptr;
}

Register* CodeGen::emitLoop(const LoopExpr& loop, const bool discardResult) {
	Register* reg = nullptr;
	// Labels
	std::string loopLabel = createLabel();
	std::string doneLabel = createLabel();

	emitLabel(loopLabel);

	bool hasReturn{false};
	for (auto& sexpr: loop.sexprs) {
		const auto when = cast::toWhen(sexpr);
		if (!when) {
			reg = emitAST(sexpr, discardResult);
			regFree(reg);
			continue;
		}

		for (auto& form: when->then) {
			if (const auto return_ = cast::toReturn(form); !return_) {
				reg = emitAST(form, discardResult);
				regFree(reg);
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

Register* CodeGen::emitLet(const LetExpr& let, const bool discardResult) {
	uint32_t requiredStackMem = 0;

	for (const auto& binding: let.bindings) {
		requiredStackMem += mMemorySizeInBytes[std::to_underlying(getMemSize(binding))];
	}

	stackAlloc(requiredStackMem);

	for (const auto& binding: let.bindings) {
		const RegisterSize memSize = getMemSize(binding);

		if (const auto var = cast::toVar(binding); cast::toUninitialized(var->value)) {
			const std::string_view varName = cast::toString(var->name)->data;
			getAddr(varName, var->vType, var->sType, memSize);
		} else {
			emitAssignment(binding, memSize, true);
		}
	}

	Register* reg = nullptr;
	for (const auto& sexpr: let.body) {
		const bool isLast = sexpr == let.body.back();
		reg = emitAST(sexpr, discardResult && !isLast);

		if (!isLast)
			regFree(reg);
	}

	stackDealloc(requiredStackMem);
	return reg;
}

Register* CodeGen::emitSetq(const SetqExpr& setq, const bool discardResult) {
	const RegisterSize memSize = getMemSize(setq.pair);
	return emitAssignment(setq.pair, memSize, discardResult);
}

void CodeGen::emitDefvar(const DefvarExpr& defvar) {
	const auto var = cast::toVar(defvar.pair);
	const std::string_view varName = cast::toString(var->name)->data;

	if (const auto str = cast::toString(var->value)) {
		std::string label = ".L.";
		label += varName;

		updateSections("\nsection .rodata\n", {.name = label, .data = strDirective(str->data)});
		updateSections(
			"\nsection .data\n",
			{
				.name = varName.data(),
				.data = memDirective(mDataSizeInitialized[std::to_underlying(RegisterSize::reg64)], label)
			});
	} else {
		emitSection(defvar.pair);
	}
}

void CodeGen::emitDefconst(const DefconstExpr& defconst) {
	emitSection(defconst.pair, true);
}

void CodeGen::emitDefun(const DefunExpr& defun) {
	const auto func = cast::toVar(defun.name);
	mCurrentScope = cast::toString(func->name)->data;

	emitLabel("\n" + mCurrentScope);
	push("rbp");
	mov("rbp", "rsp");

	uint32_t requiredStackSize{0};
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

		const auto size = mMemorySizeInBytes[std::to_underlying(getMemSize(arg))];
		requiredStackSize += size;
		mStackAllocator.pushStackFrame(mCurrentScope, paramName, param->sType, size);
	}

	stackAlloc(requiredStackSize);

	scratchIdx = 0, sseIdx = 0;
	for (const auto& arg: defun.args) {
		const auto param = cast::toVar(arg);
		const std::string_view paramName = cast::toString(param->name)->data;

		if ((param->vType == VarType::int_ && scratchIdx > 5) || (param->vType == VarType::double_ && sseIdx > 7)) {
			continue;
		}

		const auto size = getMemSize(arg);
		if (param->vType == VarType::double_) {
			movsd(getAddr(paramName, param->vType, param->sType, size),
			      mRegisterAllocator.nameFromID(mParamRegistersSSE[sseIdx++], size));
		} else {
			mov(getAddr(paramName, param->vType, param->sType, size),
			    mRegisterAllocator.nameFromID(mParamRegisters[scratchIdx++], size));
		}
	}

	Register* reg = nullptr;
	for (const auto& form: defun.forms) {
		const bool isLast = form == defun.forms.back();

		reg = emitAST(form, isLast);

		if (!isLast)
			regFree(reg);
	}

	if (reg && reg->isSSE()) {
		movsd("xmm0", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64));
	} else if (reg && !reg->isSSE()) {
		mov("rax", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64));
	}

	regFree(reg);
	stackDealloc(requiredStackSize);
	leave();
	ret();
}

Register* CodeGen::emitPrint(const PrintExpr& print) {
	ExprPtr name;

	if (const auto var = cast::toVar(print.arg); var && var->vType == VarType::int_) {
		name = std::make_shared<StringExpr>("_lrt_print_int");
	} else if (var && var->vType == VarType::double_) {
		name = std::make_shared<StringExpr>("_lrt_print_double");
	} else if (var && var->vType == VarType::string) {
		name = std::make_shared<StringExpr>("_lrt_print_str");
	} else if (const auto func = cast::toFuncCall(print.arg)) {
		if (cast::toInt(func->returnType) || cast::toNIL(func->returnType) || cast::toT(func->returnType)) {
			name = std::make_shared<StringExpr>("_lrt_print_int");
		} else if (cast::toDouble(func->returnType)) {
			name = std::make_shared<StringExpr>("_lrt_print_double");
		}
	} else if (const auto binop = cast::toBinop(print.arg)) {
		if (cast::toDouble(binop->lhs) || cast::toDouble(binop->rhs)) {
			name = std::make_shared<StringExpr>("_lrt_print_double");
		} else {
			name = std::make_shared<StringExpr>("_lrt_print_int");
		}
	}

	ExprPtr value = std::make_shared<Uninitialized>();
	const ExprPtr funcName = std::make_shared<VarExpr>(name, value);

	FuncCallExpr printFunc(funcName, {print.arg});
	printFunc.returnType = print.returnType;

	return emitFuncCall(printFunc);
}

Register* CodeGen::emitRead(const ReadExpr& read) {
	ExprPtr name;
	if (cast::toInt(read.returnType)) {
		name = std::make_shared<StringExpr>("_lrt_read_int");
	} else if (cast::toDouble(read.returnType)) {
		name = std::make_shared<StringExpr>("_lrt_read_double");
	} else if (cast::toString(read.returnType)) {
		name = std::make_shared<StringExpr>("_lrt_read_str");
	}

	ExprPtr value = std::make_shared<Uninitialized>();
	const ExprPtr funcName = std::make_shared<VarExpr>(name, value);

	FuncCallExpr readFunc(funcName, {});
	readFunc.returnType = read.returnType;

	return emitFuncCall(readFunc);
}

Register* CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {
	const auto func = cast::toVar(funcCall.name);
	const std::string_view funcName = cast::toString(func->name)->data;

	// Calculate the proper stack size before function call
	uint32_t stackAlignedSize = mStackAllocator.calculateCallStackSize(funcCall.args);
	stackAlloc(stackAlignedSize);

	Register* reg;
	int32_t scratchIdx{0};
	int32_t sseIdx{0};
	for (const auto& arg: funcCall.args) {
		if (const auto param = cast::toVar(arg)) {
			// If scratch param size > 5 or sse param size > 7, push the params onto stack
			if ((scratchIdx > 5 && param->vType == VarType::int_) || (sseIdx > 7 && param->vType == VarType::double_)) {
				pushParamOntoStack(funcName, arg);
				continue;
			}
			// Push parameter to the appropriate register
			if (const auto innerVar = cast::toVar(param->value)) {
				const std::string_view innerVarName = cast::toString(innerVar->name)->data;
				pushParamToRegister(innerVar->vType == VarType::int_
					                    ? mParamRegisters[scratchIdx++]
					                    : mParamRegistersSSE[sseIdx++],
				                    innerVar->vType,
				                    innerVar->iType,
				                    getAddr(innerVarName, innerVar->vType, innerVar->sType,
				                            RegisterSize::reg64).c_str());
			} else if (const auto binop = cast::toBinop(param->value); binop && param->sType == SymbolType::param) {
				reg = emitBinop(*binop);
				pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
				                    reg->isSSE() ? VarType::double_ : VarType::int_,
				                    InitType::unknown,
				                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64).data());
				regFree(reg);
			} else if (const auto fc = cast::toFuncCall(param->value); fc && param->sType == SymbolType::param) {
				reg = emitFuncCall(*fc);

				pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
				                    reg->isSSE() ? VarType::double_ : VarType::int_,
				                    InitType::unknown,
				                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64).data());
				regFree(reg);
			} else if (const auto read = cast::toRead(param->value); read && param->sType == SymbolType::param) {
				reg = emitRead(*read);

				pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
				                    reg->isSSE() ? VarType::double_ : VarType::int_,
				                    InitType::unknown,
				                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64).data());
				regFree(reg);
			} else {
				const std::string_view paramName = cast::toString(param->name)->data;

				switch (param->vType) {
					case VarType::int_: {
						if (param->sType == SymbolType::param) {
							pushParamToRegister(
								mParamRegisters[scratchIdx++],
								param->vType,
								param->iType,
								cast::toInt(param->value)->n);
						} else {
							pushParamToRegister(
								mParamRegisters[scratchIdx++],
								param->vType,
								param->iType,
								getAddr(paramName, param->vType, param->sType, RegisterSize::reg64).c_str());
						}
						break;
					}
					case VarType::double_: {
						if (param->sType == SymbolType::param) {
							pushParamToRegister(
								mParamRegistersSSE[sseIdx++],
								param->vType,
								param->iType,
								cast::toDouble(param->value)->n);
						} else {
							pushParamToRegister(
								mParamRegistersSSE[sseIdx++],
								param->vType,
								param->iType,
								getAddr(paramName, param->vType, param->sType, RegisterSize::reg64).c_str());
						}
						break;
					}
					case VarType::string: {
						if (param->sType == SymbolType::param) {
							emitSection(arg);

							pushParamToRegister(
								mParamRegisters[scratchIdx++],
								param->vType,
								param->iType,
								getAddr(paramName, param->vType, SymbolType::global, RegisterSize::reg64).c_str());
						} else {
							pushParamToRegister(
								mParamRegisters[scratchIdx++],
								param->vType,
								param->iType,
								getAddr(paramName, param->vType, param->sType, RegisterSize::reg64).c_str());
						}
						break;
					}
					case VarType::nil: {
						if (param->sType == SymbolType::param) {
							pushParamToRegister(
								mParamRegisters[scratchIdx++],
								param->vType,
								param->iType,
								0);
						} else {
							pushParamToRegister(
								mParamRegisters[scratchIdx++],
								param->vType,
								param->iType,
								getAddr(paramName, param->vType, param->sType, RegisterSize::reg64).c_str());
						}
						break;
					}
					case VarType::t: {
						if (param->sType == SymbolType::param) {
							pushParamToRegister(
								mParamRegisters[scratchIdx++],
								param->vType,
								param->iType,
								1);
						} else {
							pushParamToRegister(
								mParamRegisters[scratchIdx++],
								param->vType,
								param->iType,
								getAddr(paramName, param->vType, param->sType, RegisterSize::reg64).c_str());
						}
						break;
					}
					case VarType::unknown:
						break;
				}
			}
		} else if (const auto binop = cast::toBinop(arg)) {
			reg = emitBinop(*binop);
			pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
			                    reg->isSSE() ? VarType::double_ : VarType::int_,
			                    InitType::unknown,
			                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64).data());
			regFree(reg);
		} else if (const auto fc = cast::toFuncCall(arg)) {
			reg = emitFuncCall(*fc);

			pushParamToRegister(reg->isSSE() ? mParamRegistersSSE[sseIdx++] : mParamRegisters[scratchIdx++],
			                    reg->isSSE() ? VarType::double_ : VarType::int_,
			                    InitType::unknown,
			                    mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64).data());
			regFree(reg);
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

	stackDealloc(stackAlignedSize);

	return reg;
}

Register* CodeGen::emitIf(const IfExpr& if_, const bool discardResult) {
	const std::string trueLabel = createLabel();
	const std::string elseLabel = createLabel();
	const std::string done = createLabel();
	// Emit test
	emitTest(if_.test, trueLabel, elseLabel);
	// Emit then
	Register* reg = nullptr;
	reg = emitAST(if_.then, discardResult);
	// Emit else
	emitJump("jmp", done);
	emitLabel(elseLabel);
	regFree(reg);

	if (!cast::toUninitialized(if_.else_)) {
		reg = emitAST(if_.else_, discardResult);
	} else {
		if (reg->isSSE())
			reg = emitDouble(*std::make_shared<DoubleExpr>(0.0));
		else
			reg = emitInt(*std::make_shared<IntExpr>(0));
	}

	emitLabel(done);

	return reg;
}

Register* CodeGen::emitWhen(const WhenExpr& when, const bool discardResult) {
	const std::string doneLabel = createLabel();
	// Emit test
	emitTest(when.test, "", doneLabel);
	// Emit then
	Register* reg = nullptr;
	for (const auto& form: when.then) {
		reg = emitAST(form, discardResult);
		regFree(reg);
	}
	emitLabel(doneLabel);

	return reg;
}

Register* CodeGen::emitCond(const CondExpr& cond, const bool discardResult) {
	const std::string done = createLabel();

	Register* reg = nullptr;
	for (const auto& [test, forms]: cond.variants) {
		const std::string elseLabel = createLabel();
		emitTest(test, "", elseLabel);

		for (const auto& form: forms) {
			reg = emitAST(form, discardResult);
			regFree(reg);
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

	if (const auto nil = cast::toNIL(prim)) {
		return emitInt(*std::make_shared<IntExpr>(0));
	}

	if (const auto t = cast::toT(prim)) {
		return emitInt(*std::make_shared<IntExpr>(1));
	}

	return nullptr;
}

Register* CodeGen::emitInt(const IntExpr& int_) {
	Register* reg = regAlloc();
	mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), int_.n);
	return reg;
}

Register* CodeGen::emitDouble(DoubleExpr& double_) {
	Register* reg = regAlloc();
	auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

	Register* regSSE = mRegisterAllocator.alloc(RegisterType::sse);

	mov(regStr, emitHex(toHex(double_.n)));
	movq(mRegisterAllocator.nameFromReg(regSSE, RegisterSize::reg64), regStr);

	regFree(reg);

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
	return emitLoadRegFromMem(*var, getMemSize(n));
}

Register* CodeGen::emitNode(const ExprPtr& node) {
	if (const auto binOp = cast::toBinop(node)) {
		return emitBinop(*binOp);
	}

	if (const auto funcCall = cast::toFuncCall(node)) {
		return emitFuncCall(*funcCall);
	}

	if (const auto read = cast::toRead(node)) {
		return emitRead(*read);
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
		regFree(regRhs);

		emitInstr2op(opcode.opSSE, mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64), newRegStr);
		regFree(newReg);

		return regLhs;
	}

	if (!regLhs->isSSE() && regRhs->isSSE()) {
		Register* newReg = mRegisterAllocator.alloc(RegisterType::sse);
		auto newRegStr = mRegisterAllocator.nameFromReg(newReg, RegisterSize::reg64);
		auto regRhsStr = mRegisterAllocator.nameFromReg(regRhs, RegisterSize::reg64);

		emitInstr2op("cvtsi2sd", newRegStr, mRegisterAllocator.nameFromReg(regLhs, RegisterSize::reg64));
		regFree(regLhs);

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

void CodeGen::emitSection(const ExprPtr& var, const bool isConstant, const bool discardResult) {
	if (const auto var_ = cast::toVar(var); cast::toBinop(var_->value) || cast::toFuncCall(var_->value)) {
		updateSections("\nsection .bss\n", {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeUninitialized[std::to_underlying(RegisterSize::reg64)], 1)
		               });

		emitAssignment(var, RegisterSize::reg64, discardResult);
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
		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeInitialized[std::to_underlying(RegisterSize::reg64)],
			                                    emitHex(toHex(double_->n)))
		               });
	} else if (cast::toVar(var_->value)) {
		const RegisterSize memSize = getMemSize(var);

		updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
		               {
			               .name = cast::toString(var_->name)->data,
			               .data = memDirective(mDataSizeInitialized[std::to_underlying(memSize)], 0)
		               });
		emitAssignment(var, memSize, discardResult);
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
				regFree(reg);
				break;
			}
			case TokenType::equal:
			case TokenType::not_:
				reg = emitBinop(*binop);
				emitJump("jne", elseLabel);
				regFree(reg);
				break;
			case TokenType::nequal:
				reg = emitBinop(*binop);
				emitJump("je", elseLabel);
				regFree(reg);
				break;
			case TokenType::greaterThen:
				reg = emitBinop(*binop);
				emitJump("jle", elseLabel);
				regFree(reg);
				break;
			case TokenType::lessThen:
				reg = emitBinop(*binop);
				emitJump("jge", elseLabel);
				regFree(reg);
				break;
			case TokenType::greaterThenEq:
				reg = emitBinop(*binop);
				emitJump("jl", elseLabel);
				regFree(reg);
				break;
			case TokenType::lessThenEq:
				reg = emitBinop(*binop);
				emitJump("jg", elseLabel);
				regFree(reg);
				break;
			case TokenType::and_: {
				auto andComp = [&](const ExprPtr& node) {
					if (isPrimitive(node)) {
						Register* regLhs = emitCmpZero(node);
						emitJump("je", elseLabel);
						regFree(regLhs);
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
					regFree(regLhs);
				} else if (const auto bop = cast::toBinop(binop->lhs)) {
					reg = emitBinop(*bop);
					emitJmpTrueLabel(reg, bop->opToken.type, trueLabel);
					regFree(reg);
				} else {
					emitTest(binop->lhs, trueLabel, elseLabel);
				}

				if (isPrimitive(binop->rhs)) {
					Register* regRhs = emitCmpZero(binop->rhs);
					emitJump("je", elseLabel);
					regFree(regRhs);
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
		regFree(reg);
	} else if (const auto var = cast::toVar(test)) {
		reg = emitLoadRegFromMem(*var, RegisterSize::reg64);
		emitInstr2op(reg->isSSE() ? "ucomisd" : "cmp", mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), 0);
		emitJump("je", elseLabel);
		regFree(reg);
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
				emitSet8L("sete", setReg);
				break;
			case TokenType::nequal:
				setReg = emitSetReg(*binop);
				emitSet8L("setne", setReg);
				break;
			case TokenType::greaterThen:
				setReg = emitSetReg(*binop);
				emitSet8L("setg", setReg);
				break;
			case TokenType::lessThen:
				setReg = emitSetReg(*binop);
				emitSet8L("setl", setReg);
				break;
			case TokenType::greaterThenEq:
				setReg = emitSetReg(*binop);
				emitSet8L("setge", setReg);
				break;
			case TokenType::lessThenEq:
				setReg = emitSetReg(*binop);
				emitSet8L("setle", setReg);
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
	} else if (const auto read = cast::toRead(set)) {
		return emitRead(*read);
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
		regFree(lhs.setReg);
	}

	if (rhs.reg->isSSE()) {
		regFree(rhs.setReg);
	}

	regFree(rhs.reg);
	return lhs.reg;
}

Register* CodeGen::emitSetReg(const BinOpExpr& binop) {
	const auto reg = emitBinop(binop);

	if (reg->isSSE()) {
		regFree(reg);
		return regAlloc();
	}

	return reg;
}

Register* CodeGen::emitCmpZero(const ExprPtr& node) {
	const ExprPtr zero = std::make_shared<IntExpr>(0);
	return emitExpr(node, zero, {.op = "cmp", .opSSE = "ucomisd"});
}

Register* CodeGen::emitAssignment(const ExprPtr& var, const RegisterSize size, const bool discardResult) {
	const auto var_ = cast::toVar(var);
	const std::string_view varName = cast::toString(var_->name)->data;

	if (const auto int_ = cast::toInt(var_->value)) {
		mov(getAddr(varName, var_->vType, var_->sType, RegisterSize::reg64), int_->n);

		if (!discardResult) {
			Register* reg = regAlloc();
			mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64),
			    getAddr(varName, var_->vType, var_->sType, RegisterSize::reg64));
			return reg;
		}

		return nullptr;
	}
	if (const auto double_ = cast::toDouble(var_->value)) {
		Register* reg = regAlloc();
		auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

		mov(regStr, emitHex(toHex(double_->n)));
		mov(getAddr(varName, var_->vType, var_->sType, RegisterSize::reg64), regStr);

		if (discardResult) {
			regFree(reg);
			return nullptr;
		}

		return reg;
	}
	if (const auto value = cast::toVar(var_->value)) {

		if (Register* reg = emitLoadRegFromMem(*value, size)) {
			emitStoreMemFromReg(varName, var_->vType, var_->sType, reg, size);

			if (!discardResult)
				return reg;
		}

		return nullptr;
	}
	if (cast::toNIL(var_->value)) {
		mov(getAddr(varName, var_->vType, var_->sType, RegisterSize::reg64), 0);

		if (!discardResult) {
			Register* reg = regAlloc();
			mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), 0);
			return reg;
		}

		return nullptr;
	}
	if (cast::toT(var_->value)) {
		mov(getAddr(varName, var_->vType, var_->sType, RegisterSize::reg64), 1);

		if (!discardResult) {
			Register* reg = regAlloc();
			auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);
			mov(regStr, 1);
			return reg;
		}

		return nullptr;
	}
	if (const auto str = cast::toString(var_->value)) {
		std::string label = ".L.";
		label += varName;
		std::string labelAddr = getAddr(label, var_->vType, var_->sType, size);
		std::string varAddr = getAddr(varName, var_->vType, var_->sType, size);

		updateSections("\nsection .rodata\n", {.name = label, .data = strDirective(str->data)});

		Register* reg = regAlloc();
		auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

		lea(regStr, labelAddr);
		mov(varAddr, regStr);

		if (discardResult) {
			regFree(reg);
			return nullptr;
		}

		return reg;
	}

	Register* reg = emitSet(var_->value);
	emitStoreMemFromReg(varName, var_->vType, var_->sType, reg, RegisterSize::reg64);

	if (discardResult) {
		regFree(reg);
		return nullptr;
	}

	return reg;
}

Register* CodeGen::emitLoadRegFromMem(const VarExpr& var, const RegisterSize size) {
	Register* reg = nullptr;
	const std::string_view varName = cast::toString(var.name)->data;

	switch (var.sType) {
		case SymbolType::param:
		case SymbolType::local:
		case SymbolType::global: {
			if (var.vType == VarType::int_) {
				reg = regAlloc();
				mov(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64),
				    getAddr(varName, var.vType, var.sType, size));
			} else if (var.vType == VarType::double_) {
				reg = mRegisterAllocator.alloc(RegisterType::sse);
				movsd(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64),
				      getAddr(varName, var.vType, var.sType, size));
			} else if (cast::toString(var.value)) {
				reg = regAlloc();
				lea(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64),
				    getAddr(varName, var.vType, var.sType, size));
			} else if (cast::toNIL(var.value) || cast::toT(var.value)) {
				reg = regAlloc();
				movzx(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64),
				      getAddr(varName, var.vType, var.sType, size));
			}
			break;
		}
		default:
			break;
	}

	return reg;
}

void CodeGen::emitStoreMemFromReg(const std::string_view varName,
                                  const VarType vtype,
                                  const SymbolType stype,
                                  const Register* reg,
                                  const RegisterSize size) {
	auto regStr = mRegisterAllocator.nameFromReg(reg, size);

	if (reg->isSSE()) {
		movsd(getAddr(varName, vtype, stype, size), regStr);
	} else {
		mov(getAddr(varName, vtype, stype, size), regStr);
	}
}

std::string CodeGen::getAddr(const std::string_view varName,
                             const VarType vtype,
                             const SymbolType stype,
                             const RegisterSize size) {
	switch (stype) {
		case SymbolType::global: {
			if (vtype == VarType::string) {
				return std::format("[rel {}]", varName);
			}
			return std::format("{} [rel {}]", mMemorySize[std::to_underlying(size)], varName);
		}
		case SymbolType::local: {
			if (vtype == VarType::string) {
				return std::format("[rbp - {}]",
				                   mStackAllocator.pushStackFrame(
					                   mCurrentScope,
					                   varName,
					                   stype,
					                   mMemorySizeInBytes[std::to_underlying(size)]));
			}
			return std::format("{} [rbp - {}]",
			                   mMemorySize[std::to_underlying(size)],
			                   mStackAllocator.pushStackFrame(
				                   mCurrentScope,
				                   varName,
				                   stype,
				                   mMemorySizeInBytes[std::to_underlying(size)]));
		}
		case SymbolType::param:
			return std::format("{} [rbp + {}]",
			                   mMemorySize[std::to_underlying(size)],
			                   mStackAllocator.pushStackFrame(
				                   mCurrentScope,
				                   varName,
				                   stype,
				                   mMemorySizeInBytes[std::to_underlying(size)]));
		default:
			throw std::runtime_error("Unknown SymbolType.");
	}
}

RegisterSize CodeGen::getMemSize(const ExprPtr& var) {
	auto var_ = cast::toVar(var);

	do {
		if (var_->vType == VarType::nil || var_->vType == VarType::t) {
			return RegisterSize::reg8l;
		}

		if (var_->vType == VarType::int_ || var_->vType == VarType::double_) {
			return RegisterSize::reg64;
		}

		var_ = cast::toVar(var_->value);
	} while (var_);

	return RegisterSize::zero;
}

void CodeGen::pushParamOntoStack(const std::string_view funcName, const ExprPtr& param) {
	const auto param_ = cast::toVar(param);
	const std::string paramName = cast::toString(param_->name)->data;

	const auto memSize = mMemorySize[std::to_underlying(getMemSize(param))];
	const auto memSizeInt = mMemorySizeInBytes[std::to_underlying(getMemSize(param))];

	const int32_t offset = mStackAllocator.pushStackFrame(funcName, paramName, SymbolType::param, memSizeInt);

	const std::string addr = offset - 16 // Because parameters are above 16 byte from rbp
		                         ? std::format("{} [rsp + {}]", memSize, offset - 16)
		                         : std::format("{} [rsp]", memSize);

	if (const auto int_ = cast::toInt(param_->value)) {
		mov(addr, int_->n);
	} else if (const auto double_ = cast::toDouble(param_->value)) {
		Register* regScr = regAlloc();
		auto regScrStr = mRegisterAllocator.nameFromReg(regScr, RegisterSize::reg64);

		mov(regScrStr, emitHex(toHex(double_->n)));
		mov(addr, regScrStr);

		regFree(regScr);
	}
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
