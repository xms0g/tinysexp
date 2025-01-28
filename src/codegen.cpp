#include "codegen.h"
#include <format>

#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) generatedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) generatedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) generatedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) emitInstr1op(jmp, label)

#define emitSet8L(op, rp) \
    emitInstr1op(op, rtracker.strRepFromID((rp)->id, REG8L)); \
    emitInstr2op("movzx", rtracker.strRepFromID((rp)->id, REG64), rtracker.strRepFromID((rp)->id, REG8L));

#define checkRType(type, t) ((type) & (t))

#define preservedPrologue(rp) \
    if (checkRType((rp)->rType, PRESERVED)) { emitInstr1op("push", rtracker.strRepFromID((rp)->id, REG64)); }

#define preservedEpilogue(rp) \
    if (checkRType((rp)->rType, PRESERVED)) { emitInstr1op("pop", rtracker.strRepFromID((rp)->id, REG64)); }

#define register_alloc(type) \
    ([&]() -> Register* {               \
         auto* rp = rtracker.alloc(type);   \
         preservedPrologue(rp)              \
         return rp;                         \
    }())                                    \


#define register_free(rp) \
    rtracker.free(rp); \
    preservedEpilogue(rp)

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

const char* RegisterTracker::strRepFromID(uint32_t id, int size) {
    return strReps[id][size];
}

std::string CodeGen::emit(const ExprPtr& ast) {
    generatedCode =
            "[bits 64]\n"
            "section .text\n"
            "\tglobal _start\n"
            "_start:\n"
            "\tpush rbp\n"
            "\tmov rbp, rsp\n";

    auto next = ast;
    while (next != nullptr) {
        emitAST(next);
        next = next->child;
    }
    generatedCode += "\tpop rbp\n"
                     "\tret\n";

    generatedCode += !sectionData.empty() ? "\nsection .data\n" : "";
    for (auto& var: sectionData) {
        generatedCode += std::format("{}: {}\n", var.first, var.second);
    }

    generatedCode += !sectionBSS.empty() ? "\nsection .bss\n" : "";
    for (auto& var: sectionBSS) {
        generatedCode += std::format("{}: resq {}\n", var, 1);
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
        case TokenType::OR:
            return emitExpr(binop.lhs, binop.rhs, {"or", nullptr});
        case TokenType::LOGXOR:
            return emitExpr(binop.lhs, binop.rhs, {"xor", nullptr});
        case TokenType::LOGNOR: {
            ExprPtr negOne = std::make_shared<IntExpr>(-1);
            // Bitwise NOT seperately
            Register* rp1 = emitExpr(binop.lhs, negOne, {"xor", nullptr});
            Register* rp2 = emitExpr(binop.rhs, negOne, {"xor", nullptr});
            emitInstr2op("and", rtracker.strRepFromID(rp1->id, REG64), rtracker.strRepFromID(rp2->id, REG64));
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
            return emitExpr(binop.lhs, binop.rhs, {"cmp", "ucomisd"});
    }
}

void CodeGen::emitDotimes(const DotimesExpr& dotimes) {
    const auto iterVar = cast::toVar(dotimes.iterationCount);
    const std::string iterVarName = cast::toString(iterVar->name)->data;
    // Labels
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
    std::string iterVarAddr = getAddr(SymbolType::LOCAL, iterVarName);
    // Set 0 to iter var
    emitInstr2op("mov", iterVarAddr, 0);
    // Loop label
    emitLabel(loopLabel);
    emitTest(test, doneLabel);
    // Emit statements
    for (auto& statement: dotimes.statements) {
        emitAST(statement);
    }
    // Increment iteration count
    auto* rp = register_alloc(SCRATCH);

    emitInstr2op("mov", rtracker.strRepFromID(rp->id, REG64), iterVarAddr);
    emitInstr2op("add", rtracker.strRepFromID(rp->id, REG64), 1);
    emitInstr2op("mov", iterVarAddr, rtracker.strRepFromID(rp->id, REG64));

    register_free(rp)

    emitJump("jmp", loopLabel);
    emitLabel(doneLabel);
}

void CodeGen::emitLoop(const LoopExpr& loop) {
    // Labels
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
            emitTest(when->test, loopLabel);
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
        handleAssignment(var);
    }

    for (auto& sexpr: let.body) {
        emitAST(sexpr);
    }
}

void CodeGen::emitSetq(const SetqExpr& setq) {
    handleAssignment(setq.pair);
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
    // Emit test
    emitTest(if_.test, elseLabel);
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
    std::string doneLabel = createLabel();
    // Emit test
    emitTest(when.test, doneLabel);
    // Emit then
    for (auto& form: when.then) {
        emitAST(form);
    }
    emitLabel(doneLabel);
}

void CodeGen::emitCond(const CondExpr& cond) {
    std::string done = createLabel();

    for (auto& pair: cond.variants) {
        std::string elseLabel = createLabel();
        emitTest(pair.first, elseLabel);
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
        emitInstr2op("mov", rtracker.strRepFromID(rp->id, REG64), int_->n);
        return rp;
    } else if (auto double_ = cast::toDouble(n)) {
        auto* rp = rtracker.alloc(SSE);
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        emitInstr2op("movsd", rtracker.strRepFromID(rp->id, REG64), emitHex(hex));
        return rp;
    } else {
        const auto var = cast::toVar(n);
        const std::string varName = cast::toString(var->name)->data;

        return emitLoadRegFromMem(*var, varName);
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
        const char* newRPStr = rtracker.strRepFromID(newRP->id, REG64);

        emitInstr2op("cvtsi2sd", newRPStr, rtracker.strRepFromID(reg2->id, REG64));
        register_free(reg2)

        emitInstr2op(op.second, rtracker.strRepFromID(reg1->id, REG64), newRPStr);
        rtracker.free(newRP);
        return reg1;
    } else if ((checkRType(reg1->rType, SCRATCH) || checkRType(reg1->rType, PRESERVED)) && checkRType(reg2->rType, SSE)) {
        auto* newRP = rtracker.alloc(SSE);
        const char* newRPStr = rtracker.strRepFromID(newRP->id, REG64);
        const char* reg2Str = rtracker.strRepFromID(reg2->id, REG64);

        emitInstr2op("cvtsi2sd", newRPStr, rtracker.strRepFromID(reg1->id, REG64));
        register_free(reg1)

        emitInstr2op(op.second, newRPStr, reg2Str);
        emitInstr2op("movsd", reg2Str, newRPStr);
        rtracker.free(newRP);
        return reg2;
    } else if (checkRType(reg1->rType, SSE) && checkRType(reg2->rType, SSE)) {
        emitInstr2op(op.second, rtracker.strRepFromID(reg1->id, REG64), rtracker.strRepFromID(reg2->id, REG64));
        rtracker.free(reg2);
        return reg1;
    } else {
        emitInstr2op(op.first, rtracker.strRepFromID(reg1->id, REG64), rtracker.strRepFromID(reg2->id, REG64));
        register_free(reg2)
        return reg1;
    }
}

void CodeGen::emitSection(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);

    if (cast::toNIL(var_->value) || cast::toBinop(var_->value) || cast::toFuncCall(var_->value)) {
        sectionBSS.emplace(cast::toString(var_->name)->data);

        auto* rp = emitSet(var_->value);
        register_free(rp)
    } else if (auto int_ = cast::toInt(var_->value)) {
        sectionData.emplace(cast::toString(var_->name)->data, std::format("dq {}", std::to_string(int_->n)));
    } else if (auto double_ = cast::toDouble(var_->value)) {
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        sectionData.emplace(cast::toString(var_->name)->data, "dq " + emitHex(hex));
    } else if (auto str = cast::toString(var_->value)) {
        sectionData.emplace(cast::toString(var_->name)->data, std::format("db \"{}\", 10", str->data));
    }
}

void CodeGen::emitTest(const ExprPtr& test, std::string& label) {
    Register* rp;

    if (auto binop = cast::toBinop(test)) {
        if (binop->opToken.type == TokenType::AND) {
            ExprPtr zero = std::make_shared<IntExpr>(0);

            auto* rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
            register_free(rp1)
            emitJump("je", label);

            auto* rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
            register_free(rp2)
            emitJump("je", label);

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
                emitInstr2op("cmp", rtracker.strRepFromID(rp->id, REG64), 0);
                emitJump("je", label);
                break;
            }
            case TokenType::EQUAL:
            case TokenType::NOT:
                emitJump("jne", label);
                break;
            case TokenType::NEQUAL:
                emitJump("je", label);
                break;
            case TokenType::GREATER_THEN:
                emitJump("jle", label);
                break;
            case TokenType::LESS_THEN:
                emitJump("jge", label);
                break;
            case TokenType::GREATER_THEN_EQ:
                emitJump("jl", label);
                break;
            case TokenType::LESS_THEN_EQ:
                emitJump("jg", label);
                break;
        }
        register_free(rp)
    } else if (auto funcCall = cast::toFuncCall(test)) {
        rp = emitFuncCall(*funcCall);
        register_free(rp)
    } else if (auto var = cast::toVar(test)) {
        emitInstr2op("cmp", getAddr(var->sType, cast::toString(var->name)->data), 0);
        emitJump("je", label);
    } else if (cast::toNIL(test)) {
        emitJump("jmp", label);
    }
}

Register* CodeGen::emitSet(const ExprPtr& set) {
    Register* rp, *setReg;

    if (auto binop = cast::toBinop(set)) {
        if (binop->opToken.type == TokenType::AND) {
            ExprPtr zero = std::make_shared<IntExpr>(0);
            Register* setReg1, *setReg2;

            auto* rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});

            if (checkRType(rp1->rType, SSE)) {
                setReg1 = register_alloc(SCRATCH);
            } else {
                setReg1 = rp1;
            }

            const char* setReg1Str = rtracker.strRepFromID(setReg1->id, REG64);
            const char* setReg18LStr = rtracker.strRepFromID(setReg1->id, REG8L);

            emitInstr2op("xor", setReg1Str, setReg1Str);
            emitInstr1op("setne", setReg18LStr);

            auto* rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});

            if (checkRType(rp2->rType, SSE)) {
                setReg2 = register_alloc(SCRATCH);
            } else {
                setReg2 = rp2;
            }

            const char* setReg2Str = rtracker.strRepFromID(setReg2->id, REG64);
            const char* setReg28LStr = rtracker.strRepFromID(setReg2->id, REG8L);

            emitInstr2op("xor", setReg2Str, setReg2Str);
            emitInstr1op("setne", setReg28LStr);

            emitInstr2op("and", setReg18LStr, setReg28LStr);
            emitInstr2op("movzx", setReg1Str, setReg18LStr);

            if (checkRType(rp1->rType, SSE)) {
                emitInstr2op("cvtsi2sd", rtracker.strRepFromID(rp1->id, REG64), setReg1Str);
                register_free(setReg1)
            }

            if (checkRType(rp2->rType, SSE)) {
                register_free(setReg2)
            }

            register_free(rp2)
            return rp1;
        }

        //TODO: check out for double
//        if (binop->opToken.type == TokenType::OR) {
//            ExprPtr zero = std::make_shared<IntExpr>(0);
//            RegisterPair rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
//            emitInstr1op("setne", "al");
//            RegisterPair rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
//            emitInstr1op("setne", "cl");
//            emitInstr2op("or", "al", "cl");
//            emitInstr2op("movzx", rtracker.strRepFromID(rp1->id, REG64), "al");
//            rtracker.free(rp2);
//            return rp1;
//        }

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

void CodeGen::handleAssignment(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (auto int_ = cast::toInt(var_->value)) {
        handlePrimitive(*var_, varName, "mov", std::to_string(int_->n));
    } else if (auto double_ = cast::toDouble(var_->value)) {
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        handlePrimitive(*var_, varName, "movsd", emitHex(hex));
    } else if (cast::toVar(var_->value)) {
        handleVariable(*var_, varName);
    } else if (cast::toNIL(var_->value)) {
        getAddr(var_->sType, varName);
    } else if (auto str = cast::toString(var_->value)) {
        std::string label = ".L." + varName;
        std::string labelAddr = getAddr(var_->sType, label);
        std::string varAddr = getAddr(var_->sType, varName);

        sectionData.emplace(label, std::format("db \"{}\",10", str->data));

        auto* rp = register_alloc(SCRATCH);

        const char* rpStr = rtracker.strRepFromID(rp->id, REG64);

        emitInstr2op("lea", rpStr, labelAddr);
        emitInstr2op("mov", varAddr, rpStr);
        register_free(rp)
    } else {
        auto* rp = emitSet(var_->value);
        emitStoreMemFromReg(varName, var_->sType, rp);
        register_free(rp)
    }
}

void CodeGen::handlePrimitive(const VarExpr& var, const std::string& varName, const char* instr,
                              const std::string& value) {
    emitInstr2op(instr, getAddr(var.sType, varName), value);
}

void CodeGen::handleVariable(const VarExpr& var, const std::string& varName) {
    auto value = cast::toVar(var.value);
    std::string valueName = cast::toString(value->name)->data;

    Register* rp;

    do {
        rp = emitLoadRegFromMem(*value, valueName);

        if (rp.sreg[REG64]) {
            emitStoreMemFromReg(varName, var.sType, rp);
        }

        value = cast::toVar(value->value);
    } while (cast::toVar(value));

    register_free(rp)
}

Register* CodeGen::emitLoadRegFromMem(const VarExpr& value, const std::string& valueName) {
    Register* rp;

    if (cast::toInt(value.value)) {
        rp = register_alloc(SCRATCH);
        emitInstr2op("mov", rtracker.strRepFromID(rp->id, REG64), getAddr(value.sType, valueName));
    } else if (cast::toDouble(value.value)) {
        rp = rtracker.alloc(SSE);
        emitInstr2op("movsd", rtracker.strRepFromID(rp->id, REG64), getAddr(value.sType, valueName));
    }

    return rp;
}

void CodeGen::emitStoreMemFromReg(const std::string& varName, SymbolType stype, Register* rp) {
    const char* rpStr = rtracker.strRepFromID(rp->id, REG64);

    if (checkRType(rp->rType, SCRATCH) || checkRType(rp->rType, PRESERVED)) {
        emitInstr2op("mov", getAddr(stype, varName), rpStr);
    } else if (checkRType(rp->rType, SSE)) {
        emitInstr2op("movsd", getAddr(stype, varName), rpStr);
    }
}

std::string CodeGen::getAddr(SymbolType stype, const std::string& varName) {
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
            return std::format("qword [rbp - {}]", stackOffset);
        }
        case SymbolType::GLOBAL:
            return std::format("qword [rel {}]", varName);
        case SymbolType::PARAM:
            throw std::runtime_error("PARAM handling not implemented.");
        default:
            throw std::runtime_error("Unknown SymbolType.");
    }
}

std::string CodeGen::createLabel() {
    return ".L" + std::to_string(currentLabelCount++);
}

