#include "codegen.h"

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
            store(code += ".");
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

    store(ASTVisitor::getResult(let.sexpr));
//    switch (let.sexpr->type()) {
//        case TokenType::PLUS:
//            // 3 + 2
//            // +++>++[<+>-]
//            break;
//        case TokenType::MINUS:
//            // 3 - 2
//            // +++>++[<->-]
//            break;
//        case TokenType::DIV:
//            break;
//        case TokenType::MUL:
//            // 3 * 2
//            // +++>++-[<+++>-]
//            break;
//        case TokenType::PRINT:
//            break;
//        case TokenType::DOTIMES:
//            break;
//        case TokenType::VAR:
//            break;
//        case TokenType::LET:
//            break;
//    }
}

void ASTVisitor::visit(const VarExpr&) {

}

void IntVisitor::visit(const NumberExpr& num) {
    store(num.n);
}

void IntVisitor::visit(const BinOpExpr& binop) {
    int lhsi, rhsi;

    lhsi = IntVisitor::getResult(binop.lhs);
    rhsi = IntVisitor::getResult(binop.rhs);

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            store(lhsi + rhsi);
        case TokenType::MINUS:
            store(lhsi - rhsi);
        case TokenType::DIV:
            store(lhsi / rhsi);
        case TokenType::MUL:
            store(lhsi * rhsi);
    }
}

void IntVisitor::visit(const VarExpr& var) {
    store(IntVisitor::getResult(var.value));
}

void TypeVisitor::visit(const PrintExpr&) {
    store(typeid(PrintExpr).hash_code());
}

void TypeVisitor::visit(const DotimesExpr&) {
    store(typeid(DotimesExpr).hash_code());
}


