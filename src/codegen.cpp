#include "codegen.h"
#include <format>

#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) generatedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) generatedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) generatedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) emitInstr1op(jmp, label)
#define ret() generatedCode += "\tret\n"

#define push(id) emitInstr1op("push", getRegName(id, REG64))
#define pop(id) emitInstr1op("pop", getRegName(id, REG64))
#define mov(d, s) emitInstr2op("mov", d, s)
#define movd(d, s) emitInstr2op("movsd", d, s)
#define movzx(d, s) emitInstr2op("movzx", d, s)
#define strDirective(s) std::format("db \"{}\", 10", s)
#define memDirective(d, n) std::format("{} {}", d, n)

#define emitSet8L(op, rp) \
    emitInstr1op(op, getRegName(rp->id, REG8L)); \
    movzx(getRegName(rp->id, REG64), getRegName(rp->id, REG8L));

#define checkRType(type, t) ((type) & (t))

#define register_alloc(type) ([&]() {                       \
    auto* rp = rtracker.alloc(type);                        \
    if (checkRType(rp->rType, PRESERVED)) {                 \
        push(rp->id);                                       \
    }                                                       \
    return rp;                                              \
    }())
#define register_free(rp)                                   \
    rtracker.free(rp);                                      \
    if (checkRType(rp->rType, PRESERVED)) {                 \
        pop(rp->id);                                        \
    }

Register* RegisterTracker::alloc(uint8_t rtype) {
    for (auto& register_: registers) {
        if (checkRType(rtype, SSE) && !checkRType(register_.rType, SSE)) {
            continue;
        }
        if (checkRType(rtype, PARAM) && !checkRType(register_.rType, PARAM)) {
            continue;
        }

        if (!register_.inUse) {
            register_.inUse = true;
            return &register_;
        }
    }

    return nullptr;
}

void RegisterTracker::free(Register* reg) {
    reg->inUse = false;
}

const char* RegisterTracker::name(uint32_t id, int size) {
    return registerNames[id][size];
}

std::string CodeGen::emit(const ExprPtr& ast) {
    generatedCode =
            "[bits 64]\n"
            "section .text\n"
            "\tglobal _start\n"
            "_start:\n";

    emitInstr1op("push", "rbp");
    mov("rbp", "rsp");

    auto next = ast;
    while (next != nullptr) {
        emitAST(next);
        next = next->child;
    }

    emitInstr1op("pop", "rbp");
    ret();

    for (auto& [section, data]: sections) {
        generatedCode += section;

        for (auto& [name, size]: data) {
            generatedCode += std::format("{}: {}\n", name, size);
        }
    }

    return generatedCode;
}

void CodeGen::emitAST(const ExprPtr& ast) {
    if (const auto binop = cast::toBinop(ast)) {
        emitBinop(*binop);
    } else if (const auto dotimes = cast::toDotimes(ast)) {
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
    } else if (auto defun = cast::toDefun(ast)) {
        emitDefun(*defun);
    } else if (auto funcCall = cast::toFuncCall(ast)) {
        emitFuncCall(*funcCall);
    } else if (const auto if_ = cast::toIf(ast)) {
        emitIf(*if_);
    } else if (const auto when = cast::toWhen(ast)) {
        emitWhen(*when);
    } else if (const auto cond = cast::toCond(ast)) {
        emitCond(*cond);
    }
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
            ExprPtr negOne = std::make_shared<IntExpr>(-1);
            // Bitwise NOT seperately
            Register* rp1 = emitExpr(binop.lhs, negOne, {"xor", nullptr});
            Register* rp2 = emitExpr(binop.rhs, negOne, {"xor", nullptr});
            emitInstr2op("and", getRegName(rp1->id, REG64), getRegName(rp2->id, REG64));
            register_free(rp2)
            return rp1;
        }
        case TokenType::NOT: {
            ExprPtr zero = std::make_shared<IntExpr>(0);
            return emitExpr(binop.lhs, zero, {"cmp", "ucomisd"});
        }
        case TokenType::EQUAL:
        case TokenType::NEQUAL:
        case TokenType::GREATER_THEN:
        case TokenType::LESS_THEN:
        case TokenType::GREATER_THEN_EQ:
        case TokenType::LESS_THEN_EQ:
        case TokenType::AND:
        case TokenType::OR: //TODO: Check this
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
    auto* rp = register_alloc(SCRATCH);

    mov(getRegName(rp->id, REG64), iterVarAddr);
    emitInstr2op("add", getRegName(rp->id, REG64), 1);
    mov(iterVarAddr, getRegName(rp->id, REG64));

    register_free(rp)

    emitJump("jmp", loopLabel);
    emitLabel(doneLabel);
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
    for (auto& var: let.bindings) {
        const auto memSize = getMemSize(var);
        handleAssignment(var, memSize);
    }

    for (auto& sexpr: let.body) {
        emitAST(sexpr);
    }
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
}

Register* CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {
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
        auto* rp = register_alloc(SCRATCH);
        mov(getRegName(rp->id, REG64), int_->n);
        return rp;
    }

    if (const auto double_ = cast::toDouble(n)) {
        auto* rp = rtracker.alloc(SSE);
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        movd(getRegName(rp->id, REG64), emitHex(hex));
        return rp;
    }

    const auto var = cast::toVar(n);
    const std::string varName = cast::toString(var->name)->data;
    
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

    if (checkRType(reg1->rType, SSE) && (checkRType(reg2->rType, SCRATCH) || checkRType(reg2->rType, PRESERVED))) {
        auto* newRP = rtracker.alloc(SSE);
        const char* newRPStr = getRegName(newRP->id, REG64);

        emitInstr2op("cvtsi2sd", newRPStr, getRegName(reg2->id, REG64));
        register_free(reg2)

        emitInstr2op(op.second, getRegName(reg1->id, REG64), newRPStr);
        rtracker.free(newRP);
        return reg1;
    }
    if ((checkRType(reg1->rType, SCRATCH) || checkRType(reg1->rType, PRESERVED)) &&
        checkRType(reg2->rType, SSE)) {
        auto* newRP = rtracker.alloc(SSE);
        const char* newRPStr = getRegName(newRP->id, REG64);
        const char* reg2Str = getRegName(reg2->id, REG64);

        emitInstr2op("cvtsi2sd", newRPStr, getRegName(reg1->id, REG64));
        register_free(reg1)

        emitInstr2op(op.second, newRPStr, reg2Str);
        movd(reg2Str, newRPStr);
        rtracker.free(newRP);
        return reg2;
    }
    if (checkRType(reg1->rType, SSE) && checkRType(reg2->rType, SSE)) {
        emitInstr2op(op.second, getRegName(reg1->id, REG64), getRegName(reg2->id, REG64));
        rtracker.free(reg2);
        return reg1;
    }
    emitInstr2op(op.first, getRegName(reg1->id, REG64), getRegName(reg2->id, REG64));
    register_free(reg2)
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
            case TokenType::LOGXOR: {
                rp = emitBinop(*binop);
                emitInstr2op("cmp", getRegName(rp->id, REG64), 0);
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

    if (checkRType(rp1->rType, SSE)) {
        setReg1 = register_alloc(SCRATCH);
    } else {
        setReg1 = rp1;
    }

    const char* setReg1Str = getRegName(setReg1->id, REG64);
    const char* setReg18LStr = getRegName(setReg1->id, REG8L);

    emitInstr2op("xor", setReg1Str, setReg1Str);
    emitInstr1op("setne", setReg18LStr);

    auto* rp2 = emitExpr(binop.rhs, zero, {"cmp", "ucomisd"});

    if (checkRType(rp2->rType, SSE)) {
        setReg2 = register_alloc(SCRATCH);
    } else {
        setReg2 = rp2;
    }

    const char* setReg2Str = getRegName(setReg2->id, REG64);
    const char* setReg28LStr = getRegName(setReg2->id, REG8L);

    emitInstr2op("xor", setReg2Str, setReg2Str);
    emitInstr1op("setne", setReg28LStr);

    emitInstr2op(op, setReg18LStr, setReg28LStr);
    movzx(setReg1Str, setReg18LStr);

    if (checkRType(rp1->rType, SSE)) {
        emitInstr2op("cvtsi2sd", getRegName(rp1->id, REG64), setReg1Str);
        register_free(setReg1)
    }

    if (checkRType(rp2->rType, SSE)) {
        register_free(setReg2)
    }

    register_free(rp2)
    return rp1;
}

Register* CodeGen::emitSetReg(const BinOpExpr& binop) {
    const auto rp = emitBinop(binop);
    if (checkRType(rp->rType, SSE)) {
        return register_alloc(SCRATCH);
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

        auto* rp = register_alloc(SCRATCH);
        const char* rpStr = getRegName(rp->id, REG64);

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
    emitInstr2op(instr, getAddr(varName, var.sType, REG64), value);
}

void CodeGen::handleVariable(const VarExpr& var, uint32_t size) {
    const std::string varName = cast::toString(var.name)->data;
    auto value = cast::toVar(var.value);
    const std::string valueName = cast::toString(value->name)->data;

    Register* rp;

    do {
        rp = emitLoadRegFromMem(*value, size);

        if (rp) {
            emitStoreMemFromReg(varName, var.sType, rp, size);
        }

        value = cast::toVar(value->value);
    } while (cast::toVar(value));

    register_free(rp)
}

Register* CodeGen::emitLoadRegFromMem(const VarExpr& var, uint32_t size) {
    Register* rp = nullptr;
    const std::string& valueName = cast::toString(var.name)->data;

    if (cast::toInt(var.value)) {
        rp = register_alloc(SCRATCH);
        mov(getRegName(rp->id, REG64), getAddr(valueName, var.sType, size));
    } else if (cast::toDouble(var.value)) {
        rp = rtracker.alloc(SSE);
        movd(getRegName(rp->id, REG64), getAddr(valueName, var.sType, size));
    } else if (cast::toString(var.value)) {
        rp = register_alloc(SCRATCH);
        emitInstr2op("lea", getRegName(rp->id, REG64), getAddr(valueName, var.sType, size));
    } else if (cast::toNIL(var.value) || cast::toT(var.value)) {
        rp = register_alloc(SCRATCH);
        movzx(getRegName(rp->id, REG64), getAddr(valueName, var.sType, size));
    }

    return rp;
}

void CodeGen::emitStoreMemFromReg(const std::string& varName, SymbolType stype, Register* rp, uint32_t size) {
    const char* rpStr = getRegName(rp->id, size);

    if (checkRType(rp->rType, SCRATCH) || checkRType(rp->rType, PRESERVED)) {
        mov(getAddr(varName, stype, size), rpStr);
    } else if (checkRType(rp->rType, SSE)) {
        movd(getAddr(varName, stype, size), rpStr);
    }
}

std::string CodeGen::getAddr(const std::string& varName, SymbolType stype, uint32_t size) {
    switch (stype) {
        case SymbolType::LOCAL: {
            int stackOffset;
            if (stackOffsets.contains(varName)) {
                stackOffset = stackOffsets.at(varName);
            } else {
                stackOffset = currentStackOffset;
                stackOffsets.emplace(varName, currentStackOffset);
                currentStackOffset += 8;
            }
            return std::format("{} [rbp - {}]", memorySize[size], stackOffset);
        }
        case SymbolType::GLOBAL:
            return std::format("{} [rel {}]", memorySize[size], varName);
        case SymbolType::PARAM:
            throw std::runtime_error("PARAM handling not implemented.");
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

const char* CodeGen::getRegName(uint32_t id, uint32_t size) {
    switch (size) {
        case REG64:
            return rtracker.name(id, REG64);
        case REG32:
            return rtracker.name(id, REG32);
        case REG16:
            return rtracker.name(id, REG16);
        case REG8H:
            return rtracker.name(id, REG8H);
        case REG8L:
            return rtracker.name(id, REG8L);
        default:
            return "";
    }
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
