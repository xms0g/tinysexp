#include "codegen.h"
#include <format>

#define VISIT(L, R, OP) \
    std::visit([&](auto var1, auto var2) { \
        OP \
    }, L, R)

static const std::unordered_map<Register, std::string> stringRepFromReg = {
        {Register::RAX, "RAX"},
        {Register::RBX, "RBX"},
        {Register::RCX, "RCX"},
        {Register::RDX, "RDX"},
        {Register::RDI, "RDI"},
        {Register::RSI, "RSI"},
        {Register::R8,  "R8"},
        {Register::R9,  "R9"},
        {Register::R10, "R10"},
        {Register::R11, "R11"},
        {Register::R12, "R12"},
        {Register::R13, "R13"},
        {Register::R14, "R14"},
        {Register::R15, "R15"},
};

std::string CodeGen::emit(ExprPtr& ast) {
    std::string generatedCode =
            "section .text\n"
            "\tglobal _start\n"
            "_start:\n"
            "\tpush rbp\n"
            "\tmov rbp, rsp\n";

    while (ast != nullptr) {
        generatedCode += ASTVisitor::get(ast);
        ast = ast->child;
    }
    generatedCode += "\tpop rbp\n"
                     "\tret\n";

    generatedCode += !sectionData.empty() ? "section .data\n" : "";
    for (auto& var: sectionData) {
        generatedCode += std::format("{}: dq {}", var.first, var.second);
    }
    return generatedCode;
}


void ASTVisitor::visit(const BinOpExpr& binop) {
    std::variant<int, double> lhs, rhs;
    std::string reg;

    lhs = NumberEvaluator::get(binop.lhs);
    rhs = NumberEvaluator::get(binop.rhs);

    // unsigned long long l = *((unsigned long long*)&dToUse);

    reg = (std::holds_alternative<double>(lhs) || std::holds_alternative<double>(rhs)) ? "xmm0" : "rax";

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            VISIT(lhs, rhs,
                  set(code += std::format("\tmov {}, {}\n", reg, var1 + var2));
            );
            break;
        case TokenType::MINUS:
            VISIT(lhs, rhs,
                  set(code += std::format("\tmov {}, {}\n", reg, var1 - var2));
            );
            break;
        case TokenType::DIV:
            VISIT(lhs, rhs,
                  set(code += std::format("\tmov {}, {}\n", reg, var1 / var2));
            );
            break;
        case TokenType::MUL:
            VISIT(lhs, rhs,
                  set(code += std::format("\tmov {}, {}\n", reg, var1 * var2));
            );
            break;
    }
}

void ASTVisitor::visit(const DotimesExpr& dotimes) {

}

void ASTVisitor::visit(const LetExpr& let) {
    for (auto& var: let.variables) {
        std::string vs = VarEvaluator::get(var);
        set(code += std::format("\tmov qword [rbp - {}], {}\n",
                                CodeGen::stackOffset, vs));

        CodeGen::stackOffsets.emplace(vs, CodeGen::stackOffset);
        CodeGen::stackOffset += 8;
    }

    for (auto& sexpr: let.body) {
        set(code += VarEvaluator::get(sexpr));
    }

}

void ASTVisitor::visit(const SetqExpr& setq) {

}

void ASTVisitor::visit(const DefvarExpr& defvar) {
    CodeGen::sectionData.emplace(StringEvaluator::get(defvar.var), VarEvaluator::get(defvar.var));
}

void NumberEvaluator::visit(const IntExpr& num) {
    set(num.n);
}

void NumberEvaluator::visit(const DoubleExpr& num) {
    set(num.n);
}

void NumberEvaluator::visit(const BinOpExpr& binop) {
    std::variant<int, double> lhs, rhs;

    lhs = NumberEvaluator::get(binop.lhs);
    rhs = NumberEvaluator::get(binop.rhs);

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            VISIT(lhs, rhs,
                  set(var1 + var2);
            );
            break;
        case TokenType::MINUS:
            VISIT(lhs, rhs,
                  set(var1 - var2);
            );
            break;
        case TokenType::DIV:
            VISIT(lhs, rhs,
                  set(var1 / var2);
            );
            break;
        case TokenType::MUL:
            VISIT(lhs, rhs,
                  set(var1 * var2);
            );
            break;
    }
}

void VarEvaluator::visit(const BinOpExpr& binop) {
    std::string lhs, rhs, code;

    lhs = VarEvaluator::get(binop.lhs);
    rhs = VarEvaluator::get(binop.rhs);

    //TODO: remove then
    auto isNumber = [&](std::string& s) {
        char* p;
        strtol(s.c_str(), &p, 10);
        return *p == 0;
    };

    int loffset, roffset;

    //TODO: do in semantic analyse
    bool isRHSNum = isNumber(rhs);
    bool isLHSGlobal = CodeGen::sectionData.contains(StringEvaluator::get(binop.lhs));
    bool isRHSGlobal = CodeGen::sectionData.contains(StringEvaluator::get(binop.rhs));

    switch (binop.opToken.type) {
        case TokenType::PLUS: {
            if (!isRHSNum) {
                set(code += rhs);

                if (isLHSGlobal) {
                    std::string name = StringEvaluator::get(binop.lhs);
                    set(code += std::format("\tadd rax, qword [rip + {}]\n", name));
                } else {
                    if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
                        loffset = itr->second;
                        set(code += std::format("\tadd rax, qword [rbp - {}]\n", loffset));
                    } else {
                        set(code += std::format("\tadd rax, {}\n", lhs));
                    }
                }
            } else {
                if (isLHSGlobal) {
                    std::string name = StringEvaluator::get(binop.lhs);
                    set(code += std::format("\tmov rax, qword [rip + {}]\n", name));
                } else {
                    if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
                        loffset = itr->second;
                        set(code += std::format("\tmov rax, qword [rbp - {}]\n", loffset));
                    } else {
                        set(code += std::format("\tmov rax, {}\n", lhs));
                    }
                }

                if (isRHSGlobal) {
                    std::string name = StringEvaluator::get(binop.rhs);
                    set(code += std::format("\tadd rax, qword [rip + {}]\n", name));
                } else {
                    if (auto itr = CodeGen::stackOffsets.find(rhs); itr != CodeGen::stackOffsets.end()) {
                        roffset = itr->second;
                        set(code += std::format("\tadd rax, qword [rbp - {}]\n", roffset));
                    } else {
                        set(code += std::format("\tadd rax, {}\n", rhs));
                    }
                }

            }

            break;
        }
        case TokenType::MINUS: {
            if (!isRHSNum) {
                set(code += rhs);
                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
                    loffset = itr->second;
                    set(code += std::format("\tsub rax, qword [rbp - {}]\n", loffset));
                } else {
                    set(code += std::format("\tsub rax, {}\n", lhs));
                }
            } else {
                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
                    loffset = itr->second;
                    set(code += std::format("\tmov rax, qword [rbp - {}]\n", loffset));
                } else {
                    set(code += std::format("\tmov rax, {}\n", lhs));
                }

                if (auto itr = CodeGen::stackOffsets.find(rhs); itr != CodeGen::stackOffsets.end()) {
                    roffset = itr->second;
                    set(code += std::format("\tsub rax, qword [rbp - {}]\n", roffset));
                } else {
                    set(code += std::format("\tsub rax, {}\n", rhs));
                }
            }
            break;
        }
        case TokenType::DIV: {
            if (!isRHSNum) {
                set(code += rhs);
                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
                    loffset = itr->second;
                    set(code += std::format("\tidiv rax, qword [rbp - {}]\n", loffset));
                } else {
                    set(code += std::format("\tidiv rax, {}\n", lhs));
                }
            } else {
                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
                    loffset = itr->second;
                    set(code += std::format("\tmov rax, qword [rbp - {}]\n", loffset));
                } else {
                    set(code += std::format("\tmov rax, {}\n", lhs));
                }

                if (auto itr = CodeGen::stackOffsets.find(rhs); itr != CodeGen::stackOffsets.end()) {
                    roffset = itr->second;
                    set(code += std::format("\tidiv rax, qword [rbp - {}]\n", roffset));
                } else {
                    set(code += std::format("\tidiv rax, {}\n", rhs));
                }
            }
            break;
        }
        case TokenType::MUL: {
            if (!isRHSNum) {
                set(code += rhs);
                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
                    loffset = itr->second;
                    set(code += std::format("\timul rax, qword [rbp - {}]\n", loffset));
                } else {
                    set(code += std::format("\timul rax, {}\n", lhs));
                }
            } else {
                if (auto itr = CodeGen::stackOffsets.find(lhs); itr != CodeGen::stackOffsets.end()) {
                    loffset = itr->second;
                    set(code += std::format("\tmov rax, qword [rbp - {}]\n", loffset));
                } else {
                    set(code += std::format("\tmov rax, {}\n", lhs));
                }

                if (auto itr = CodeGen::stackOffsets.find(rhs); itr != CodeGen::stackOffsets.end()) {
                    roffset = itr->second;
                    set(code += std::format("\timul rax, qword [rbp - {}]\n", roffset));
                } else {
                    set(code += std::format("\timul rax, {}\n", rhs));
                }
            }
            break;
        }
    }
}

void VarEvaluator::visit(const VarExpr& var) {
    set(VarEvaluator::get(var.value));
}

void VarEvaluator::visit(const IntExpr& num) {
    set(std::to_string(num.n));
}

void VarEvaluator::visit(const StringExpr& str) {
    set(str.data);
}

void StringEvaluator::visit(const StringExpr& str) {
    set(str.data);
}

void StringEvaluator::visit(const VarExpr& var) {
    set(StringEvaluator::get(var.name));
}

