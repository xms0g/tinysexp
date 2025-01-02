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
        const auto var_ = cast::toVar(var);
        const std::string varName = cast::toString(var_->name)->data;

        if (auto int_ = cast::toInt(var_->value)) {
            emit1(generatedCode, "mov", std::format("qword [rbp - {}]", currentStackOffset), int_->n);
        } else if (auto double_ = cast::toDouble(var_->value)) {
            uint64_t hex = *((uint64_t*) &double_->n);

            emit1(generatedCode, "movsd", std::format("qword [rbp - {}]", currentStackOffset),
                  std::format("0x{:X}", hex));
        } else {
            auto value = cast::toVar(var_->value);
            const std::string valueName = cast::toString(value->name)->data;

            RegisterPair rp{};

            do {
                if (cast::toInt(value->value)) {
                    rp = rtracker.alloc();

                    switch (value->sType) {
                        case SymbolType::LOCAL:
                            emit1(generatedCode, "mov", rp.sreg, std::format("qword [rbp - {}]", stackOffsets.at(valueName)));
                            break;
                        case SymbolType::PARAM:
                            break;
                        case SymbolType::GLOBAL:
                            emit1(generatedCode, "mov", rp.sreg, std::format("qword [rel {}]", valueName));
                            break;
                    }

                    emit1(generatedCode, "mov", std::format("qword [rbp - {}]", currentStackOffset), rp.sreg);
                    break;
                } else if (cast::toDouble(value->value)) {
                    rp = rtracker.alloc(14);

                    switch (value->sType) {
                        case SymbolType::LOCAL:
                            emit1(generatedCode, "movsd", rp.sreg, std::format("qword [rbp - {}]", stackOffsets.at(valueName)));
                            break;
                        case SymbolType::PARAM:
                            break;
                        case SymbolType::GLOBAL:
                            emit1(generatedCode, "movsd", rp.sreg, std::format("qword [rel {}]", valueName));
                            break;
                    }

                    emit1(generatedCode, "movsd", std::format("qword [rbp - {}]", currentStackOffset), rp.sreg);
                    break;
                } else {
                    value = cast::toVar(value->value);
                }
            } while (cast::toVar(value));

            rtracker.free(rp.reg);
        }

        stackOffsets.emplace(varName, currentStackOffset);
        currentStackOffset += 8;
    }

    for (auto& sexpr: let.body) {
        emitAST(sexpr);
    }

}

void CodeGen::emitSetq(const SetqExpr& setq) {
    const auto var = cast::toVar(setq.pair);
    const std::string varName = cast::toString(var->name)->data;

    if (auto int_ = cast::toInt(var->value)) {
        switch (var->sType) {
            case SymbolType::LOCAL:
                emit1(generatedCode, "mov", std::format("qword [rbp - {}]", stackOffsets.at(varName)), int_->n);
                break;
            case SymbolType::PARAM:
                break;
            case SymbolType::GLOBAL:
                emit1(generatedCode, "mov", std::format("qword [rel {}]", varName), int_->n);
                break;
        }

    } else if (auto double_ = cast::toDouble(var->value)) {
        uint64_t hex = *((uint64_t*) &double_->n);

        switch (var->sType) {
            case SymbolType::LOCAL:
                emit1(generatedCode, "movsd", std::format("qword [rbp - {}]", stackOffsets.at(varName)), hex);
                break;
            case SymbolType::PARAM:
                break;
            case SymbolType::GLOBAL:
                emit1(generatedCode, "movsd", std::format("qword [rel {}]", varName), hex);
                break;
        }

    } else {
        auto value = cast::toVar(var->value);
        const std::string valueName = cast::toString(value->name)->data;

        RegisterPair rp{};

        do {
            if (cast::toInt(value->value)) {
                rp = rtracker.alloc();

                switch (value->sType) {
                    case SymbolType::LOCAL:
                        emit1(generatedCode, "mov", rp.sreg, std::format("qword [rbp - {}]", stackOffsets.at(valueName)));
                        break;
                    case SymbolType::PARAM:
                        break;
                    case SymbolType::GLOBAL:
                        emit1(generatedCode, "mov", rp.sreg, std::format("qword [rel {}]", valueName));
                        break;
                }

                switch (var->sType) {
                    case SymbolType::LOCAL:
                        emit1(generatedCode, "mov", std::format("qword [rbp - {}]", stackOffsets.at(varName)), rp.sreg);
                        break;
                    case SymbolType::PARAM:
                        break;
                    case SymbolType::GLOBAL:
                        emit1(generatedCode, "mov", std::format("qword [rel {}]", varName), rp.sreg);
                        break;
                }
                break;
            } else if (cast::toDouble(value->value)) {
                rp = rtracker.alloc(14);

                switch (value->sType) {
                    case SymbolType::LOCAL:
                        emit1(generatedCode, "movsd", rp.sreg, std::format("qword [rbp - {}]", stackOffsets.at(valueName)));
                        break;
                    case SymbolType::PARAM:
                        break;
                    case SymbolType::GLOBAL:
                        emit1(generatedCode, "movsd", rp.sreg, std::format("qword [rel {}]", valueName));
                        break;
                }

                switch (var->sType) {
                    case SymbolType::LOCAL:
                        emit1(generatedCode, "movsd", std::format("qword [rbp - {}]", stackOffsets.at(varName)), rp.sreg);
                        break;
                    case SymbolType::PARAM:
                        break;
                    case SymbolType::GLOBAL:
                        emit1(generatedCode, "movsd", std::format("qword [rel {}]", varName), rp.sreg);
                        break;
                }
                break;
            } else {
                value = cast::toVar(value->value);
            }
        } while (cast::toVar(value));

        rtracker.free(rp.reg);

    }
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
        const std::string varName = cast::toString(var->name)->data;

        if (auto value = cast::toInt(var->value)) {
            rp = rtracker.alloc();

            switch (var->sType) {
                case SymbolType::LOCAL:
                    emit1(generatedCode, "mov", rp.sreg, std::format("qword [rbp - {}]", stackOffsets.at(varName)));
                    break;
                case SymbolType::PARAM:
                    break;
                case SymbolType::GLOBAL:
                    emit1(generatedCode, "mov", rp.sreg, std::format("qword [rel {}]", varName));
                    break;
            }

        } else if (auto value_ = cast::toDouble(var->value)) {
            rp = rtracker.alloc(14);

            switch (var->sType) {
                case SymbolType::LOCAL:
                    emit1(generatedCode, "movsd", rp.sreg, std::format("qword [rbp - {}]", stackOffsets.at(varName)));
                    break;
                case SymbolType::PARAM:
                    break;
                case SymbolType::GLOBAL:
                    emit1(generatedCode, "movsd", rp.sreg, std::format("qword [rel {}]", varName));
                    break;
            }
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

void CodeGen::emitSection(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);

    if (cast::toNIL(var_->value)) {
        sectionBSS.emplace(cast::toString(var_->name)->data);
    } else {
        if (auto int_ = cast::toInt(var_->value)) {
            sectionData.emplace(cast::toString(var_->name)->data, std::to_string(int_->n));
        } else if (auto double_ = cast::toDouble(var_->value)) {
            uint64_t hex = *((uint64_t*) &double_->n);
            sectionData.emplace(cast::toString(var_->name)->data, std::format("0x{:X}", hex));
        }
    }
}

void CodeGen::emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op, RegisterPair& rp) {
    RegisterPair reg1{};
    RegisterPair reg2{};

    emitNumb(lhs, reg1);
    emitRHS(rhs, reg2);

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

