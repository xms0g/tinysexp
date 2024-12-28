#include "codegen.h"
#include <format>

#define emit1(op, reg, n) generatedCode += std::format("\t{} {}, {}\n", op, reg, n)

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
            "section .text\n"
            "\tglobal _start\n"
            "_start:\n"
            "\tpush rbp\n"
            "\tmov rbp, rsp\n";

    auto next = ast;
    while (next != nullptr) {
        emitAST(ast);
        next = next->child;
    }
    generatedCode += "\tpop rbp\n"
                     "\tret\n";

    generatedCode += !sectionData.empty() ? "section .data\n" : "";
    for (auto& var: sectionData) {
        generatedCode += std::format("{}: dq {}", var.first, var.second);
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
        emit1("mov", rp.sreg, int_->n);
    } else if (auto double_ = cast::toDouble(n)) {
        rp = rtracker.alloc(14);
        uint64_t l = *((uint64_t*) &double_->n);
        emit1("movsd", rp.sreg, std::format("0x{:X}", l));
    } else {
        const auto var = cast::toVar(n);
        if (auto value = cast::toInt(var->value)) {
            rp = rtracker.alloc();
            emit1("mov", rp.sreg, value->n);
        } else if (auto value_ = cast::toDouble(var->value)) {
            rp = rtracker.alloc(14);
            emit1("movsd", rp.sreg, value_->n);
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

void CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, const char* op1, const char* op2, RegisterPair& rp) {
    RegisterPair reg1{};
    RegisterPair reg2{};
    const char* op = op1;

    emitNumb(lhs, reg1);
    emitRHS(rhs, reg2);

    if (reg1.rType == SSE && reg2.rType == GP) {
        op = op2;
        RegisterPair new_rp = rtracker.alloc(14);
        emit1("cvtsi2sd", new_rp.sreg, reg2.sreg);
        reg2 = new_rp;
    } else if (reg1.rType == GP && reg2.rType == SSE) {
        op = op2;
        RegisterPair new_rp = rtracker.alloc(14);
        emit1("cvtsi2sd", new_rp.sreg, reg1.sreg);
        reg1 = new_rp;
    } else if (reg1.rType == SSE && reg2.rType == SSE) {
        op = op2;
    }
    emit1(op, reg1.sreg, reg2.sreg);
    rtracker.free(reg2.reg);
    rp = reg1;
}


void CodeGen::emitBinop(const BinOpExpr& binop, RegisterPair& rp) {
    switch (binop.opToken.type) {
        case TokenType::PLUS: {
            emitExpr(binop.lhs, binop.rhs, "add", "addsd", rp);
            break;
        }
        case TokenType::MINUS: {
            emitExpr(binop.lhs, binop.rhs, "sub", "subsd", rp);
            break;
        }
        case TokenType::DIV: {
            emitExpr(binop.lhs, binop.rhs, "idiv", "divsd", rp);
            break;
        }
        case TokenType::MUL: {
            emitExpr(binop.lhs, binop.rhs, "imul", "mulsd", rp);
            break;
        }
        case TokenType::EQUAL:
            emitExpr(binop.lhs, binop.rhs, "cmp", nullptr, rp);
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






    // unsigned long long l = *((unsigned long long*)&dToUse);

    //reg = (std::holds_alternative<double>(lhs) || std::holds_alternative<double>(rhs)) ? "xmm0" : "rax";


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
    sectionData.emplace();
}

//void VarEvaluator::visit(BinOpExpr& binop) {
//    std::string lhs, rhs, code;
//
//    lhs = VarEvaluator::get(binop.lhs);
//    rhs = VarEvaluator::get(binop.rhs);
//
//    //TODO: remove then
//    auto isNumber = [&](std::string& s) {
//        char* p;
//        strtol(s.c_str(), &p, 10);
//        return *p == 0;
//    };
//
//    int loffset, roffset;
//
//    //TODO: do in semantic analyse
//    bool isRHSNum = isNumber(rhs);
//    bool isLHSGlobal = CodeGen::sectionData.contains(StringEval::get(binop.lhs));
//    bool isRHSGlobal = CodeGen::sectionData.contains(StringEval::get(binop.rhs));
//
//    switch (binop.opToken.type) {
//        case TokenType::PLUS: {
//            if (!isRHSNum) {
//                set(code += rhs);
//
//                if (isLHSGlobal) {
//                    std::string name = StringEval::get(binop.lhs);
//                    set(code += std::format("\tadd rax, qword [rip + {}]\n", name));
//                } else {
//                    if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
//                        loffset = itr->second;
//                        set(code += std::format("\tadd rax, qword [rbp - {}]\n", loffset));
//                    } else {
//                        set(code += std::format("\tadd rax, {}\n", lhs));
//                    }
//                }
//            } else {
//                if (isLHSGlobal) {
//                    std::string name = StringEval::get(binop.lhs);
//                    set(code += std::format("\tmov rax, qword [rip + {}]\n", name));
//                } else {
//                    if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
//                        loffset = itr->second;
//                        set(code += std::format("\tmov rax, qword [rbp - {}]\n", loffset));
//                    } else {
//                        set(code += std::format("\tmov rax, {}\n", lhs));
//                    }
//                }
//
//                if (isRHSGlobal) {
//                    std::string name = StringEval::get(binop.rhs);
//                    set(code += std::format("\tadd rax, qword [rip + {}]\n", name));
//                } else {
//                    if (auto itr = CodeGen::stackOffsets.find(rhs); itr != CodeGen::stackOffsets.end()) {
//                        roffset = itr->second;
//                        set(code += std::format("\tadd rax, qword [rbp - {}]\n", roffset));
//                    } else {
//                        set(code += std::format("\tadd rax, {}\n", rhs));
//                    }
//                }
//
//            }
//
//            break;
//        }
//        case TokenType::MINUS: {
//            if (!isRHSNum) {
//                set(code += rhs);
//                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
//                    loffset = itr->second;
//                    set(code += std::format("\tsub rax, qword [rbp - {}]\n", loffset));
//                } else {
//                    set(code += std::format("\tsub rax, {}\n", lhs));
//                }
//            } else {
//                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
//                    loffset = itr->second;
//                    set(code += std::format("\tmov rax, qword [rbp - {}]\n", loffset));
//                } else {
//                    set(code += std::format("\tmov rax, {}\n", lhs));
//                }
//
//                if (auto itr = CodeGen::stackOffsets.find(rhs); itr != CodeGen::stackOffsets.end()) {
//                    roffset = itr->second;
//                    set(code += std::format("\tsub rax, qword [rbp - {}]\n", roffset));
//                } else {
//                    set(code += std::format("\tsub rax, {}\n", rhs));
//                }
//            }
//            break;
//        }
//        case TokenType::DIV: {
//            if (!isRHSNum) {
//                set(code += rhs);
//                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
//                    loffset = itr->second;
//                    set(code += std::format("\tidiv rax, qword [rbp - {}]\n", loffset));
//                } else {
//                    set(code += std::format("\tidiv rax, {}\n", lhs));
//                }
//            } else {
//                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
//                    loffset = itr->second;
//                    set(code += std::format("\tmov rax, qword [rbp - {}]\n", loffset));
//                } else {
//                    set(code += std::format("\tmov rax, {}\n", lhs));
//                }
//
//                if (auto itr = CodeGen::stackOffsets.find(rhs); itr != CodeGen::stackOffsets.end()) {
//                    roffset = itr->second;
//                    set(code += std::format("\tidiv rax, qword [rbp - {}]\n", roffset));
//                } else {
//                    set(code += std::format("\tidiv rax, {}\n", rhs));
//                }
//            }
//            break;
//        }
//        case TokenType::MUL: {
//            if (!isRHSNum) {
//                set(code += rhs);
//                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
//                    loffset = itr->second;
//                    set(code += std::format("\timul rax, qword [rbp - {}]\n", loffset));
//                } else {
//                    set(code += std::format("\timul rax, {}\n", lhs));
//                }
//            } else {
//                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
//                    loffset = itr->second;
//                    set(code += std::format("\tmov rax, qword [rbp - {}]\n", loffset));
//                } else {
//                    set(code += std::format("\tmov rax, {}\n", lhs));
//                }
//
//                if (auto itr = CodeGen::stackOffsets.find(rhs); itr != CodeGen::stackOffsets.end()) {
//                    roffset = itr->second;
//                    set(code += std::format("\timul rax, qword [rbp - {}]\n", roffset));
//                } else {
//                    set(code += std::format("\timul rax, {}\n", rhs));
//                }
//            }
//            break;
//        }
//    }
//}
//

void CodeGen::emitDefconst(const DefconstExpr& defconst) {}

void CodeGen::emitDefun(const DefunExpr& defun) {}

void CodeGen::emitFuncCall(const FuncCallExpr& funcCall) {}

void CodeGen::emitIf(const IfExpr& if_) {}

void CodeGen::emitWhen(const WhenExpr& when) {}

void CodeGen::emitCond(const CondExpr& cond) {}
