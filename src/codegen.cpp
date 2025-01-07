#include "codegen.h"
#include <format>

#define emitInstruction(op, d, s) generatedCode += std::format("\t{} {}, {}\n", op, d, s)

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
        generatedCode += std::format("{}: dq {}\n", var, 0);
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
        case TokenType::EQUAL:
            emitExpr(binop.lhs, binop.rhs, {"cmp", nullptr});
            generatedCode += "jne .L{}";
        case TokenType::NEQUAL:
            break;
        case TokenType::GREATER_THEN:
            break;
        case TokenType::LESS_THEN:
            break;
        case TokenType::GREATER_THEN_EQ:
            break;
        case TokenType::LESS_THEN_EQ:
            break;
        case TokenType::AND:
            break;
        case TokenType::OR:
            break;
        case TokenType::NOT:
            break;
    }
}

void CodeGen::emitDotimes(const DotimesExpr& dotimes) {

}

void CodeGen::emitLoop(const LoopExpr& loop) {}

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

RegisterPair CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {

}

void CodeGen::emitIf(const IfExpr& if_) {}

void CodeGen::emitWhen(const WhenExpr& when) {}

void CodeGen::emitCond(const CondExpr& cond) {}

RegisterPair CodeGen::emitNumb(const ExprPtr& n) {
    RegisterPair rp{};

    if (auto int_ = cast::toInt(n)) {
        rp = rtracker.alloc(RegisterType::GP);
        emitInstruction("mov", rp.pair.second, int_->n);
    } else if (auto double_ = cast::toDouble(n)) {
        rp = rtracker.alloc(RegisterType::SSE);
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        emitInstruction("movsd", rp.pair.second, std::format("0x{:X}", hex));
    } else {
        const auto var = cast::toVar(n);
        const std::string varName = cast::toString(var->name)->data;

        rp = emitLoadInstruction(*var, varName);
    }

    return rp;
}

RegisterPair CodeGen::emitRHS(const ExprPtr& rhs) {
    RegisterPair reg{};

    if (auto binOp = cast::toBinop(rhs)) {
        reg = emitBinop(*binOp);
    } else {
        reg = emitNumb(rhs);
    }

    return reg;
}

RegisterPair CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op) {
    RegisterPair reg1{};
    RegisterPair reg2{};

    reg1 = emitNumb(lhs);
    reg2 = emitRHS(rhs);

    if (reg1.rType == RegisterType::SSE && reg2.rType == RegisterType::GP) {
        RegisterPair new_rp = rtracker.alloc(RegisterType::SSE);
        emitInstruction("cvtsi2sd", new_rp.pair.second, reg2.pair.second);
        rtracker.free(reg2.pair.first);

        emitInstruction(op.second, reg1.pair.second, new_rp.pair.second);
        rtracker.free(new_rp.pair.first);
        return reg1;
    } else if (reg1.rType == RegisterType::GP && reg2.rType == RegisterType::SSE) {
        RegisterPair new_rp = rtracker.alloc(RegisterType::SSE);
        emitInstruction("cvtsi2sd", new_rp.pair.second, reg1.pair.second);
        rtracker.free(reg1.pair.first);

        emitInstruction(op.second, new_rp.pair.second, reg2.pair.second);
        emitInstruction("movsd", reg2.pair.second, new_rp.pair.second);
        rtracker.free(new_rp.pair.first);
        return reg2;
    } else if (reg1.rType == RegisterType::SSE && reg2.rType == RegisterType::SSE) {
        emitInstruction(op.second, reg1.pair.second, reg2.pair.second);
        rtracker.free(reg2.pair.first);
        return reg1;
    } else {
        emitInstruction(op.first, reg1.pair.second, reg2.pair.second);
        rtracker.free(reg2.pair.first);
        return reg1;
    }
}

void CodeGen::emitSection(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);

    if (cast::toNIL(var_->value)) {
        sectionBSS.emplace(cast::toString(var_->name)->data);
    } else {
        if (auto int_ = cast::toInt(var_->value)) {
            sectionData.emplace(cast::toString(var_->name)->data, std::format("dq {}", std::to_string(int_->n)));
        } else if (auto double_ = cast::toDouble(var_->value)) {
            uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
            sectionData.emplace(cast::toString(var_->name)->data, std::format("dq 0x{:X}", hex));
        } else if (auto str = cast::toString(var_->value)) {
            sectionData.emplace(cast::toString(var_->name)->data, std::format("db \"{}\", 10", str->data));
        }
    }
}

void CodeGen::handleAssignment(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (auto int_ = cast::toInt(var_->value)) {
        handlePrimitive(*var_, varName, "mov", std::to_string(int_->n));
    } else if (auto double_ = cast::toDouble(var_->value)) {
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        handlePrimitive(*var_, varName, "movsd", std::format("0x{:X}", hex));
    } else if (cast::toVar(var_->value)) {
        handleVariable(*var_, varName);
    } else if (cast::toNIL(var_->value)) {
        getAddr(var_->sType, varName);
    } else if (auto str = cast::toString(var_->value)) {
        std::string label = ".L." + varName;
        std::string labelAddr = getAddr(var_->sType, label);
        std::string varAddr = getAddr(var_->sType, varName);

        sectionData.emplace(label, std::format("db \"{}\",10", str->data));

        RegisterPair reg = rtracker.alloc(RegisterType::GP);
        emitInstruction("lea", reg.pair.second, labelAddr);
        emitInstruction("mov", varAddr, reg.pair.second);
        rtracker.free(reg.pair.first);
    } else {
        RegisterPair rp{};

        if (auto binop = cast::toBinop(var_->value)) {
            rp = emitBinop(*binop);
        } else if (auto funcCall = cast::toFuncCall(var_->value)) {
            rp = emitFuncCall(*funcCall);
        }

        emitStoreInstruction(varName, var_->sType, rp);
        rtracker.free(rp.pair.first);
    }
}

void CodeGen::handlePrimitive(const VarExpr& var, const std::string& varName, const char* instr,
                              const std::string& value) {
    const std::string address = getAddr(var.sType, varName);
    emitInstruction(instr, address, value);
}

void CodeGen::handleVariable(const VarExpr& var, const std::string& varName) {
    auto value = cast::toVar(var.value);
    std::string valueName = cast::toString(value->name)->data;

    RegisterPair rp{};

    do {
        rp = emitLoadInstruction(*value, valueName);

        if (rp.pair.second)
            emitStoreInstruction(varName, var.sType, rp);

        value = cast::toVar(value->value);
    } while (cast::toVar(value));

    rtracker.free(rp.pair.first);
}

RegisterPair CodeGen::emitLoadInstruction(const VarExpr& value, const std::string& valueName) {
    RegisterPair rp{};

    if (cast::toInt(value.value)) {
        rp = rtracker.alloc(RegisterType::GP);
        emitInstruction("mov", rp.pair.second, getAddr(value.sType, valueName));
    } else if (cast::toDouble(value.value)) {
        rp = rtracker.alloc(RegisterType::SSE);
        emitInstruction("movsd", rp.pair.second, getAddr(value.sType, valueName));
    }
    return rp;
}

void CodeGen::emitStoreInstruction(const std::string& varName, SymbolType stype, RegisterPair reg) {
    switch (reg.rType) {
        case RegisterType::GP:
            emitInstruction("mov", getAddr(stype, varName), reg.pair.second);
            break;
        case RegisterType::SSE:
            emitInstruction("movsd", getAddr(stype, varName), reg.pair.second);
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

