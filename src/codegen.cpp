#include "codegen.h"
#include <format>

#define emit1(code, op, reg, n) code += std::format("\t{} {}, {}\n", op, reg, n)

RegisterPair RegisterTracker::alloc(int index) {
    for (int i = index; i < EOR; ++i) {
        if (!registersInUse.contains((Register) i)) {
            registersInUse.emplace((Register) i);
            return {(Register) i, stringRepFromReg[i], i < 14 ? GP : SSE};
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

    generatedCode += !sectionData.empty() ? "section .data\n" : "";
    for (auto& var: sectionData) {
        generatedCode += std::format("{}: dq {}\n", var.first, var.second);
    }

    generatedCode += !sectionBSS.empty() ? "section .bss\n" : "";
    for (auto& var: sectionBSS) {
        generatedCode += std::format("{}: resq {}\n", var, 1);
    }
    return generatedCode;
}

void CodeGen::emitAST(const ExprPtr& ast) {
    if (auto binop = cast::toBinop(ast)) {
        RegisterPair rp{};
        emitBinop(*binop, rp);
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

void CodeGen::emitBinop(const BinOpExpr& binop, RegisterPair& rp) {
    switch (binop.opToken.type) {
        case TokenType::PLUS: {
            emitExpr(binop.lhs, binop.rhs, {"add", "addsd"}, rp);
            break;
        }
        case TokenType::MINUS: {
            emitExpr(binop.lhs, binop.rhs, {"sub", "subsd"}, rp);
            break;
        }
        case TokenType::DIV: {
            emitExpr(binop.lhs, binop.rhs, {"idiv", "divsd"}, rp);
            break;
        }
        case TokenType::MUL: {
            emitExpr(binop.lhs, binop.rhs, {"imul", "mulsd"}, rp);
            break;
        }
        case TokenType::EQUAL:
            emitExpr(binop.lhs, binop.rhs, {"cmp", nullptr}, rp);
            generatedCode += "jne .L{}";
            break;
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

void CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {}

void CodeGen::emitIf(const IfExpr& if_) {}

void CodeGen::emitWhen(const WhenExpr& when) {}

void CodeGen::emitCond(const CondExpr& cond) {}

RegisterPair CodeGen::emitNumb(const ExprPtr& n) {
    RegisterPair rp{};

    if (auto int_ = cast::toInt(n)) {
        rp = rtracker.alloc();
        emit1(generatedCode, "mov", rp.sreg, int_->n);
    } else if (auto double_ = cast::toDouble(n)) {
        rp = rtracker.alloc(14);
        uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
        emit1(generatedCode, "movsd", rp.sreg, std::format("0x{:X}", hex));
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
        emitBinop(*binOp, reg);
    } else {
        reg = emitNumb(rhs);
    }

    return reg;
}

void CodeGen::emitSection(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);

    if (cast::toNIL(var_->value)) {
        sectionBSS.emplace(cast::toString(var_->name)->data);
    } else {
        if (auto int_ = cast::toInt(var_->value)) {
            sectionData.emplace(cast::toString(var_->name)->data, std::to_string(int_->n));
        } else if (auto double_ = cast::toDouble(var_->value)) {
            uint64_t hex = *reinterpret_cast<uint64_t*>(&double_->n);
            sectionData.emplace(cast::toString(var_->name)->data, std::format("0x{:X}", hex));
        }
    }
}

void CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs,
                       std::pair<const char*, const char*> op, RegisterPair& rp) {
    RegisterPair reg1{};
    RegisterPair reg2{};

    reg1 = emitNumb(lhs);
    reg2 = emitRHS(rhs);

    if (reg1.rType == SSE && reg2.rType == GP) {
        RegisterPair new_rp = rtracker.alloc(14);
        emit1(generatedCode, "cvtsi2sd", new_rp.sreg, reg2.sreg);
        rtracker.free(reg2.reg);

        emit1(generatedCode, op.second, reg1.sreg, new_rp.sreg);
        rtracker.free(new_rp.reg);
        rp = reg1;
    } else if (reg1.rType == GP && reg2.rType == SSE) {
        RegisterPair new_rp = rtracker.alloc(14);
        emit1(generatedCode, "cvtsi2sd", new_rp.sreg, reg1.sreg);
        rtracker.free(reg1.reg);

        emit1(generatedCode, op.second, new_rp.sreg, reg2.sreg);
        emit1(generatedCode, "movsd", reg2.sreg, new_rp.sreg);
        rtracker.free(new_rp.reg);
        rp = reg2;
    } else if (reg1.rType == SSE && reg2.rType == SSE) {
        emit1(generatedCode, op.second, reg1.sreg, reg2.sreg);
        rtracker.free(reg2.reg);
        rp = reg1;
    } else {
        emit1(generatedCode, op.first, reg1.sreg, reg2.sreg);
        rtracker.free(reg2.reg);
        rp = reg1;
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
    } else {
        emitAST(var_->value);
    }
}

void CodeGen::handlePrimitive(const VarExpr& var, const std::string& varName, const char* instr,
                              const std::string& value) {
    const std::string address = getAddr(var.sType, varName);
    emit1(generatedCode, instr, address, value);
}

void CodeGen::handleVariable(const VarExpr& var, const std::string& varName) {
    auto value = cast::toVar(var.value);
    std::string valueName = cast::toString(value->name)->data;

    RegisterPair rp{};
    do {
        rp = emitLoadInstruction(*value, valueName);
        emitStoreInstruction(varName, value->value, var.sType, rp);

        value = cast::toVar(value->value);
    } while (cast::toVar(value));

    rtracker.free(rp.reg);
}

RegisterPair CodeGen::emitLoadInstruction(const VarExpr& value, const std::string& valueName) {
    RegisterPair rp{};

    if (cast::toInt(value.value)) {
        rp = rtracker.alloc();
        emit1(generatedCode, "mov", rp.sreg, getAddr(value.sType, valueName));
    } else if (cast::toDouble(value.value)) {
        rp = rtracker.alloc(14);
        emit1(generatedCode, "movsd", rp.sreg, getAddr(value.sType, valueName));
    }
    return rp;
}

void CodeGen::emitStoreInstruction(const std::string& varName, const ExprPtr& value, SymbolType stype,
                                   RegisterPair reg) {
    if (cast::toInt(value)) {
        emit1(generatedCode, "mov", getAddr(stype, varName), reg.sreg);
    } else if (cast::toDouble(value)) {
        emit1(generatedCode, "movsd", getAddr(stype, varName), reg.sreg);
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

