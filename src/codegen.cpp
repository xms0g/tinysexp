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

#define push(v) \
    emitInstr1op("push", v); \
    stackAllocator.alloc(8);

#define pop(rn) \
    emitInstr1op("pop", rn); \
    stackAllocator.dealloc(8);

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
    if (rp && (rp->rType >> PRESERVED_IDX & 1)) {           \
        push(getRegName(rp, REG64))                         \
    }                                                       \
    return rp;                                              \
    }())

#define register_free(rp)                                   \
    if (rp && !(rp->status >> INUSE_FOR_PARAM_IDX & 1)) {   \
            registerAllocator.free(rp);                     \
            if (rp->rType >> PRESERVED_IDX & 1) {           \
                pop(getRegName(rp, REG64))                  \
            }                                               \
                                                            \
    }

#define stack_alloc(size) \
    emitInstr2op("sub", "rsp", size); \
    stackAllocator.alloc(size);

#define stack_dealloc(size) \
    emitInstr2op("add", "rsp", size); \
    stackAllocator.dealloc(size);

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
    }

    if (const auto dotimes = cast::toDotimes(ast)) {
        emitDotimes(*dotimes);
    } else if (const auto loop = cast::toLoop(ast)) {
        emitLoop(*loop);
    } else if (const auto let = cast::toLet(ast)) {
        emitLet(*let);
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
        emitIf(*if_);
    } else if (const auto when = cast::toWhen(ast)) {
        emitWhen(*when);
    } else if (const auto cond = cast::toCond(ast)) {
        emitCond(*cond);
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

void CodeGen::emitDotimes(const DotimesExpr& dotimes) {
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
        emitAST(statement);
    }
    // Increment iteration count
    auto* rp = register_alloc();

    mov(getRegName(rp, REG64), iterVarAddr);
    emitInstr2op("add", getRegName(rp, REG64), 1);
    mov(iterVarAddr, getRegName(rp, REG64));

    register_free(rp)

    emitJump("jmp", loopLabel);
    emitLabel(doneLabel);

    stack_dealloc(memorySizeInBytes[REG64])
}

void CodeGen::emitLoop(const LoopExpr& loop) {
    // Labels
    std::string trueLabel;
    std::string loopLabel = createLabel();
    std::string doneLabel = createLabel();

    emitLabel(loopLabel);

    bool hasReturn{false};
    for (auto& sexpr: loop.sexprs) {
        const auto when = cast::toWhen(sexpr);
        if (!when) {
            emitAST(sexpr);
            continue;
        }

        for (auto& form: when->then) {
            if (const auto return_ = cast::toReturn(form); !return_) {
                emitAST(form);
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
}

void CodeGen::emitLet(const LetExpr& let) {
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
        emitAST(sexpr);
    }

    stack_dealloc(requiredStackMem)
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
    const Register* rv = nullptr;
    const auto func = cast::toVar(defun.name);
    const std::string funcName = cast::toString(func->name)->data;
    currentScope = funcName;

    newLine();
    emitLabel(funcName);
    push("rbp")
    mov("rbp", "rsp");

    for (auto& form: defun.forms) {
        rv = emitAST(form);
    }

    for (const int paramRegister : paramRegisters) {
        auto* reg = registerAllocator.regFromID(paramRegister);
        reg->status &= ~INUSE_FOR_PARAM;
    }

    for (const int sse : paramRegistersSSE) {
        auto* reg = registerAllocator.regFromID(sse);
        reg->status &= ~INUSE_FOR_PARAM;
    }

    if (rv->rType >> SSE_IDX & 1 && rv->id != xmm0) {
        mov("xmm0", getRegName(rv, REG64));
    } else if (!(rv->rType >> SSE_IDX & 1) && rv->id != RAX) {
        mov("rax", getRegName(rv, REG64));
    }

    pop("rbp")
    ret();
}

Register* CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {
    const auto func = cast::toVar(funcCall.name);
    const std::string funcName = cast::toString(func->name)->data;
    const size_t argCount = funcCall.args.size();

    const uint32_t stackOffset = stackAllocator.getOffset();
    uint32_t stackAlignedSize = stackOffset < 16 ? 16 : stackOffset + 8;
    stack_alloc(stackAlignedSize)

    for (int i = 0; i < argCount; ++i) {
        // If param size > 5, push the params onto stack
        if (i > 5) {
            for (int j = 6; j < argCount; ++j) {
                const auto arg_ = cast::toVar(funcCall.args[j]);
                const std::string argName = cast::toString(arg_->name)->data;

                stackAllocator.pushStackFrame(funcName, argName, arg_->sType);
            }

            for (size_t j = argCount - 1; j > 5; --j) {
                const auto arg_ = cast::toVar(funcCall.args[j]);
                push(cast::toInt(arg_->value)->n)
            }

            break;
        }

        const auto arg = cast::toVar(funcCall.args[i]);

        if (const auto int_ = cast::toInt(arg->value)) {
            auto* reg = registerAllocator.regFromID(paramRegisters[i]);
            if (reg->status >> INUSE_IDX & 1) {
                push(getRegName(reg, REG64));
            }
            reg->status &= ~NO_USE;
            reg->status |= INUSE_FOR_PARAM;

            paramToRegisters.emplace(funcName + cast::toString(arg->name)->data, reg->id);
            mov(getRegName(reg, REG64), int_->n);
        } else if (auto double_ = cast::toDouble(arg->value)) {
            paramToRegisters.emplace(funcName + cast::toString(arg->name)->data, paramRegistersSSE[i]);
        }
    }

    emitInstr1op("call", funcName);

    for (int i = 0; i < 6; ++i) {
        if (const auto* reg = registerAllocator.regFromID(paramRegisters[i]); reg->status >> INUSE_IDX & 1) {
            pop(getRegName(reg, REG64));
        }
    }

    stack_dealloc(stackAlignedSize)

    return registerAllocator.regFromID(RAX);
}

void CodeGen::emitIf(const IfExpr& if_) {
    std::string elseLabel = createLabel();
    std::string trueLabel = createLabel();
    // Emit test
    emitTest(if_.test, trueLabel, elseLabel);
    // Emit then
    emitAST(if_.then);
    // Emit else
    if (!cast::toUninitialized(if_.else_)) {
        std::string done = createLabel();
        emitJump("jmp", done);
        emitLabel(elseLabel);
        emitAST(if_.else_);
        emitLabel(done);
    } else {
        emitLabel(elseLabel);
    }
}

void CodeGen::emitWhen(const WhenExpr& when) {
    std::string trueLabel;
    std::string doneLabel = createLabel();
    // Emit test
    emitTest(when.test, trueLabel, doneLabel);
    // Emit then
    for (auto& form: when.then) {
        emitAST(form);
    }
    emitLabel(doneLabel);
}

void CodeGen::emitCond(const CondExpr& cond) {
    std::string trueLabel;
    std::string done = createLabel();

    for (const auto& [test, forms]: cond.variants) {
        std::string elseLabel = createLabel();
        emitTest(test, trueLabel, elseLabel);

        for (auto& form: forms) {
            emitAST(form);
        }

        emitJump("jmp", done);
        emitLabel(elseLabel);
    }
    emitLabel(done);
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
    Register* reg1 = emitNode(lhs);
    Register* reg2 = emitNode(rhs);

    if (reg1->rType >> SSE_IDX & 1 && (reg2->rType >> SCRATCH_IDX & 1 || reg2->rType >> PRESERVED_IDX & 1)) {
        auto* newRP = registerAllocator.alloc(SSE);
        const char* newRPStr = getRegName(newRP, REG64);

        emitInstr2op("cvtsi2sd", newRPStr, getRegName(reg2, REG64));
        register_free(reg2)

        emitInstr2op(op.second, getRegName(reg1, REG64), newRPStr);
        register_free(newRP);

        return reg1;
    }

    if ((reg1->rType >> SCRATCH_IDX & 1 || reg1->rType >> PRESERVED_IDX & 1) && reg2->rType >> SSE_IDX & 1) {
        auto* newRP = registerAllocator.alloc(SSE);
        const char* newRPStr = getRegName(newRP, REG64);
        const char* reg2Str = getRegName(reg2, REG64);

        emitInstr2op("cvtsi2sd", newRPStr, getRegName(reg1, REG64));
        register_free(reg1)

        emitInstr2op(op.second, newRPStr, reg2Str);
        movd(reg2Str, newRPStr);
        register_free(newRP);
        return reg2;
    }

    if (reg1->rType >> SSE_IDX & 1 && reg2->rType >> SSE_IDX & 1) {
        emitInstr2op(op.second, getRegName(reg1, REG64), getRegName(reg2, REG64));
        register_free(reg2);
        return reg1;
    }

    // rax -> dividend
    // idiv divisor[register/memory]
    // We have to check out reg1 isn't rax and divisor is rax cases.
    if (std::strcmp(op.first, "idiv") == 0) {
        const bool raxInUse = registerAllocator.regFromID(RAX)->status >> INUSE_IDX & 1;

        if (reg1->id != RAX) {
            if (reg2->id == RAX) {
                auto* newRP = register_alloc();
                mov(getRegName(newRP, REG64), "rax");
                mov("rax", getRegName(reg1, REG64));
                reg2 = newRP;
            } else {
                if (raxInUse) {
                    push("rax")
                }
                mov("rax", getRegName(reg1, REG64));
            }
        }

        cqo();
        emitInstr1op("idiv", getRegName(reg2, REG64));

        if (reg1->id != RAX) {
            mov(getRegName(reg1, REG64), "rax");
        }

        if (raxInUse) {
            pop("rax")
        }
    } else {
        emitInstr2op(op.first, getRegName(reg1, REG64), getRegName(reg2, REG64));
    }

    register_free(reg2);
    return reg1;
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
    } else if (auto funcCall = cast::toFuncCall(test)) {
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
                return emitLogAO(*binop, "and");
            case TokenType::OR:
                return emitLogAO(*binop, "or");
            default:
                break;
        }
    } else if (const auto funcCall = cast::toFuncCall(set)) {
        return emitFuncCall(*funcCall);
    }

    return setReg;
}

Register* CodeGen::emitLogAO(const BinOpExpr& binop, const char* op) {
    Register* setReg1,* setReg2;
    const ExprPtr zero = std::make_shared<IntExpr>(0);

    auto* rp1 = emitExpr(binop.lhs, zero, {"cmp", "ucomisd"});

    if (rp1->rType >> SSE_IDX & 1) {
        setReg1 = register_alloc();
    } else {
        setReg1 = rp1;
    }

    const char* setReg1Str = getRegName(setReg1, REG64);
    const char* setReg18LStr = getRegName(setReg1, REG8L);

    emitInstr2op("xor", setReg1Str, setReg1Str);
    emitInstr1op("setne", setReg18LStr);

    auto* rp2 = emitExpr(binop.rhs, zero, {"cmp", "ucomisd"});

    if (rp2->rType >> SSE_IDX & 1) {
        setReg2 = register_alloc();
    } else {
        setReg2 = rp2;
    }

    const char* setReg2Str = getRegName(setReg2, REG64);
    const char* setReg28LStr = getRegName(setReg2, REG8L);

    emitInstr2op("xor", setReg2Str, setReg2Str);
    emitInstr1op("setne", setReg28LStr);

    emitInstr2op(op, setReg18LStr, setReg28LStr);
    movzx(setReg1Str, setReg18LStr);

    if (rp1->rType >> SSE_IDX & 1) {
        emitInstr2op("cvtsi2sd", getRegName(rp1, REG64), setReg1Str);
        register_free(setReg1)
    }

    if (rp2->rType >> SSE_IDX & 1) {
        register_free(setReg2)
    }

    register_free(rp2)
    return rp1;
}

Register* CodeGen::emitSetReg(const BinOpExpr& binop) {
    const auto rp = emitBinop(binop);

    if (rp->rType >> SSE_IDX & 1) {
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

    if (const std::string key = currentScope + varName; paramToRegisters.contains(key)) {
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
            if (const std::string key = currentScope + varName; paramToRegisters.contains(key)) {
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

void CodeGen::emitStoreMemFromReg(const std::string& varName, SymbolType stype, Register* rp, uint32_t size) {
    const char* rpStr = getRegName(rp, size);

    if (rp->rType >> SCRATCH_IDX & 1 || rp->rType >> PRESERVED_IDX & 1) {
        mov(getAddr(varName, stype, size), rpStr);
    } else if (rp->rType >> SSE_IDX & 1) {
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
    } while (cast::toVar(var_));

    return 0;
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
