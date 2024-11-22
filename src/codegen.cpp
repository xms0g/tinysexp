#include "codegen.h"
#include <typeindex>

std::string CodeGen::emit(ExprPtr& ast) {
    return ASTVisitor::getResult(ast);
}

void ASTVisitor::visit(const BinOpExpr& binop) {
    int lhsi, rhsi;

    lhsi = IntVisitor::getResult(binop.lhs);
    rhsi = IntVisitor::getResult(binop.rhs);

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            for (int i = 0; i < lhsi + rhsi; ++i) {
                store(code += "+");
            }
            break;
        case TokenType::MINUS:
            for (int i = 0; i < lhsi - rhsi; ++i) {
                store(code += "+");
            }
            break;
        case TokenType::DIV:
            for (int i = 0; i < lhsi / rhsi; ++i) {
                store(code += "+");
            }
            break;
        case TokenType::MUL:
            for (int i = 0; i < lhsi * rhsi; ++i) {
                store(code += "+");
            }
            break;
    }
}

void ASTVisitor::visit(const DotimesExpr& dotimes) {
    int iterCount = IntVisitor::getResult(dotimes.iterationCount);
    bool hasPrint{false}, hasRecursive{false};

    if (dotimes.statement) {
        hasPrint = TypeVisitor::getResult(dotimes.statement) == typeid(PrintExpr).hash_code();
        hasRecursive = TypeVisitor::getResult(dotimes.statement) == typeid(DotimesExpr).hash_code();
    }

    // ++++[>++[-]<-]
    for (int i = 0; i < iterCount; ++i) {
        if (hasPrint) {
            store(code += ASTVisitor::getResult(dotimes.statement)); //TODO: (print (+ i 1)) case not handled
        }
        store(code += "+");
    }

    store(code += "[");
    if (hasRecursive) {
        store(code += ">");
        store(code += ASTVisitor::getResult(dotimes.statement));
        store(code += "<");
    }
    store(code += "-]");
}

void ASTVisitor::visit(const PrintExpr& print) {
    int sexpr = IntVisitor::getResult(print.sexpr);

    for (int i = 0; i < sexpr; ++i) {
        store(code += "+");
    }
    store(code += ".");
}

void ASTVisitor::visit(const LetExpr& let) {
    for (int i = 0; i < let.variables.size(); ++i) {
        int value = IntVisitor::getResult(let.variables[i]);

        for (int j = 0; j < value; ++j) {
            store(code += "+");
        }

        if (let.variables.size() > 1 && i != let.variables.size() - 1)
            store(code += ">");
    }

    if (let.sexpr) {
        store(code += ">");
        store(code += ASTVisitor::getResult(let.sexpr));
    }
}

void IntVisitor::visit(const NumberExpr& num) {
    store(num.n);
}

void IntVisitor::visit(const StringExpr& str) {
    store(0);
}

void IntVisitor::visit(const BinOpExpr& binop) {
    int lhsi, rhsi;

    lhsi = IntVisitor::getResult(binop.lhs);
    rhsi = IntVisitor::getResult(binop.rhs);

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            store(lhsi + rhsi);
            break;
        case TokenType::MINUS:
            store(lhsi - rhsi);
            break;
        case TokenType::DIV:
            store(lhsi / rhsi);
            break;
        case TokenType::MUL:
            store(lhsi * rhsi);
            break;
    }
}

void IntVisitor::visit(const VarExpr& var) {
    store(IntVisitor::getResult(var.value));
}

void StringVisitor::visit(const VarExpr& var) {
    store(StringVisitor::getResult(var.name));
}

void StringVisitor::visit(const StringExpr& str) {
    store(str.str);
}

void TypeVisitor::visit(const PrintExpr&) {
    store(typeid(PrintExpr).hash_code());
}

void TypeVisitor::visit(const DotimesExpr&) {
    store(typeid(DotimesExpr).hash_code());
}

