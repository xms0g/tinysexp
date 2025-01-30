#include "codegen.h"
#include <format>

#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) generatedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) generatedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) generatedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) emitInstr1op(jmp, label)

#define push(id) emitInstr1op("push", register64(id))
#define pop(id) emitInstr1op("pop", register64(id))
#define mov(d, s) emitInstr2op("mov", d, s)
#define movd(d, s) emitInstr2op("movsd", d, s)
#define strDirective(s) std::format("db \"{}\", 10", s)
#define quadDirective(state, n) std::format("{} {}", state, n)
#define byteDirective(n) std::format("db {}", n)
#define ret() generatedCode += "\tret\n"

#define emitSet8L(op, rp) \
    emitInstr1op(op, register8L(rp->id)); \
    emitInstr2op("movzx", register64(rp->id), register8L(rp->id));

#define checkRType(type, t) ((type) & (t))
#define register64(id) rtracker.name(id, REG64)
#define register32(id) rtracker.name(id, REG32)
#define register16(id) rtracker.name(id, REG16)
#define register8H(id) rtracker.name(id, REG8H)
#define register8L(id) rtracker.name(id, REG8L)

#define register_alloc(type) ([&]() {                       \
    auto* rp = rtracker.alloc(type);                        \
    if (checkRType(rp->rType, PRESERVED)) {                 \
        push(rp->id);                                       \
    }                                                       \
    return rp;                                              \
    }())                                                    \

#define register_free(rp)                                   \
    rtracker.free(rp);                                      \
    if (checkRType(rp->rType, PRESERVED)) {                 \
        pop(rp->id);                                        \
    }

Register* RegisterTracker::alloc(uint8_t rtype) {
    for (auto& register_: registers) {
        if (checkRType(rtype, SSE)) {
            if (!checkRType(register_.rType, SSE))
                continue;
        } else if (checkRType(rtype, PARAM)) {
            if (!checkRType(register_.rType, PARAM))
                continue;
        }

        if (!register_.inUse) {
            register_.inUse = true;
            return &register_;
        }
    }
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

    push(RBP);
    mov(register64(RBP), register64(RSP));

    auto next = ast;
    while (next != nullptr) {
        emitAST(next);
        next = next->child;
    }

    pop(RBP);
    ret();

    for (auto& section: sections) {
        generatedCode += section.first;
        for (auto& pair: section.second) {
            generatedCode += std::format("{}: {}\n", pair.first, pair.second);
        }
    }

    return generatedCode;
}

void CodeGen::emitAST(const ExprPtr& ast) {
    if (auto binop = cast::toBinop(ast)) {
        emitBinop(*binop);
    } else if (auto dotimes = cast::toDotimes(ast)) {
        emitDotimes(*dotimes);
    } else if (auto loop = cast::toLoop(ast)) {
        emitLoop(*loop);
    } else if (auto let = cast::toLet(ast)) {
        emitLet(*let);
    } else if (auto setq = cast::toSetq(ast)) {
        emitSetq(*setq);
    } else if (auto defvar = cast::toDefvar(ast)) {
        emitDefvar(*defvar);
    } else if (auto defconst = cast::toDefconstant(ast)) {
        emitDefconst(*defconst);
    } else if (auto defun = cast::toDefun(ast)) {
        emitDefun(*defun);
    } else if (auto funcCall = cast::toFuncCall(ast)) {
        emitFuncCall(*funcCall);
    } else if (auto if_ = cast::toIf(ast)) {
        emitIf(*if_);
    } else if (auto when = cast::toWhen(ast)) {
        emitWhen(*when);
    } else if (auto cond = cast::toCond(ast)) {
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
            emitInstr2op("and", register64(rp1->id), register64(rp2->id));
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
    Token token = Token{TokenType::LESS_THEN};
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

    mov(register64(rp->id), iterVarAddr);
    emitInstr2op("add", register64(rp->id), 1);
    mov(iterVarAddr, register64(rp->id));

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
        auto when = cast::toWhen(sexpr);
        if (!when) {
            emitAST(sexpr);
            continue;
        }

        for (auto& form: when->then) {
            auto return_ = cast::toReturn(form);
            if (!return_) {
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
        auto sizeInfo = getSize(var);
        handleAssignment(var, sizeInfo.first);
    }

    for (auto& sexpr: let.body) {
        emitAST(sexpr);
    }
}

void CodeGen::emitSetq(const SetqExpr& setq) {
    auto sizeInfo = getSize(setq.pair);
    handleAssignment(setq.pair, sizeInfo.first);
}

void CodeGen::emitDefvar(const DefvarExpr& defvar) {
    emitSection(defvar.pair);
}

void CodeGen::emitDefconst(const DefconstExpr& defconst) {
    emitSection(defconst.pair);
}

void CodeGen::emitDefun(const DefunExpr& defun) {}

Register* CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {}

void CodeGen::emitIf(const IfExpr& if_) {
    std::string elseLabel = createLabel();
    std::string trueLabel = createLabel();
    // Emit test
    emitTest(if_.test, trueLabel, elseLabel);
    // Emit then
    emitAST(if_.then);
    // Emit else
    if (cast::toNIL(if_.else_)) {
        emitLabel(elseLabel);
        return;
    }
    std::string done = createLabel();
    emitJump("jmp", done);
    emitLabel(elseLabel);
    emitAST(if_.else_);
    emitLabel(done);
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

    for (auto& pair: cond.variants) {
        std::string elseLabel = createLabel();
        emitTest(pair.first, trueLabel, elseLabel);
        for (auto& form: pair.second) {
            emitAST(form);
        }

        emitJump("jmp", done);
        emitLabel(elseLabel);
    }
    emitLabel(done);
}

Register* CodeGen::emitNumb(const ExprPtr& n) {
    if (auto int_ = cast::toInt(n)) {
        auto* rp = register_alloc(SCRATCH);
        mov(register64(rp->id), int_->n);
        return rp;
    } else if (auto double_ = cast::toDouble(n)) {
        auto* rp = rtracker.alloc(SSE);
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        movd(register64(rp->id), emitHex(hex));
        return rp;
    } else {
        const auto var = cast::toVar(n);
        const std::string varName = cast::toString(var->name)->data;

        return emitLoadRegFromMem(*var, varName, REG64);
    }
}

Register* CodeGen::emitNode(const ExprPtr& node) {
    if (auto binOp = cast::toBinop(node)) {
        return emitBinop(*binOp);
    } else if (auto funcCall = cast::toFuncCall(node)) {
        return emitFuncCall(*funcCall);
    } else {
        return emitNumb(node);
    }
}

Register* CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op) {
    Register* reg1;
    Register* reg2;

    reg1 = emitNode(lhs);
    reg2 = emitNode(rhs);

    if (checkRType(reg1->rType, SSE) && (checkRType(reg2->rType, SCRATCH) || checkRType(reg2->rType, PRESERVED))) {
        auto* newRP = rtracker.alloc(SSE);
        const char* newRPStr = register64(newRP->id);

        emitInstr2op("cvtsi2sd", newRPStr, register64(reg2->id));
        register_free(reg2)

        emitInstr2op(op.second, register64(reg1->id), newRPStr);
        rtracker.free(newRP);
        return reg1;
    } else if ((checkRType(reg1->rType, SCRATCH) || checkRType(reg1->rType, PRESERVED)) &&
               checkRType(reg2->rType, SSE)) {
        auto* newRP = rtracker.alloc(SSE);
        const char* newRPStr = register64(newRP->id);
        const char* reg2Str = register64(reg2->id);

        emitInstr2op("cvtsi2sd", newRPStr, register64(reg1->id));
        register_free(reg1)

        emitInstr2op(op.second, newRPStr, reg2Str);
        movd(reg2Str, newRPStr);
        rtracker.free(newRP);
        return reg2;
    } else if (checkRType(reg1->rType, SSE) && checkRType(reg2->rType, SSE)) {
        emitInstr2op(op.second, register64(reg1->id), register64(reg2->id));
        rtracker.free(reg2);
        return reg1;
    } else {
        emitInstr2op(op.first, register64(reg1->id), register64(reg2->id));
        register_free(reg2)
        return reg1;
    }
}

void CodeGen::emitSection(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);

    if (cast::toBinop(var_->value) || cast::toFuncCall(var_->value)) {
        updateSections("\nsection .bss\n",
                       std::make_pair(cast::toString(var_->name)->data, quadDirective("resq", 1)));

        handleAssignment(var, REG64);
    } else if (cast::toUninitialized(var_->value)) {
        updateSections("\nsection .bss\n",
                       std::make_pair(cast::toString(var_->name)->data, quadDirective("resq", 1)));
    } else if (cast::toNIL(var_->value)) {
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data, byteDirective(0)));
    } else if (cast::toT(var_->value)) {
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data, byteDirective(1)));
    } else if (auto int_ = cast::toInt(var_->value)) {
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data, quadDirective("dq", int_->n)));
    } else if (auto double_ = cast::toDouble(var_->value)) {
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data, quadDirective("dq", emitHex(hex))));
    } else if (cast::toVar(var_->value)) {
        auto sizeInfo = getSize(var_);

        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data, sizeInfo.second));

        handleAssignment(var, sizeInfo.first);
    } else if (auto str = cast::toString(var_->value)) {
        updateSections("\nsection .data\n",
                       std::make_pair(cast::toString(var_->name)->data, strDirective(str->data)));
    }
}

void CodeGen::emitTest(const ExprPtr& test, std::string& trueLabel, std::string& elseLabel) {
    Register* rp;

    if (auto binop = cast::toBinop(test)) {
        if (binop->opToken.type == TokenType::AND) {
            ExprPtr zero = std::make_shared<IntExpr>(0);

            auto* rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
            register_free(rp1)
            emitJump("je", elseLabel);

            auto* rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
            register_free(rp2)
            emitJump("je", elseLabel);

            return;
        }

        if (binop->opToken.type == TokenType::OR) {
            ExprPtr zero = std::make_shared<IntExpr>(0);

            auto* rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
            register_free(rp1)
            emitJump("jne", trueLabel);

            auto* rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
            register_free(rp2)
            emitJump("je", elseLabel);

            emitLabel(trueLabel);

            return;
        }

        rp = emitBinop(*binop);

        switch (binop->opToken.type) {
            case TokenType::PLUS:
            case TokenType::MINUS:
            case TokenType::DIV:
            case TokenType::MUL:
            case TokenType::LOGAND:
            case TokenType::LOGIOR:
            case TokenType::LOGXOR: {
                emitInstr2op("cmp", register64(rp->id), 0);
                emitJump("je", elseLabel);
                break;
            }
            case TokenType::EQUAL:
            case TokenType::NOT:
                emitJump("jne", elseLabel);
                break;
            case TokenType::NEQUAL:
                emitJump("je", elseLabel);
                break;
            case TokenType::GREATER_THEN:
                emitJump("jle", elseLabel);
                break;
            case TokenType::LESS_THEN:
                emitJump("jge", elseLabel);
                break;
            case TokenType::GREATER_THEN_EQ:
                emitJump("jl", elseLabel);
                break;
            case TokenType::LESS_THEN_EQ:
                emitJump("jg", elseLabel);
                break;
        }
        register_free(rp)
    } else if (auto funcCall = cast::toFuncCall(test)) {
        rp = emitFuncCall(*funcCall);
        register_free(rp)
    } else if (auto var = cast::toVar(test)) {
        emitInstr2op("cmp", getAddr(cast::toString(var->name)->data, var->sType, REG64), 0);
        emitJump("je", elseLabel);
    } else if (cast::toNIL(test)) {
        emitJump("jmp", elseLabel);
    }
}

Register* CodeGen::emitSet(const ExprPtr& set) {
    Register* rp, * setReg;

    if (auto binop = cast::toBinop(set)) {
        if (binop->opToken.type == TokenType::AND) {
            return emitLogAO(*binop, "and");
        }

        if (binop->opToken.type == TokenType::OR) {
            return emitLogAO(*binop, "or");
        }

        rp = emitBinop(*binop);

        if (checkRType(rp->rType, SSE)) {
            setReg = register_alloc(SCRATCH);
        } else {
            setReg = rp;
        }

        switch (binop->opToken.type) {
            case TokenType::EQUAL:
            case TokenType::NOT:
                emitSet8L("sete", setReg)
                break;
            case TokenType::NEQUAL:
                emitSet8L("setne", setReg)
                break;
            case TokenType::GREATER_THEN:
                emitSet8L("setg", setReg)
                break;
            case TokenType::LESS_THEN:
                emitSet8L("setl", setReg)
                break;
            case TokenType::GREATER_THEN_EQ:
                emitSet8L("setge", setReg)
                break;
            case TokenType::LESS_THEN_EQ:
                emitSet8L("setle", setReg)
                break;
        }

    } else if (auto funcCall = cast::toFuncCall(set)) {
        rp = emitFuncCall(*funcCall);
    }

    return setReg;
}

Register* CodeGen::emitLogAO(const BinOpExpr& binop, const char* op) {
    Register* setReg1, * setReg2;
    ExprPtr zero = std::make_shared<IntExpr>(0);

    auto* rp1 = emitExpr(binop.lhs, zero, {"cmp", "ucomisd"});

    if (checkRType(rp1->rType, SSE)) {
        setReg1 = register_alloc(SCRATCH);
    } else {
        setReg1 = rp1;
    }

    const char* setReg1Str = register64(setReg1->id);
    const char* setReg18LStr = register8L(setReg1->id);

    emitInstr2op("xor", setReg1Str, setReg1Str);
    emitInstr1op("setne", setReg18LStr);

    auto* rp2 = emitExpr(binop.rhs, zero, {"cmp", "ucomisd"});

    if (checkRType(rp2->rType, SSE)) {
        setReg2 = register_alloc(SCRATCH);
    } else {
        setReg2 = rp2;
    }

    const char* setReg2Str = register64(setReg2->id);
    const char* setReg28LStr = register8L(setReg2->id);

    emitInstr2op("xor", setReg2Str, setReg2Str);
    emitInstr1op("setne", setReg28LStr);

    emitInstr2op(op, setReg18LStr, setReg28LStr);
    emitInstr2op("movzx", setReg1Str, setReg18LStr);

    if (checkRType(rp1->rType, SSE)) {
        emitInstr2op("cvtsi2sd", register64(rp1->id), setReg1Str);
        register_free(setReg1)
    }

    if (checkRType(rp2->rType, SSE)) {
        register_free(setReg2)
    }

    register_free(rp2)
    return rp1;
}

void CodeGen::handleAssignment(const ExprPtr& var, uint32_t size) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (auto int_ = cast::toInt(var_->value)) {
        handlePrimitive(*var_, varName, "mov", std::to_string(int_->n));
    } else if (auto double_ = cast::toDouble(var_->value)) {
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        handlePrimitive(*var_, varName, "movsd", emitHex(hex));
    } else if (cast::toVar(var_->value)) {
        handleVariable(*var_, varName, size);
    } else if (cast::toNIL(var_->value)) {
        handlePrimitive(*var_, varName, "mov", std::to_string(0));
    } else if (cast::toT(var_->value)) {
        handlePrimitive(*var_, varName, "mov", std::to_string(1));
    } else if (cast::toUninitialized(var_->value) && var_->sType == SymbolType::LOCAL) {
        getAddr(varName, var_->sType, REG64);
    } else if (auto str = cast::toString(var_->value)) {
        std::string label = ".L." + varName;
        std::string labelAddr = getAddr(label, var_->sType, size);
        std::string varAddr = getAddr(varName, var_->sType, size);

        updateSections("\nsection .data\n",
                       std::make_pair(label, strDirective(str->data)));

        auto* rp = register_alloc(SCRATCH);
        const char* rpStr = register64(rp->id);

        emitInstr2op("lea", rpStr, labelAddr);
        mov(varAddr, rpStr);
        register_free(rp)
    } else {
        auto* rp = emitSet(var_->value);
        emitStoreMemFromReg(varName, var_->sType, rp, REG64);
        register_free(rp)
    }
}

void CodeGen::handlePrimitive(const VarExpr& var, const std::string& varName, const char* instr,
                              const std::string& value) {
    emitInstr2op(instr, getAddr(varName, var.sType, REG64), value);
}

void CodeGen::handleVariable(const VarExpr& var, const std::string& varName, uint32_t size) {
    auto value = cast::toVar(var.value);
    std::string valueName = cast::toString(value->name)->data;

    Register* rp;

    do {
        rp = emitLoadRegFromMem(*value, valueName, size);

        if (rp) {
            emitStoreMemFromReg(varName, var.sType, rp, size);
        }

        value = cast::toVar(value->value);
    } while (cast::toVar(value));

    register_free(rp)
}

Register* CodeGen::emitLoadRegFromMem(const VarExpr& value, const std::string& valueName, uint32_t size) {
    Register* rp = nullptr;

    if (cast::toInt(value.value)) {
        rp = register_alloc(SCRATCH);
        mov(register64(rp->id), getAddr(valueName, value.sType, size));
    } else if (cast::toDouble(value.value)) {
        rp = rtracker.alloc(SSE);
        movd(register64(rp->id), getAddr(valueName, value.sType, size));
    } else if (cast::toString(value.value)) {
        rp = register_alloc(SCRATCH);
        emitInstr2op("lea", register64(rp->id), getAddr(valueName, value.sType, size));
    } else if (cast::toNIL(value.value) || cast::toT(value.value)) {
        rp = register_alloc(SCRATCH);
        emitInstr2op("movzx", register64(rp->id), getAddr(valueName, value.sType, size));
    }

    return rp;
}

void CodeGen::emitStoreMemFromReg(const std::string& varName, SymbolType stype, Register* rp, uint32_t size) {
    const char* rpStr = size == REG64 ? register64(rp->id) :
                        size == REG32 ? register32(rp->id) :
                        size == REG16 ? register16(rp->id) :
                        size == REG8H ? register8H(rp->id) : register8L(rp->id);

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
            return std::format("{} [rbp - {}]", dataSize[size], stackOffset);
        }
        case SymbolType::GLOBAL:
            return std::format("{} [rel {}]", dataSize[size], varName);
        case SymbolType::PARAM:
            throw std::runtime_error("PARAM handling not implemented.");
        default:
            throw std::runtime_error("Unknown SymbolType.");
    }
}

std::pair<uint32_t , std::string> CodeGen::getSize(const ExprPtr& var) {
    auto var_ = cast::toVar(var);

    do {
        if (cast::toNIL(var_->value) || cast::toT(var_->value)) {
            return std::make_pair(REG8L, byteDirective(0));
        } else if (cast::toInt(var_->value) || cast::toDouble(var_->value)) {
            return std::make_pair(REG64, quadDirective("dq", 0));
        }

        var_ = cast::toVar(var_->value);
    } while (cast::toVar(var_));

    return {};
}

std::string CodeGen::createLabel() {
    return ".L" + std::to_string(currentLabelCount++);
}

void CodeGen::updateSections(const char* name, const std::pair<std::string, std::string>& data) {
    if (!sections.contains(name)) {
        sections[name] = std::vector<std::pair<std::string, std::string>>();
    }

    sections.at(name).emplace_back(data.first, data.second);
}

