#include "codegen.h"
#include <format>

RegisterPair RegisterTracker::alloc() {
    for (int i = 0; i < EOF_R; ++i) {
        if (!registersInUse.contains((Register)i)) {
            registersInUse.emplace((Register)i);
            return {(Register)i, stringRepFromReg[i]};
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
    //TODO: implement for double
    rp = rtracker.alloc();

    if (auto int_ = cast::toInt(n)) {
        generatedCode += std::format("\tmov {}, {}\n", rp.sreg, int_->n);
    } else if (auto double_ = cast::toDouble(n)) {
        generatedCode += std::format("\tmov xmm0, {}\n", double_->n);
    } else {
        const auto var = cast::toVar(n);

        if (auto value = cast::toInt(var->value)) {
            generatedCode += std::format("\tmov {}, {}\n", rp.sreg, value->n);
        } else if (auto value_ = cast::toDouble(var->value)) {
            generatedCode += std::format("\tmov xmm0, {}\n", value_->n);
        }
    }
}

void CodeGen::emitExpr(const char* op, const ExprPtr& lhs, const ExprPtr& rhs, RegisterPair& rp) {
    RegisterPair reg1{};
    RegisterPair reg2{};

    emitNumb(lhs, reg1);

    if (auto binOp = cast::toBinop(rhs)) {
        emitBinop(*binOp, reg2);
    } else {
        emitNumb(rhs, reg2);

    }

    generatedCode += std::format("\t{} {}, {}\n", op, reg1.sreg, reg2.sreg);
    rtracker.free(reg2.reg);
    rp = reg1;
}


void CodeGen::emitBinop(const BinOpExpr& binop, RegisterPair& rp) {
    switch (binop.opToken.type) {
        case TokenType::PLUS:
            emitExpr("add", binop.lhs, binop.rhs, rp);
            break;
        case TokenType::MINUS:
            emitExpr("sub", binop.lhs, binop.rhs, rp);
            break;
        case TokenType::DIV:
            emitExpr("div", binop.lhs, binop.rhs, rp);
            break;
        case TokenType::MUL:
            emitExpr("mul", binop.lhs, binop.rhs, rp);
            break;
        case TokenType::EQUAL:
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
