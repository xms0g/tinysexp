#include "codegen.h"
#include <format>

#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) generatedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) generatedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) generatedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) emitInstr1op(jmp, label)
#define ret() generatedCode += "\tret\n"
#define cqo() generatedCode += "\tcqo\n"
#define newLine() generatedCode += "\n";

#define stack_alloc(size) \
    emitInstr2op("sub", "rsp", size); \
    stackAllocator.alloc(size);

#define stack_dealloc(size) \
    emitInstr2op("add", "rsp", size); \
    stackAllocator.dealloc(size);

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

#define popInUseRegisters(registers, pop)                                   \
    for (const int paramRegister: registers) {                              \
        if (const auto* reg = registerAllocator.regFromID(paramRegister);   \
            isInUse(reg->status)) {                                         \
            pop(getRegName(reg, REG64));                                    \
        }                                                                   \
    }

#define mov(d, s) emitInstr2op("mov", d, s)
#define movd(d, s) emitInstr2op("movsd", d, s)
#define movzx(d, s) emitInstr2op("movzx", d, s)
#define strDirective(s) std::format("db \"{}\", 10", s)
#define memDirective(d, n) std::format("{} {}", d, n)

#define emitSet8L(op, rp) \
    emitInstr1op(op, getRegName(rp, REG8L)); \
    movzx(getRegName(rp, REG64), getRegName(rp, REG8L));

#define register_alloc() ([&]() {                           \
    auto* rp = registerAllocator.alloc();                   \
    if (rp && isPRESERVED(rp->rType)) {                     \
        push(getRegName(rp, REG64))                         \
    }                                                       \
    return rp;                                              \
    }())

#define register_free(rp)                                   \
    if (rp && !(rp->status >> INUSE_FOR_PARAM_IDX & 1)) {   \
            registerAllocator.free(rp);                     \
            if (isPRESERVED(rp->rType)) {                   \
                pop(getRegName(rp, REG64))                  \
            }                                               \
    }

Register* RegisterAllocator::alloc(uint8_t rt) {
    if (rt == SSE) {
        return scan(priorityOrderSSE, 2);
    }

    return scan(priorityOrder, 3);
}

void RegisterAllocator::free(Register* reg) {
    reg->status = NO_USE;
}

const char* RegisterAllocator::nameFromReg(const Register* reg, uint32_t size) {
    return registerNames[reg->id][size];
}

const char* RegisterAllocator::nameFromID(uint32_t id, uint32_t size) {
    return registerNames[id][size];
}

Register* RegisterAllocator::regFromName(const char* name, uint32_t size) {
    for (int i = 0; i < REGISTER_COUNT; i++) {
        if (std::strcmp(name, registerNames[i][size]) == 0) {
            return &registers[i];
        }
    }

    return nullptr;
}

Register* RegisterAllocator::regFromID(uint32_t id) {
    return &registers[id];
}

Register* RegisterAllocator::scan(const uint32_t* priorityOrder, int size) {
    for (int i = 0; i < size; ++i) {
        for (auto& register_: registers) {
            if (priorityOrder[i] == register_.rType && register_.status >> NO_USE_IDX & 1) {
                register_.status &= ~NO_USE;
                register_.status |= INUSE;
                return &register_;
            }
        }
    }

    return nullptr;
}

void StackAllocator::alloc(uint32_t size) {
    stackOffset += size;
}

void StackAllocator::dealloc(uint32_t size) {
    stackOffset -= size;
}

int StackAllocator::pushStackFrame(const std::string& funcName, const std::string& varName, SymbolType stype) {
    StackFrame* sf = nullptr;

    if (stack.contains(funcName)) {
        sf = &stack.at(funcName);

        if (sf->offsets.contains(varName)) {
            return sf->offsets.at(varName);
        }
    }

    if (!sf) {
        StackFrame stackFrame;

        const int offset = updateStackFrame(&stackFrame, varName, stype);
        stack.emplace(funcName, stackFrame);

        return offset;
    }

    return updateStackFrame(sf, varName, stype);
}

int StackAllocator::updateStackFrame(StackFrame* sf, const std::string& varName, SymbolType stype) {
    int offset;

    if (stype == SymbolType::LOCAL) {
        offset = sf->currentVarOffset;
        sf->currentVarOffset += 8;
    } else {
        offset = sf->currentParamOffset;
        sf->currentParamOffset += 8;
    }

    sf->offsets.emplace(varName, offset);

    stackOffset += 8;

    return offset;
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
        emitAST(next);
        next = next->child;
    }

    pop("rbp")
    ret();

    // Function definitions
    for (auto& [func, param]: functions) {
        (this->*func)(param);
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
            Register* rp1 = emitExpr(binop.lhs, negOne, {"xor", nullptr});
            Register* rp2 = emitExpr(binop.rhs, negOne, {"xor", nullptr});
            emitInstr2op("and", getRegName(rp1, REG64), getRegName(rp2, REG64));
            register_free(rp2)
            return rp1;
        }
        case TokenType::NOT: {
            const ExprPtr zero = std::make_shared<IntExpr>(0);
            return emitExpr(binop.lhs, zero, {"cmp", "ucomisd"});
        }
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
    emitSection(defconst.pair);
}

void CodeGen::emitDefun(const DefunExpr& defun) {
    const Register* reg = nullptr;
    const auto func = cast::toVar(defun.name);
    currentScope = cast::toString(func->name)->data;

    newLine();
    emitLabel(currentScope);
    push("rbp")
    mov("rbp", "rsp");

    for (auto& form: defun.forms) {
        reg = emitAST(form);
    }

    if (reg && isSSE(reg->rType) && reg->id != xmm0) {
        mov("xmm0", getRegName(reg, REG64));
    } else if (reg && !isSSE(reg->rType) && reg->id != RAX) {
        mov("rax", getRegName(reg, REG64));
    }

    auto clearInUseParamBit = [&](const auto& registers) {
        for (const int paramRegister: registers) {
            auto* r = registerAllocator.regFromID(paramRegister);
            r->status &= ~INUSE_FOR_PARAM;
        }
    };

    clearInUseParamBit(paramRegisters);
    clearInUseParamBit(paramRegistersSSE);

    pop("rbp")
    ret();
}

Register* CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {
    const auto func = cast::toVar(funcCall.name);
    currentScope = cast::toString(func->name)->data;

    // Calculate the proper stack size before function call
    const size_t stackParamsSize = funcCall.args.size() > 6 ? funcCall.args.size() - 6 : 0;
    const uint32_t stackOffset = stackAllocator.getOffset();
    uint32_t stackAlignedSize = stackOffset + stackParamsSize * 8;

    if (stackAlignedSize % 16 != 0)
        stackAlignedSize += 8;

    stack_alloc(stackAlignedSize)

    int scratchIdx = 0, sseIdx = 0, stackIdx = 0;
    for (const auto& arg: funcCall.args) {
        const auto param = cast::toVar(arg);
        const std::string paramName = cast::toString(param->name)->data;

        // If scratch param size > 5 or sse param size > 7, push the params onto stack
        if ((scratchIdx > 5 && cast::toInt(param->value)) ||
            (sseIdx > 7 && cast::toDouble(param->value))) {
            pushParamOntoStack(*param, stackIdx);
            continue;
        }
        // Push parameter to the appropriate register
        if (const auto int_ = cast::toInt(param->value)) {
            pushParamToRegister(paramName, paramRegisters[scratchIdx++], int_->n);
        } else if (const auto double_ = cast::toDouble(param->value)) {
            pushParamToRegister(paramName, paramRegistersSSE[sseIdx++], double_->n);
        }
    }

    emitInstr1op("call", currentScope);

    popInUseRegisters(paramRegisters, pop);
    popInUseRegisters(paramRegistersSSE, popxmm);

    stack_dealloc(stackAlignedSize)

    auto* rax = registerAllocator.regFromID(RAX);
    rax->status &= ~NO_USE;
    rax->status |= INUSE;

    return rax;
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
        register_free(reg)

        std::string done = createLabel();
        emitJump("jmp", done);
        emitLabel(elseLabel);

        reg = emitAST(if_.else_);
        emitLabel(done);
    } else {
        emitLabel(elseLabel);
    }

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
        auto* rp = register_alloc();
        mov(getRegName(rp, REG64), int_->n);
        return rp;
    }

    if (const auto double_ = cast::toDouble(n)) {
        auto* rp = registerAllocator.alloc(SSE);
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        movd(getRegName(rp, REG64), emitHex(hex));
        return rp;
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

    if (isSSE(regLhs->rType) && (isSCRATCH(regRhs->rType) || isPRESERVED(regRhs->rType))) {
        auto* newRP = registerAllocator.alloc(SSE);
        const char* newRPStr = getRegName(newRP, REG64);

        emitInstr2op("cvtsi2sd", newRPStr, getRegName(regRhs, REG64));
        register_free(regRhs)

        emitInstr2op(op.second, getRegName(regLhs, REG64), newRPStr);
        register_free(newRP);

        return regLhs;
    }

    if ((isSCRATCH(regLhs->rType) || isPRESERVED(regLhs->rType)) && isSSE(regRhs->rType)) {
        auto* newRP = registerAllocator.alloc(SSE);
        const char* newRPStr = getRegName(newRP, REG64);
        const char* reg2Str = getRegName(regRhs, REG64);

        emitInstr2op("cvtsi2sd", newRPStr, getRegName(regLhs, REG64));
        register_free(regLhs)

        emitInstr2op(op.second, newRPStr, reg2Str);
        movd(reg2Str, newRPStr);
        register_free(newRP);
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
        const bool isReg1NotRax = regLhs->id != RAX;

        if (isReg1NotRax) {
            if (regRhs->id == RAX) {
                auto* newRP = register_alloc();
                mov(getRegName(newRP, REG64), "rax");
                regRhs = newRP;
            } else {
                raxInUse = isInUse(registerAllocator.regFromID(RAX)->status);

                if (raxInUse) {
                    push("rax")
                }
            }
            mov("rax", getRegName(regLhs, REG64));
        }

        cqo();
        emitInstr1op("idiv", getRegName(regRhs, REG64));

        if (isReg1NotRax) {
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

void CodeGen::emitSection(const ExprPtr& var) {
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
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[REG8L], 0)));
    } else if (cast::toT(var_->value)) {
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[REG8L], 1)));
    } else if (const auto int_ = cast::toInt(var_->value)) {
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[REG64], int_->n)));
    } else if (const auto double_ = cast::toDouble(var_->value)) {
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[REG64], emitHex(hex))));
    } else if (cast::toVar(var_->value)) {
        const auto memSize = getMemSize(var_);

        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      memDirective(dataSizeInitialized[memSize], 0)));
        handleAssignment(var, memSize);
    } else if (const auto str = cast::toString(var_->value)) {
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data,
                                      strDirective(str->data)));
    }
}

void CodeGen::emitTest(const ExprPtr& test, std::string& trueLabel, std::string& elseLabel) {
    Register* rp;

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
                rp = emitBinop(*binop);
                emitInstr2op("cmp", getRegName(rp, REG64), 0);
                emitJump("je", elseLabel);
                register_free(rp)
                break;
            }
            case TokenType::EQUAL:
            case TokenType::NOT:
                rp = emitBinop(*binop);
                emitJump("jne", elseLabel);
                register_free(rp)
                break;
            case TokenType::NEQUAL:
                rp = emitBinop(*binop);
                emitJump("je", elseLabel);
                register_free(rp)
                break;
            case TokenType::GREATER_THEN:
                rp = emitBinop(*binop);
                emitJump("jle", elseLabel);
                register_free(rp)
                break;
            case TokenType::LESS_THEN:
                rp = emitBinop(*binop);
                emitJump("jge", elseLabel);
                register_free(rp)
                break;
            case TokenType::GREATER_THEN_EQ:
                rp = emitBinop(*binop);
                emitJump("jl", elseLabel);
                register_free(rp)
                break;
            case TokenType::LESS_THEN_EQ:
                rp = emitBinop(*binop);
                emitJump("jg", elseLabel);
                register_free(rp)
                break;
            case TokenType::AND: {
                const ExprPtr zero = std::make_shared<IntExpr>(0);

                auto* rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
                register_free(rp1)
                emitJump("je", elseLabel);

                auto* rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
                register_free(rp2)
                emitJump("je", elseLabel);
                break;
            }
            case TokenType::OR: {
                const ExprPtr zero = std::make_shared<IntExpr>(0);

                auto* rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
                register_free(rp1)
                emitJump("jne", trueLabel);

                auto* rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
                register_free(rp2)
                emitJump("je", elseLabel);

                emitLabel(trueLabel);
                break;
            }
            default:
                break;
        }
    } else if (const auto funcCall = cast::toFuncCall(test)) {
        rp = emitFuncCall(*funcCall);
        register_free(rp)
    } else if (auto var = cast::toVar(test)) {
        emitInstr2op("cmp", getAddr(cast::toString(var->name)->data, var->sType, REG64), 0);
        emitJump("je", elseLabel);
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
    const ExprPtr zero = std::make_shared<IntExpr>(0);

    auto* regLhs = emitExpr(binop.lhs, zero, {"cmp", "ucomisd"});
    Register* setRegLhs = isSSE(regLhs->rType) ? register_alloc() : regLhs;

    const char* setRegLhsStr = getRegName(setRegLhs, REG64);
    const char* setRegLhs8LStr = getRegName(setRegLhs, REG8L);

    emitInstr2op("xor", setRegLhsStr, setRegLhsStr);
    emitInstr1op("setne", setRegLhs8LStr);

    auto* regRhs = emitExpr(binop.rhs, zero, {"cmp", "ucomisd"});
    Register* setRegRhs = isSSE(regRhs->rType) ? register_alloc() : regRhs;

    const char* setRegRhsStr = getRegName(setRegRhs, REG64);
    const char* setRegRhs8LStr = getRegName(setRegRhs, REG8L);

    emitInstr2op("xor", setRegRhsStr, setRegRhsStr);
    emitInstr1op("setne", setRegRhs8LStr);

    emitInstr2op(op, setRegLhs8LStr, setRegRhs8LStr);
    movzx(setRegLhsStr, setRegLhs8LStr);

    if (isSSE(regLhs->rType)) {
        emitInstr2op("cvtsi2sd", getRegName(regLhs, REG64), setRegLhsStr);
        register_free(setRegLhs)
    }

    if (isSSE(regRhs->rType)) {
        register_free(setRegRhs)
    }

    register_free(regRhs)
    return regLhs;
}

Register* CodeGen::emitSetReg(const BinOpExpr& binop) {
    const auto rp = emitBinop(binop);

    if (isSSE(rp->rType)) {
        return register_alloc();
    }
    return rp;
}

void CodeGen::handleAssignment(const ExprPtr& var, uint32_t size) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (const auto int_ = cast::toInt(var_->value)) {
        handlePrimitive(*var_, "mov", std::to_string(int_->n));
    } else if (const auto double_ = cast::toDouble(var_->value)) {
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        handlePrimitive(*var_, "movsd", emitHex(hex));
    } else if (cast::toVar(var_->value)) {
        handleVariable(*var_, size);
    } else if (cast::toNIL(var_->value)) {
        handlePrimitive(*var_, "mov", std::to_string(0));
    } else if (cast::toT(var_->value)) {
        handlePrimitive(*var_, "mov", std::to_string(1));
    } else if (cast::toUninitialized(var_->value) && var_->sType == SymbolType::LOCAL) {
        getAddr(varName, var_->sType, REG64);
    } else if (const auto str = cast::toString(var_->value)) {
        std::string label = ".L." + varName;
        std::string labelAddr = getAddr(label, var_->sType, size);
        std::string varAddr = getAddr(varName, var_->sType, size);

        updateSections("\nsection .data\n",
                       std::make_pair(label, strDirective(str->data)));

        auto* rp = register_alloc();
        const char* rpStr = getRegName(rp, REG64);

        emitInstr2op("lea", rpStr, labelAddr);
        mov(varAddr, rpStr);
        register_free(rp)
    } else {
        auto* rp = emitSet(var_->value);
        emitStoreMemFromReg(varName, var_->sType, rp, REG64);
        register_free(rp)
    }
}

void CodeGen::handlePrimitive(const VarExpr& var, const char* instr, const std::string& value) {
    const std::string varName = cast::toString(var.name)->data;

    if (const std::string key = currentScope + varName;
        paramToRegisters.contains(key)) {
        const uint32_t rid = paramToRegisters.at(key);
        emitInstr2op(instr, registerAllocator.nameFromID(rid, REG64), value);
    } else {
        emitInstr2op(instr, getAddr(varName, var.sType, REG64), value);
    }
}

void CodeGen::handleVariable(const VarExpr& var, uint32_t size) {
    const std::string varName = cast::toString(var.name)->data;
    auto value = cast::toVar(var.value);

    do {
        if (Register* rp = emitLoadRegFromMem(*value, size)) {
            emitStoreMemFromReg(varName, var.sType, rp, size);

            if (value->sType != SymbolType::PARAM) {
                register_free(rp)
            }
        }

        value = cast::toVar(value->value);
    } while (value);
}

Register* CodeGen::emitLoadRegFromMem(const VarExpr& var, uint32_t size) {
    Register* rp = nullptr;
    const std::string varName = cast::toString(var.name)->data;

    switch (var.sType) {
        case SymbolType::PARAM: {
            if (const std::string key = currentScope + varName;
                paramToRegisters.contains(key)) {
                const uint32_t rid = paramToRegisters.at(key);
                rp = registerAllocator.regFromID(rid);
            } else {
                rp = register_alloc();

                mov(getRegName(rp, REG64), getAddr(varName, var.sType, size));
            }
            break;
        }
        case SymbolType::LOCAL:
        case SymbolType::GLOBAL: {
            if (cast::toInt(var.value)) {
                rp = register_alloc();
                mov(getRegName(rp, REG64), getAddr(varName, var.sType, size));
            } else if (cast::toDouble(var.value)) {
                rp = registerAllocator.alloc(SSE);
                movd(getRegName(rp, REG64), getAddr(varName, var.sType, size));
            } else if (cast::toString(var.value)) {
                rp = register_alloc();
                emitInstr2op("lea", getRegName(rp, REG64), getAddr(varName, var.sType, size));
            } else if (cast::toNIL(var.value) || cast::toT(var.value)) {
                rp = register_alloc();
                movzx(getRegName(rp, REG64), getAddr(varName, var.sType, size));
            }
            break;
        }
    }

    return rp;
}

void CodeGen::emitStoreMemFromReg(const std::string& varName, SymbolType stype, const Register* rp, uint32_t size) {
    const char* rpStr = getRegName(rp, size);

    if (isSCRATCH(rp->rType) || isPRESERVED(rp->rType)) {
        mov(getAddr(varName, stype, size), rpStr);
    } else if (isSSE(rp->rType)) {
        movd(getAddr(varName, stype, size), rpStr);
    }
}

std::string CodeGen::getAddr(const std::string& varName, SymbolType stype, uint32_t size) {
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

template<typename T>
void CodeGen::pushParamToRegister(const std::string& paramName, uint32_t rid, T value) {
    auto* reg = registerAllocator.regFromID(rid);

    if (isInUse(reg->status)) {
        if (isSSE(reg->rType)) {
            pushxmm(getRegName(reg, REG64));
        } else {
            push(getRegName(reg, REG64));
        }
    }

    reg->status &= ~NO_USE;
    reg->status |= INUSE_FOR_PARAM;

    mov(getRegName(reg, REG64), value);
    paramToRegisters.emplace(currentScope + paramName, reg->id);
}

void CodeGen::pushParamOntoStack(const VarExpr& param, int& stackIdx) {
    const std::string paramName = cast::toString(param.name)->data;

    stackAllocator.pushStackFrame(currentScope, paramName, param.sType);

    const std::string addr = stackIdx ? std::format("qword [rsp + {}]", stackIdx) : "qword [rsp]";

    if (const auto int_ = cast::toInt(param.value)) {
        mov(addr, int_->n);
    } else if (const auto double_ = cast::toDouble(param.value)) {
        movd(addr, double_->n);
    }

    stackIdx += 8;
}

const char* CodeGen::getRegName(const Register* reg, uint32_t size) {
    return registerAllocator.nameFromReg(reg, size);
}

const char* CodeGen::getRegNameByID(uint32_t id, uint32_t size) {
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
