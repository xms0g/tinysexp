#include "codegen.h"
#include <format>

#define emit1(code, op, reg, n) code += std::format("\t{} {}, {}\n", op, reg, n)

RegisterPair RegisterTracker::alloc(int index) {
    for (int i = index; i < EOR; ++i) {
        if (!registersInUse.contains((Register) i)) {
            registersInUse.emplace((Register) i);
            return {(Register) i, i < 14 ? GP : SSE, stringRepFromReg[i]};
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
        generatedCode += std::format("{}: dq {}", var.first, var.second);
    }

    generatedCode += !sectionBSS.empty() ? "section .bss\n" : "";
    for (auto& var: sectionBSS) {
        generatedCode += std::format("{}: resq {}", var, 1);
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

void CodeGen::emitNumb(const ExprPtr& n, RegisterPair& rp) {
    if (auto int_ = cast::toInt(n)) {
        rp = rtracker.alloc();
        emit1(generatedCode, "mov", rp.sreg, int_->n);
    } else if (auto double_ = cast::toDouble(n)) {
        rp = rtracker.alloc(14);
        uint64_t hex = *((uint64_t*) &double_->n);
        emit1(generatedCode, "movsd", rp.sreg, std::format("0x{:X}", hex));
    } else {
        const auto var = cast::toVar(n);
        if (auto value = cast::toInt(var->value)) {
            rp = rtracker.alloc();
            emit1(generatedCode, "mov", rp.sreg, std::format("qword [rip + {}]", cast::toString(var->name)->data));
        } else if (auto value_ = cast::toDouble(var->value)) {
            rp = rtracker.alloc(14);
            emit1(generatedCode, "movsd", rp.sreg, std::format("qword [rip + {}]", cast::toString(var->name)->data));
        }
    }
}

void CodeGen::emitRHS(const ExprPtr& rhs, RegisterPair& rp) {
    RegisterPair reg{};

    if (auto binOp = cast::toBinop(rhs)) {
        emitBinop(*binOp, reg);
    } else {
        emitNumb(rhs, reg);
    }

    rp = reg;
}

void CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, OpcodePair op, RegisterPair& rp) {
    RegisterPair reg1{};
    RegisterPair reg2{};

    emitNumb(lhs, reg1);
    emitRHS(rhs, reg2);

    if (reg1.rType == SSE && reg2.rType == GP) {
        RegisterPair new_rp = rtracker.alloc(14);
        emit1(generatedCode, "cvtsi2sd", new_rp.sreg, reg2.sreg);
        rtracker.free(reg2.reg);
        emit1(generatedCode, op.sse, reg1.sreg, new_rp.sreg);
        rtracker.free(new_rp.reg);
        rp = reg1;
    } else if (reg1.rType == GP && reg2.rType == SSE) {
        RegisterPair new_rp = rtracker.alloc(14);
        emit1(generatedCode, "cvtsi2sd", new_rp.sreg, reg1.sreg);
        rtracker.free(reg1.reg);
        emit1(generatedCode, op.sse, new_rp.sreg, reg2.sreg);
        emit1(generatedCode, "mov", reg2.sreg, new_rp.sreg);
        rtracker.free(new_rp.reg);
        rp = reg2;
    } else if (reg1.rType == SSE && reg2.rType == SSE) {
        emit1(generatedCode, op.sse, reg1.sreg, reg2.sreg);
        rtracker.free(reg2.reg);
        rp = reg1;
    } else {
        emit1(generatedCode, op.gp, reg1.sreg, reg2.sreg);
        rtracker.free(reg2.reg);
        rp = reg1;
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
//    for (auto& var: let.bindings) {
//        std::string vs = VarEvaluator::get(var);
//        set(code += std::format("\tmov qword [rbp - {}], {}\n",
//                                CodeGen::stackOffset, vs));
//
//        CodeGen::stackOffsets.emplace(vs, CodeGen::stackOffset);
//        CodeGen::stackOffset += 8;
//    }
//
//    for (auto& sexpr: let.body) {
//        set(code += VarEvaluator::get(sexpr));
//    }

}

void CodeGen::emitSetq(const SetqExpr& setq) {

}

void CodeGen::emitDefvar(const DefvarExpr& defvar) {
    const auto var = cast::toVar(defvar.pair);
    if (cast::toNIL(var->value)) {
        sectionBSS.emplace(cast::toString(var->name)->data);
    } else {
        if (auto int_ = cast::toInt(var->value)) {
            sectionData.emplace(cast::toString(var->name)->data, std::to_string(int_->n));
        } else if (auto double_ = cast::toDouble(var->value)) {
            uint64_t hex = *((uint64_t*) &double_->n);
            sectionData.emplace(cast::toString(var->name)->data, std::format("0x{:X}", hex));
        }
    }
}

void CodeGen::emitDefconst(const DefconstExpr& defconst) {
    const auto var = cast::toVar(defconst.pair);

    if (auto int_ = cast::toInt(var->value)) {
        sectionData.emplace(cast::toString(var->name)->data, std::to_string(int_->n));
    } else if (auto double_ = cast::toDouble(var->value)) {
        uint64_t hex = *((uint64_t*) &double_->n);
        sectionData.emplace(cast::toString(var->name)->data, std::format("0x{:X}", hex));
    }
}

void CodeGen::emitDefun(const DefunExpr& defun) {}

void CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {}

void CodeGen::emitIf(const IfExpr& if_) {}

void CodeGen::emitWhen(const WhenExpr& when) {}

void CodeGen::emitCond(const CondExpr& cond) {}
