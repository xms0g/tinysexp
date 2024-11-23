#include "codegen.h"
#include <typeindex>

std::string CodeGen::emit(ExprPtr& ast) {
    return ASTVisitor::getResult(ast);
}

void ASTVisitor::visit(const BinOpExpr& binop) {
    int lhsi, rhsi;

    lhsi = IntVisitor::getResult(binop.lhs);
    rhsi = IntVisitor::getResult(binop.rhs);

    store(code += std::string(lhsi, '+'));
    store(code += ">");
    store(code += std::string(rhsi, '+'));

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            store(code += "[<+>-]<");
            break;
        case TokenType::MINUS:
            store(code += "[<->-]<");
            break;
        case TokenType::DIV:
            store(code += std::format("<[{}>>+<<]>>[-<<+>>]", std::string(rhsi, '-')));
            break;
        case TokenType::MUL:
            store(code += std::format("-[<{}>-]",  std::string(lhsi, '+')));
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

    store(code += std::string(iterCount, '+'));
    store(code += "[>");

//    TokenType t = TokenVisitor::getResult(dotimes.statement);
//    if (t == TokenType::PLUS) {
//        store(code += "<<+>>");
//    }
//    store(code += "[");
    if (hasPrint) {
        store(code += VarVisitor::getResult(dotimes.statement));
    }
//
//    if (hasRecursive) {
//        store(code += ">");
//        store(code += ASTVisitor::getResult(dotimes.statement));
//        store(code += "<");
//    }
    if (!hasPrint)
        store(code += "<-]");
}

void ASTVisitor::visit(const PrintExpr& print) {
    store(code += ASTVisitor::getResult(print.sexpr));
    store(code += ".");
}

void ASTVisitor::visit(const LetExpr& let) {
    if (let.sexpr) {
        store(code += ASTVisitor::getResult(let.sexpr));
        return;
    }

    for (int i = 0; i < let.variables.size(); ++i) {
        int value = IntVisitor::getResult(let.variables[i]);
        store(code += std::string(value , '+'));

        if (let.variables.size() > 1 && i != let.variables.size() - 1)
            store(code += ">");
    }
}

void IntVisitor::visit(const NumberExpr& num) {
    store(num.n);
}

void IntVisitor::visit(const StringExpr& str) {
    store(-1);
}

void IntVisitor::visit(const BinOpExpr& binop) {
    int lhsi, rhsi;

    lhsi = IntVisitor::getResult(binop.lhs);
    rhsi = IntVisitor::getResult(binop.rhs);

    if (lhsi > 0) {
        store(lhsi);
    } else {
        store(rhsi);
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

void VarVisitor::visit(const BinOpExpr& binop) {
    int lhsi, rhsi;
    std::string bf;

    lhsi = IntVisitor::getResult(binop.lhs);
    rhsi = IntVisitor::getResult(binop.rhs);

//    if (lhsi < 0) {
//        step = rhsi;
//    } else if (rhsi < 0) {
//        step = lhsi;
//    }

    store(bf += std::string(lhsi, '+'));
    store(bf += ">");
    store(bf += std::string(rhsi, '+'));

    switch (binop.opToken.type) {
        case TokenType::PLUS:
            store(bf += "[<+>-]<");
            break;
        case TokenType::MINUS:
            store(bf += "[<->-]<");
            break;
        case TokenType::DIV:
            store(bf += std::format("<[{}>>+<<]>>[-<<+>>]", std::string(rhsi, '-')));
            break;
        case TokenType::MUL:
            store(bf += std::format("-[<{}>-]",  std::string(lhsi, '+')));
            break;
    }
}

void VarVisitor::visit(const PrintExpr& print) {
    std::string bf;
    store(bf += VarVisitor::getResult(print.sexpr));
    store(bf += ".");
    store(bf += "<->[-]<]");
}

void TokenVisitor::visit(const BinOpExpr& binop) {
    store(binop.opToken.type);
}

void TokenVisitor::visit(const PrintExpr& print) {
    store(TokenVisitor::getResult(print.sexpr));
}

