#include "codegen.h"
#include <format>

#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) generatedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) generatedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) generatedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) generatedCode += std::format("\t{} {}\n", jmp, label);

RegisterPair RegisterTracker::alloc(RegisterType rtype) {
    switch (rtype) {
        case RegisterType::GP: {
            for (const auto& sreg: scratchRegisters) {
                if (!registersInUse.contains(sreg.first)) {
                    registersInUse.emplace(sreg.first);
                    return {sreg, RegisterType::GP};
                }
            }

            for (const auto& preg: preservedRegisters) {
                if (!registersInUse.contains(preg.first)) {
                    registersInUse.emplace(preg.first);
                    return {preg, RegisterType::GP};
                }
            }
            break;
        }
        case RegisterType::SSE: {
            for (const auto& sse: sseRegisters) {
                if (!registersInUse.contains(sse.first)) {
                    registersInUse.emplace(sse.first);
                    return {sse, RegisterType::SSE};
                }
            }
            break;
        }
    }
}

void RegisterTracker::free(Register reg) {
    registersInUse.erase(reg);
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

RegisterPair CodeGen::emitBinop(const BinOpExpr& binop) {
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
            RegisterPair rp1 = emitExpr(binop.lhs, negOne, {"xor", nullptr});
            RegisterPair rp2 = emitExpr(binop.rhs, negOne, {"xor", nullptr});
            emitInstr2op("and", rp1.pair.second, rp2.pair.second);
            rtracker.free(rp2.pair.first);
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
    RegisterPair rp = rtracker.alloc(RegisterType::GP);
    emitInstr2op("mov", rp.pair.second, iterVarAddr);
    emitInstr2op("add", rp.pair.second, 1);
    emitInstr2op("mov", iterVarAddr, rp.pair.second);
    rtracker.free(rp.pair.first);

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

RegisterPair CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {}

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

RegisterPair CodeGen::emitNumb(const ExprPtr& n) {
    RegisterPair rp;

    if (auto int_ = cast::toInt(n)) {
        rp = rtracker.alloc(RegisterType::GP);
        emitInstr2op("mov", rp.pair.second, int_->n);
    } else if (auto double_ = cast::toDouble(n)) {
        rp = rtracker.alloc(RegisterType::SSE);
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        emitInstr2op("movsd", rp.pair.second, emitHex(hex));
    } else {
        const auto var = cast::toVar(n);
        const std::string varName = cast::toString(var->name)->data;

        rp = emitLoadRegFromMem(*var, varName);
    }

    return rp;
}

RegisterPair CodeGen::emitNode(const ExprPtr& node) {
    RegisterPair rp;

    if (auto binOp = cast::toBinop(node)) {
        rp = emitBinop(*binOp);
    } else if (auto funcCall = cast::toFuncCall(node)) {
        rp = emitFuncCall(*funcCall);
    } else {
        rp = emitNumb(node);
    }

    return rp;
}

RegisterPair CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op) {
    RegisterPair reg1;
    RegisterPair reg2;

    reg1 = emitNode(lhs);
    reg2 = emitNode(rhs);

    if (reg1.rType == RegisterType::SSE && reg2.rType == RegisterType::GP) {
        RegisterPair new_rp = rtracker.alloc(RegisterType::SSE);
        emitInstr2op("cvtsi2sd", new_rp.pair.second, reg2.pair.second);
        rtracker.free(reg2.pair.first);

        emitInstr2op(op.second, reg1.pair.second, new_rp.pair.second);
        rtracker.free(new_rp.pair.first);
        return reg1;
    } else if (reg1.rType == RegisterType::GP && reg2.rType == RegisterType::SSE) {
        RegisterPair new_rp = rtracker.alloc(RegisterType::SSE);
        emitInstr2op("cvtsi2sd", new_rp.pair.second, reg1.pair.second);
        rtracker.free(reg1.pair.first);

        emitInstr2op(op.second, new_rp.pair.second, reg2.pair.second);
        emitInstr2op("movsd", reg2.pair.second, new_rp.pair.second);
        rtracker.free(new_rp.pair.first);
        return reg2;
    } else if (reg1.rType == RegisterType::SSE && reg2.rType == RegisterType::SSE) {
        emitInstr2op(op.second, reg1.pair.second, reg2.pair.second);
        rtracker.free(reg2.pair.first);
        return reg1;
    } else {
        emitInstr2op(op.first, reg1.pair.second, reg2.pair.second);
        rtracker.free(reg2.pair.first);
        return reg1;
    }
}

void CodeGen::emitSection(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);

    if (cast::toNIL(var_->value) || cast::toBinop(var_->value) || cast::toFuncCall(var_->value)) {
        sectionBSS.emplace(cast::toString(var_->name)->data);

        RegisterPair rp = emitSet(var_->value);
        rtracker.free(rp.pair.first);
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
    RegisterPair rp;

    if (auto binop = cast::toBinop(test)) {
        if (binop->opToken.type == TokenType::AND) {
            ExprPtr zero = std::make_shared<IntExpr>(0);
            RegisterPair rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
            emitJump("je", label);
            RegisterPair rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
            emitJump("je", label);
            rtracker.free(rp1.pair.first);
            rtracker.free(rp2.pair.first);
            return;
        }

        if (binop->opToken.type == TokenType::OR) {
            RegisterPair rp1 = emitNode(binop->lhs);
            RegisterPair rp2 = emitNode(binop->rhs);
            emitInstr2op("or", rp1.pair.second, rp2.pair.second);
            emitJump("je", label);
            rtracker.free(rp1.pair.first);
            rtracker.free(rp2.pair.first);
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
                emitInstr2op("cmp", rp.pair.second, 0);
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
        rtracker.free(rp.pair.first);
    } else if (auto funcCall = cast::toFuncCall(test)) {
        rp = emitFuncCall(*funcCall);
        rtracker.free(rp.pair.first);
    } else if (auto var = cast::toVar(test)) {
        emitInstr2op("cmp", getAddr(var->sType, cast::toString(var->name)->data), 0);
        emitJump("je", label);
    } else if (cast::toNIL(test)) {
        emitJump("jmp", label);
    }
}

RegisterPair CodeGen::emitSet(const ExprPtr& set) {
    RegisterPair rp;

    if (auto binop = cast::toBinop(set)) {
        if (binop->opToken.type == TokenType::AND) {
            ExprPtr zero = std::make_shared<IntExpr>(0);
            RegisterPair rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
            emitInstr1op("setne", "al");
            RegisterPair rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
            emitInstr1op("setne", "cl");
            emitInstr2op("and", "al", "cl");
            emitInstr2op("movzx", rp1.pair.second, "al");
            rtracker.free(rp2.pair.first);
            return rp1;
        }

        if (binop->opToken.type == TokenType::OR) {
            ExprPtr zero = std::make_shared<IntExpr>(0);
            RegisterPair rp1 = emitExpr(binop->lhs, zero, {"cmp", "ucomisd"});
            emitInstr1op("setne", "al");
            RegisterPair rp2 = emitExpr(binop->rhs, zero, {"cmp", "ucomisd"});
            emitInstr1op("setne", "cl");
            emitInstr2op("or", "al", "cl");
            emitInstr2op("movzx", rp1.pair.second, "al");
            rtracker.free(rp2.pair.first);
            return rp1;
        }

        rp = emitBinop(*binop);

        switch (binop->opToken.type) {
            case TokenType::EQUAL:
            case TokenType::NOT:
                emitInstr1op("sete", "al");
                emitInstr2op("movzx", rp.pair.second, "al");
                break;
            case TokenType::NEQUAL:
                emitInstr1op("setne", "al");
                emitInstr2op("movzx", rp.pair.second, "al");
                break;
            case TokenType::GREATER_THEN:
                emitInstr1op("setg", "al");
                emitInstr2op("movzx", rp.pair.second, "al");
                break;
            case TokenType::LESS_THEN:
                emitInstr1op("setl", "al");
                emitInstr2op("movzx", rp.pair.second, "al");
                break;
            case TokenType::GREATER_THEN_EQ:
                emitInstr1op("setge", "al");
                emitInstr2op("movzx", rp.pair.second, "al");
                break;
            case TokenType::LESS_THEN_EQ:
                emitInstr1op("setle", "al");
                emitInstr2op("movzx", rp.pair.second, "al");
                break;
        }

    } else if (auto funcCall = cast::toFuncCall(set)) {
        rp = emitFuncCall(*funcCall);
    }

    return rp;
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

        RegisterPair rp = rtracker.alloc(RegisterType::GP);
        emitInstr2op("lea", rp.pair.second, labelAddr);
        emitInstr2op("mov", varAddr, rp.pair.second);
        rtracker.free(rp.pair.first);
    } else {
        RegisterPair rp = emitSet(var_->value);
        emitStoreMemFromReg(varName, var_->sType, rp);
        rtracker.free(rp.pair.first);
    }
}

void CodeGen::handlePrimitive(const VarExpr& var, const std::string& varName, const char* instr,
                              const std::string& value) {
    emitInstr2op(instr, getAddr(var.sType, varName), value);
}

void CodeGen::handleVariable(const VarExpr& var, const std::string& varName) {
    auto value = cast::toVar(var.value);
    std::string valueName = cast::toString(value->name)->data;

    RegisterPair rp;

    do {
        rp = emitLoadRegFromMem(*value, valueName);

        if (rp.pair.second)
            emitStoreMemFromReg(varName, var.sType, rp);

        value = cast::toVar(value->value);
    } while (cast::toVar(value));

    rtracker.free(rp.pair.first);
}

RegisterPair CodeGen::emitLoadRegFromMem(const VarExpr& value, const std::string& valueName) {
    RegisterPair rp;

    if (cast::toInt(value.value)) {
        rp = rtracker.alloc(RegisterType::GP);
        emitInstr2op("mov", rp.pair.second, getAddr(value.sType, valueName));
    } else if (cast::toDouble(value.value)) {
        rp = rtracker.alloc(RegisterType::SSE);
        emitInstr2op("movsd", rp.pair.second, getAddr(value.sType, valueName));
    }
    return rp;
}

void CodeGen::emitStoreMemFromReg(const std::string& varName, SymbolType stype, RegisterPair rp) {
    switch (rp.rType) {
        case RegisterType::GP:
            emitInstr2op("mov", getAddr(stype, varName), rp.pair.second);
            break;
        case RegisterType::SSE:
            emitInstr2op("movsd", getAddr(stype, varName), rp.pair.second);
            break;
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

