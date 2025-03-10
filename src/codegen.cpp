#include "codegen.h"
#include <format>

#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) generatedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) generatedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) generatedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) emitInstr1op(jmp, label)
#define ret() generatedCode += "\tret\n"
#define cqo() generatedCode += "\tcqo\n"
#define newLine() generatedCode += "\n"
#define syscall() generatedCode += "\tsyscall\n"

#define isSSE(type) ((type >> SSE_IDX) & 1)
#define isSCRATCH(type) ((type >> SCRATCH_IDX) & 1)
#define isPRESERVED(type) ((type >> PRESERVED_IDX) & 1)
#define isINUSE(type) ((type >> INUSE_IDX) & 1)

#define stack_alloc(size) \
    if (size > 0) { \
        emitInstr2op("sub", "rsp", size); \
        stackAllocator.alloc(size); \
    }

#define stack_dealloc(size) \
    if (size > 0) { \
        emitInstr2op("add", "rsp", size); \
        stackAllocator.dealloc(size); \
    }

#define push(v) \
    emitInstr1op("push", v); \
    stackAllocator.alloc(8);

#define pop(rn) \
    emitInstr1op("pop", rn); \
    stackAllocator.dealloc(8);

#define pushxmm(xmm) \
    stack_alloc(16) \
    emitInstr2op("movdqu", "dqword [rsp]", xmm);

#define popxmm(xmm) \
    emitInstr2op("movdqu", xmm, "dqword [rsp]"); \
    stack_dealloc(16)

#define popInUseRegisters(registers, pop) \
    for (const int paramRegister: registers) { \
        if (const auto* reg = registerAllocator.regFromID(paramRegister); \
            isINUSE(reg->status)) { \
            pop(getRegName(reg, REG64)); \
        } \
    }

#define mov(d, s) emitInstr2op("mov", d, s)
#define movq(d, s) emitInstr2op("movq", d, s)
#define movsd(d, s) emitInstr2op("movsd", d, s)
#define movzx(d, s) emitInstr2op("movzx", d, s)
#define strDirective(s) std::format("db \"{}\", 10", s)
#define memDirective(d, n) std::format("{} {}", d, n)

#define emitSet8L(op, reg) \
    emitInstr1op(op, getRegName(reg, REG8L)); \
    movzx(getRegName(reg, REG64), getRegName(reg, REG8L));

#define register_alloc() ([&]() { \
    auto* reg = registerAllocator.alloc(); \
    if (reg && isPRESERVED(reg->rType)) { \
        push(getRegName(reg, REG64)) \
    } \
    return reg; \
    }())

#define register_free(reg) \
    if (reg) { \
        registerAllocator.free(reg); \
        if (isPRESERVED(reg->rType)) { \
            pop(getRegName(reg, REG64)) \
        } \
    }

std::string CodeGen::emit(const ExprPtr& ast) {
    generatedCode =
            "[bits 64]\n"
            "section .text\n"
            "\tglobal _start\n"
            "_start:\n";

    push("rbp")
    mov("rbp", "rsp");

    auto next = ast;
    while (next != nullptr) {
        auto* reg = emitAST(next);
        register_free(reg)
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
    for (auto& [func, defun]: functions) {
        const auto var = cast::toVar(defun.name);
        if (std::string funcName = cast::toString(var->name)->data;
            !calledFunctions.contains(funcName)) {
            continue;
        }

        (this->*func)(defun);
    }
    // Sections
    for (auto& [section, data]: sections) {
        generatedCode += section;

        for (auto& [name, size]: data) {
            generatedCode += std::format("{}: {}\n", name, size);
        }
    }

    return generatedCode;
}

Register* CodeGen::emitAST(const ExprPtr& ast) {
    if (const auto binop = cast::toBinop(ast)) {
        return emitBinop(*binop);
    } else if (const auto dotimes = cast::toDotimes(ast)) {
        return emitDotimes(*dotimes);
    } else if (const auto loop = cast::toLoop(ast)) {
        return emitLoop(*loop);
    } else if (const auto let = cast::toLet(ast)) {
        return emitLet(*let);
    } else if (const auto setq = cast::toSetq(ast)) {
        emitSetq(*setq);
    } else if (const auto defvar = cast::toDefvar(ast)) {
        emitDefvar(*defvar);
    } else if (const auto defconst = cast::toDefconstant(ast)) {
        emitDefconst(*defconst);
    } else if (const auto defun = cast::toDefun(ast)) {
        functions.emplace_back(&CodeGen::emitDefun, *defun);
    } else if (const auto funcCall = cast::toFuncCall(ast)) {
        return emitFuncCall(*funcCall);
    } else if (const auto if_ = cast::toIf(ast)) {
        return emitIf(*if_);
    } else if (const auto when = cast::toWhen(ast)) {
        return emitWhen(*when);
    } else if (const auto cond = cast::toCond(ast)) {
        return emitCond(*cond);
    }

    return nullptr;
}

Register* CodeGen::emitBinop(const BinOpExpr& binop) {
    switch (binop.opToken.type) {
        case TokenType::PLUS:
            return emitExpr(binop.lhs, binop.rhs, {"add", "addsd"});
        case TokenType::MINUS:
            return emitExpr(binop.lhs, binop.rhs, {"sub", "subsd"});
        case TokenType::DIV:
            return emitExpr(binop.lhs, binop.rhs, {"idiv", "divsd"});
        case TokenType::MUL:
            return emitExpr(binop.lhs, binop.rhs, {"imul", "mulsd"});
        case TokenType::LOGAND:
            return emitExpr(binop.lhs, binop.rhs, {"and", nullptr});
        case TokenType::LOGIOR:
            return emitExpr(binop.lhs, binop.rhs, {"or", nullptr});
        case TokenType::LOGXOR:
            return emitExpr(binop.lhs, binop.rhs, {"xor", nullptr});
        case TokenType::LOGNOR: {
            const ExprPtr negOne = std::make_shared<IntExpr>(-1);
            // Bitwise NOT seperately
            Register* regLhs = emitExpr(binop.lhs, negOne, {"xor", nullptr});
            Register* regRhs = emitExpr(binop.rhs, negOne, {"xor", nullptr});
            emitInstr2op("and", getRegName(regLhs, REG64), getRegName(regRhs, REG64));
            register_free(regRhs)
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
            return emitExpr(binop.lhs, binop.rhs, {"cmp", "ucomisd"});
        default:
            return nullptr;
    }
}

Register* CodeGen::emitDotimes(const DotimesExpr& dotimes) {
    Register* reg = nullptr;
    const auto iterVar = cast::toVar(dotimes.iterationCount);
    const std::string iterVarName = cast::toString(iterVar->name)->data;
    // Labels
    std::string trueLabel;
    std::string loopLabel = createLabel();
    std::string doneLabel = createLabel();
    // Loop condition
    ExprPtr name = iterVar->name;
    ExprPtr value = std::make_shared<IntExpr>(0);
    ExprPtr lhs = std::make_shared<VarExpr>(name, value, SymbolType::LOCAL);
    ExprPtr rhs = iterVar->value;
    auto token = Token{TokenType::LESS_THEN};
    ExprPtr test = std::make_shared<BinOpExpr>(lhs, rhs, token);
    // Address of iter var
    std::string iterVarAddr = getAddr(iterVarName, SymbolType::LOCAL, REG64);
    stack_alloc(memorySizeInBytes[REG64])
    // Set 0 to iter var
    mov(iterVarAddr, 0);
    // Loop label
    emitLabel(loopLabel);
    emitTest(test, trueLabel, doneLabel);
    // Emit statements
    for (auto& statement: dotimes.statements) {
        reg = emitAST(statement);
        register_free(reg)
    }
    // Increment iteration count
    auto* tempReg = register_alloc();
    const char* tempRegStr = getRegName(tempReg, REG64);

    mov(tempRegStr, iterVarAddr);
    emitInstr2op("add", tempRegStr, 1);
    mov(iterVarAddr, tempRegStr);

    register_free(tempReg)

    emitJump("jmp", loopLabel);
    emitLabel(doneLabel);

    stack_dealloc(memorySizeInBytes[REG64])

    return reg;
}

Register* CodeGen::emitLoop(const LoopExpr& loop) {
    Register* reg = nullptr;
    // Labels
    std::string trueLabel;
    std::string loopLabel = createLabel();
    std::string doneLabel = createLabel();

    emitLabel(loopLabel);

    bool hasReturn{false};
    for (auto& sexpr: loop.sexprs) {
        const auto when = cast::toWhen(sexpr);
        if (!when) {
            reg = emitAST(sexpr);
            register_free(reg)
            continue;
        }

        for (auto& form: when->then) {
            if (const auto return_ = cast::toReturn(form); !return_) {
                reg = emitAST(form);
                register_free(reg)
                continue;
            }

            emitTest(when->test, trueLabel, loopLabel);
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

    for (auto& var: let.bindings) {
        const int size = memorySizeInBytes[getMemSize(var)];
        requiredStackMem += size;
    }

    stack_alloc(requiredStackMem)

    for (auto& var: let.bindings) {
        const auto memSize = getMemSize(var);
        handleAssignment(var, memSize);
    }

    for (auto& sexpr: let.body) {
        reg = emitAST(sexpr);
        register_free(reg)
    }

    stack_dealloc(requiredStackMem)

    return reg;
}

void CodeGen::emitSetq(const SetqExpr& setq) {
    const auto memSize = getMemSize(setq.pair);
    handleAssignment(setq.pair, memSize);
}

void CodeGen::emitDefvar(const DefvarExpr& defvar) {
    emitSection(defvar.pair);
}

void CodeGen::emitDefconst(const DefconstExpr& defconst) {
    emitSection(defconst.pair, true);
}

void CodeGen::emitDefun(const DefunExpr& defun) {
    const Register* reg = nullptr;
    const auto func = cast::toVar(defun.name);
    currentScope = cast::toString(func->name)->data;

    newLine();
    emitLabel(currentScope);
    push("rbp")
    mov("rbp", "rsp");

    uint32_t stackSize = 0;
    int scratchIdx = 0, sseIdx = 0;
    for (auto& arg: defun.args) {
        const auto param = cast::toVar(arg);
        const std::string paramName = cast::toString(param->name)->data;

        if (cast::toInt(param->value)) {
            if (scratchIdx > 5)
                continue;
            scratchIdx++;
        } else if (cast::toDouble(param->value)) {
            if (sseIdx > 7)
                continue;
            sseIdx++;
        }

        stackSize += memorySizeInBytes[getMemSize(arg)];
        stackAllocator.pushStackFrame(currentScope, paramName, param->sType);
    }

    stack_alloc(stackSize)

    scratchIdx = 0, sseIdx = 0;
    for (const auto& arg: defun.args) {
        const auto param = cast::toVar(arg);
        const std::string paramName = cast::toString(param->name)->data;
        const bool isInt = cast::toInt(param->value) != nullptr;

        if ((isInt && scratchIdx > 5) || (cast::toDouble(param->value) && sseIdx > 7)) {
            continue;
        }

        mov(getAddr(paramName, param->sType, REG64), getRegNameByID(
                isInt ? paramRegisters[scratchIdx++] : paramRegistersSSE[sseIdx++], REG64));
    }

    for (auto& form: defun.forms) {
        reg = emitAST(form);
    }

    if (reg && isSSE(reg->rType) && reg->id != xmm0) {
        movsd("xmm0", getRegName(reg, REG64));
    } else if (reg && !isSSE(reg->rType) && reg->id != RAX) {
        mov("rax", getRegName(reg, REG64));
    }

    pop("rbp")
    stack_dealloc(stackSize)
    ret();
}

Register* CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {
    const auto func = cast::toVar(funcCall.name);
    currentScope = cast::toString(func->name)->data;
    calledFunctions.emplace(currentScope);

    // Calculate the proper stack size before function call
    uint32_t stackAlignedSize = stackAllocator.calculateRequiredStackSize(funcCall.args);
    stack_alloc(stackAlignedSize)

    int scratchIdx = 0, sseIdx = 0, stackIdx = 0;
    for (const auto& arg: funcCall.args) {
        const auto param = cast::toVar(arg);

        // If scratch param size > 5 or sse param size > 7, push the params onto stack
        if ((scratchIdx > 5 && cast::toInt(param->value)) ||
            (sseIdx > 7 && cast::toDouble(param->value))) {
            pushParamOntoStack(*param, stackIdx);
            continue;
        }
        // Push parameter to the appropriate register
        if (const auto int_ = cast::toInt(param->value)) {
            pushParamToRegister(paramRegisters[scratchIdx++], int_->n);
        } else if (const auto double_ = cast::toDouble(param->value)) {
            pushParamToRegister(paramRegistersSSE[sseIdx++], double_->n);
        }
    }

    emitInstr1op("call", currentScope);

    popInUseRegisters(paramRegisters, pop);
    popInUseRegisters(paramRegistersSSE, popxmm);

    stack_dealloc(stackAlignedSize)

    auto makeInUseRegister = [&](const uint32_t id) {
        auto* reg = registerAllocator.regFromID(id);
        reg->status &= ~NO_USE;
        reg->status |= INUSE;
        return reg;
    };

    if (cast::toDouble(funcCall.returnType)) {
        return makeInUseRegister(xmm0);
    }
    return makeInUseRegister(RAX);
}

Register* CodeGen::emitIf(const IfExpr& if_) {
    Register* reg = nullptr;
    std::string elseLabel = createLabel();
    std::string trueLabel = createLabel();
    // Emit test
    emitTest(if_.test, trueLabel, elseLabel);
    // Emit then
    reg = emitAST(if_.then);
    // Emit else
    if (!cast::toUninitialized(if_.else_)) {
        std::string done = createLabel();
        emitJump("jmp", done);
        emitLabel(elseLabel);

        register_free(reg)
        reg = emitAST(if_.else_);
        emitLabel(done);
    } else {
        emitLabel(elseLabel);
    }

    register_free(reg)
    return reg;
}

Register* CodeGen::emitWhen(const WhenExpr& when) {
    Register* reg = nullptr;
    std::string trueLabel;
    std::string doneLabel = createLabel();
    // Emit test
    emitTest(when.test, trueLabel, doneLabel);
    // Emit then
    for (auto& form: when.then) {
        reg = emitAST(form);
        register_free(reg)
    }
    emitLabel(doneLabel);

    return reg;
}

Register* CodeGen::emitCond(const CondExpr& cond) {
    Register* reg = nullptr;
    std::string trueLabel;
    std::string done = createLabel();

    for (const auto& [test, forms]: cond.variants) {
        std::string elseLabel = createLabel();
        emitTest(test, trueLabel, elseLabel);

        for (auto& form: forms) {
            reg = emitAST(form);
            register_free(reg)
        }

        emitJump("jmp", done);
        emitLabel(elseLabel);
    }
    emitLabel(done);

    return reg;
}

Register* CodeGen::emitNumb(const ExprPtr& n) {
    if (const auto int_ = cast::toInt(n)) {
        auto* reg = register_alloc();
        mov(getRegName(reg, REG64), int_->n);
        return reg;
    }

    if (const auto double_ = cast::toDouble(n)) {
        auto* reg = register_alloc();
        const char* regStr = getRegName(reg, REG64);

        auto* regSSE = registerAllocator.alloc(SSE);

        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);

        mov(regStr, emitHex(hex));
        movq(getRegName(regSSE, REG64), regStr);

        register_free(reg)

        return regSSE;
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

Register* CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op) {
    Register* regLhs = emitNode(lhs);
    Register* regRhs = emitNode(rhs);

    if (isSSE(regLhs->rType) && !isSSE(regRhs->rType)) {
        auto* newReg = registerAllocator.alloc(SSE);
        const char* newRegStr = getRegName(newReg, REG64);

        emitInstr2op("cvtsi2sd", newRegStr, getRegName(regRhs, REG64));
        register_free(regRhs)

        emitInstr2op(op.second, getRegName(regLhs, REG64), newRegStr);
        register_free(newReg);

        return regLhs;
    }

    if (!isSSE(regLhs->rType) && isSSE(regRhs->rType)) {
        auto* newReg = registerAllocator.alloc(SSE);
        const char* newRegStr = getRegName(newReg, REG64);
        const char* regRhsStr = getRegName(regRhs, REG64);

        emitInstr2op("cvtsi2sd", newRegStr, getRegName(regLhs, REG64));
        register_free(regLhs)

        emitInstr2op(op.second, newRegStr, regRhsStr);
        movsd(regRhsStr, newRegStr);
        register_free(newReg);
        return regRhs;
    }

    if (isSSE(regLhs->rType) && isSSE(regRhs->rType)) {
        emitInstr2op(op.second, getRegName(regLhs, REG64), getRegName(regRhs, REG64));
        register_free(regRhs);
        return regLhs;
    }

    // rax -> dividend
    // idiv divisor[register/memory]
    // We have to check out lhs isn't rax and divisor is in rax cases.
    if (std::strcmp(op.first, "idiv") == 0) {
        bool raxInUse{false};
        const bool isRegLhsNotRax = regLhs->id != RAX;

        if (isRegLhsNotRax) {
            if (regRhs->id == RAX) {
                auto* newReg = register_alloc();
                mov(getRegName(newReg, REG64), "rax");
                regRhs = newReg;
            } else {
                raxInUse = isINUSE(registerAllocator.regFromID(RAX)->status);

                if (raxInUse) {
                    push("rax")
                }
            }
            mov("rax", getRegName(regLhs, REG64));
        }

        cqo();
        emitInstr1op("idiv", getRegName(regRhs, REG64));

        if (isRegLhsNotRax) {
            mov(getRegName(regLhs, REG64), "rax");
        }

        if (raxInUse) {
            pop("rax")
        }
    } else {
        emitInstr2op(op.first, getRegName(regLhs, REG64), getRegName(regRhs, REG64));
    }

    register_free(regRhs);
    return regLhs;
}

void CodeGen::emitSection(const ExprPtr& var, const bool isConstant) {
    const auto var_ = cast::toVar(var);

    if (cast::toBinop(var_->value) || cast::toFuncCall(var_->value)) {
        updateSections("\nsection .bss\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeUninitialized[REG64], 1)));

        handleAssignment(var, REG64);
    } else if (cast::toUninitialized(var_->value)) {
        updateSections("\nsection .bss\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeUninitialized[REG64], 1)));
    } else if (cast::toNIL(var_->value)) {
        updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[REG8L], 0)));
    } else if (cast::toT(var_->value)) {
        updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[REG8L], 1)));
    } else if (const auto int_ = cast::toInt(var_->value)) {
        updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[REG64], int_->n)));
    } else if (const auto double_ = cast::toDouble(var_->value)) {
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[REG64], emitHex(hex))));
    } else if (cast::toVar(var_->value)) {
        const uint32_t memSize = getMemSize(var_);

        updateSections(isConstant ? "\nsection .rodata\n" : "\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[memSize], 0)));
        handleAssignment(var, memSize);
    } else if (const auto str = cast::toString(var_->value)) {
        updateSections("\nsection .rodata\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      strDirective(str->data)));
    }
}

void CodeGen::emitTest(const ExprPtr& test, std::string& trueLabel, std::string& elseLabel) {
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
                register_free(reg)
                break;
            }
            case TokenType::EQUAL:
            case TokenType::NOT:
                reg = emitBinop(*binop);
                emitJump("jne", elseLabel);
                register_free(reg)
                break;
            case TokenType::NEQUAL:
                reg = emitBinop(*binop);
                emitJump("je", elseLabel);
                register_free(reg)
                break;
            case TokenType::GREATER_THEN:
                reg = emitBinop(*binop);
                emitJump("jle", elseLabel);
                register_free(reg)
                break;
            case TokenType::LESS_THEN:
                reg = emitBinop(*binop);
                emitJump("jge", elseLabel);
                register_free(reg)
                break;
            case TokenType::GREATER_THEN_EQ:
                reg = emitBinop(*binop);
                emitJump("jl", elseLabel);
                register_free(reg)
                break;
            case TokenType::LESS_THEN_EQ:
                reg = emitBinop(*binop);
                emitJump("jg", elseLabel);
                register_free(reg)
                break;
            case TokenType::AND: {
                auto* regLhs = emitCmpZero(binop->lhs);
                register_free(regLhs)
                emitJump("je", elseLabel);

                auto* regRhs = emitCmpZero(binop->rhs);
                register_free(regRhs)
                emitJump("je", elseLabel);
                break;
            }
            case TokenType::OR: {
                auto* regLhs = emitCmpZero(binop->lhs);
                register_free(regLhs)
                emitJump("jne", trueLabel);

                auto* regRhs = emitCmpZero(binop->rhs);
                register_free(regRhs)
                emitJump("je", elseLabel);

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
        register_free(reg)
    } else if (const auto var = cast::toVar(test)) {
        reg = emitLoadRegFromMem(*var, REG64);
        emitInstr2op((isSSE(reg->rType) ? "ucomisd" : "cmp"), getRegName(reg, REG64), 0);
        emitJump("je", elseLabel);
        register_free(reg)
    } else if (cast::toNIL(test)) {
        emitJump("jmp", elseLabel);
    } else if (cast::toT(test)) {
        emitJump("jmp", trueLabel);
        emitLabel(trueLabel);
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

Register* CodeGen::emitLogOp(const BinOpExpr& binop, const char* op) {
    struct RegisterInfo {
        Register* reg{nullptr};
        Register* setReg{nullptr};
        const char* setRegStr{};
        const char* setReg8LStr{};
    };

    auto prepareRegister = [&](const ExprPtr& node, RegisterInfo& regInfo) {
        regInfo.reg = emitCmpZero(node);
        regInfo.setReg = isSSE(regInfo.reg->rType) ? register_alloc() : regInfo.reg;

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
        register_free(lhs.setReg)
    }

    if (isSSE(rhs.reg->rType)) {
        register_free(rhs.setReg)
    }

    register_free(rhs.reg)
    return lhs.reg;
}

Register* CodeGen::emitSetReg(const BinOpExpr& binop) {
    const auto reg = emitBinop(binop);

    if (isSSE(reg->rType)) {
        register_free(reg)
        return register_alloc();
    }
    return reg;
}

Register* CodeGen::emitCmpZero(const ExprPtr& node) {
    const ExprPtr zero = std::make_shared<IntExpr>(0);
    return emitExpr(node, zero, {"cmp", "ucomisd"});
}

void CodeGen::handleAssignment(const ExprPtr& var, const uint32_t size) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (const auto int_ = cast::toInt(var_->value)) {
        mov(getAddr(varName, var_->sType, REG64), int_->n);
    } else if (const auto double_ = cast::toDouble(var_->value)) {
        auto* reg = register_alloc();
        const char* regStr = getRegName(reg, REG64);

        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);

        mov(regStr, emitHex(hex));
        mov(getAddr(varName, var_->sType, REG64), regStr);
        register_free(reg)
    } else if (cast::toVar(var_->value)) {
        handleVariable(*var_, size);
    } else if (cast::toNIL(var_->value)) {
        mov(getAddr(varName, var_->sType, REG64), 0);
    } else if (cast::toT(var_->value)) {
        mov(getAddr(varName, var_->sType, REG64), 1);
    } else if (cast::toUninitialized(var_->value) && var_->sType == SymbolType::LOCAL) {
        getAddr(varName, var_->sType, REG64);
    } else if (const auto str = cast::toString(var_->value)) {
        std::string label = ".L." + varName;
        std::string labelAddr = getAddr(label, var_->sType, size);
        std::string varAddr = getAddr(varName, var_->sType, size);

        updateSections("\nsection .data\n",
                       std::make_pair(label, strDirective(str->data)));

        auto* reg = register_alloc();
        const char* regStr = getRegName(reg, REG64);

        emitInstr2op("lea", regStr, labelAddr);
        mov(varAddr, regStr);
        register_free(reg)
    } else {
        auto* reg = emitSet(var_->value);
        emitStoreMemFromReg(varName, var_->sType, reg, REG64);
        register_free(reg)
    }
}

void CodeGen::handleVariable(const VarExpr& var, uint32_t size) {
    const std::string varName = cast::toString(var.name)->data;
    auto value = cast::toVar(var.value);

    do {
        if (Register* reg = emitLoadRegFromMem(*value, size)) {
            emitStoreMemFromReg(varName, var.sType, reg, size);
            register_free(reg)
        }

        value = cast::toVar(value->value);
    } while (value);
}

Register* CodeGen::emitLoadRegFromMem(const VarExpr& var, const uint32_t size) {
    Register* reg = nullptr;
    const std::string varName = cast::toString(var.name)->data;

    switch (var.sType) {
        case SymbolType::PARAM: {
            reg = register_alloc();
            mov(getRegName(reg, REG64), getAddr(varName, var.sType, size));
            break;
        }
        case SymbolType::LOCAL:
        case SymbolType::GLOBAL: {
            if (cast::toInt(var.value)) {
                reg = register_alloc();
                mov(getRegName(reg, REG64), getAddr(varName, var.sType, size));
            } else if (cast::toDouble(var.value)) {
                reg = registerAllocator.alloc(SSE);
                movsd(getRegName(reg, REG64), getAddr(varName, var.sType, size));
            } else if (cast::toString(var.value)) {
                reg = register_alloc();
                emitInstr2op("lea", getRegName(reg, REG64), getAddr(varName, var.sType, size));
            } else if (cast::toNIL(var.value) || cast::toT(var.value)) {
                reg = register_alloc();
                movzx(getRegName(reg, REG64), getAddr(varName, var.sType, size));
            }
            break;
        }
    }

    return reg;
}

void CodeGen::emitStoreMemFromReg(const std::string& varName,
                                  const SymbolType stype,
                                  const Register* reg,
                                  const uint32_t size) {
    const char* regStr = getRegName(reg, size);

    if (isSSE(reg->rType)) {
        movsd(getAddr(varName, stype, size), regStr);
    } else {
        mov(getAddr(varName, stype, size), regStr);
    }
}

std::string CodeGen::getAddr(const std::string& varName, const SymbolType stype, const uint32_t size) {
    switch (stype) {
        case SymbolType::GLOBAL:
            return std::format("{} [rel {}]", memorySize[size], varName);
        case SymbolType::LOCAL:
            return std::format("{} [rbp - {}]",
                               memorySize[size],
                               stackAllocator.pushStackFrame(currentScope, varName, stype));
        case SymbolType::PARAM:
            return std::format("{} [rbp + {}]",
                               memorySize[size],
                               stackAllocator.pushStackFrame(currentScope, varName, stype));
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

void CodeGen::pushParamToRegister(const uint32_t rid, auto value) {
    const auto* reg = registerAllocator.regFromID(rid);
    const char* regStr = getRegName(reg, REG64);

    if (isINUSE(reg->status)) {
        if (isSSE(reg->rType)) {
            pushxmm(regStr)
        } else {
            push(regStr)
        }
    }

    if (isSSE(reg->rType)) {
        auto* regScr = register_alloc();
        const char* regScrStr = getRegName(regScr, REG64);

        uint64_t hex = *reinterpret_cast<uint64_t*>(&value);

        mov(regScrStr, emitHex(hex));
        movq(regStr, regScrStr);
        register_free(regScr)
    } else {
        mov(regStr, value);
    }
}

void CodeGen::pushParamOntoStack(const VarExpr& param, int& stackIdx) {
    const std::string paramName = cast::toString(param.name)->data;

    stackAllocator.pushStackFrame(currentScope, paramName, SymbolType::PARAM);

    const std::string addr = stackIdx ? std::format("qword [rsp + {}]", stackIdx) : "qword [rsp]";

    if (const auto int_ = cast::toInt(param.value)) {
        mov(addr, int_->n);
    } else if (const auto double_ = cast::toDouble(param.value)) {
        auto* regScr = register_alloc();
        const char* regScrStr = getRegName(regScr, REG64);

        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);

        mov(regScrStr, emitHex(hex));
        mov(addr, regScrStr);

        register_free(regScr)
    }

    stackIdx += 8;
}

const char* CodeGen::getRegName(const Register* reg, const uint32_t size) {
    return registerAllocator.nameFromReg(reg, size);
}

const char* CodeGen::getRegNameByID(const uint32_t id, const uint32_t size) {
    return registerAllocator.nameFromID(id, size);
}

std::string CodeGen::createLabel() {
    return ".L" + std::to_string(currentLabelCount++);
}

void CodeGen::updateSections(const char* name, const std::pair<std::string, std::string>& data) {
    if (!sections.contains(name)) {
        sections[name] = std::vector<std::pair<std::string, std::string> >();
    }

    sections.at(name).emplace_back(data.first, data.second);
}
