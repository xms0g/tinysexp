#include "codegen.h"

std::string CodeGen::emit(ExprPtr& ast) {
    std::string code;

    switch (ast->type()) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::DIV:
        case TokenType::MUL:
            emitBinOp(ast, code);
            break;
        case TokenType::PRINT:
            emitPrint(ast, code);
            break;
        case TokenType::DOTIMES:
            emitDotimes(ast, code);
            break;
        case TokenType::LET:
            emitLet(ast, code);
            break;
    }

    return code;
}

int CodeGen::emitSExpr(ExprPtr& expr) {
    int lhsi, rhsi;

    if (expr->type() == TokenType::INT) {
        return expr->asNumber()->n;
    } else if (expr->type() == TokenType::VAR) {
        return expr->asVar()->value->asNumber()->n;
    } else {
        lhsi = emitSExpr(expr->asBinOp()->lhs);
        rhsi = emitSExpr(expr->asBinOp()->rhs);

        switch (expr->asBinOp()->opToken.type) {
            case TokenType::PLUS:
                return lhsi + rhsi;
            case TokenType::MINUS:
                return lhsi - rhsi;
            case TokenType::DIV:
                return lhsi / rhsi;
            case TokenType::MUL:
                return lhsi * rhsi;
        }
    }

    return 0;
}

void CodeGen::emitBinOp(ExprPtr& expr, std::string& code) {
    int lhsi, rhsi;

    lhsi = emitSExpr(expr->asBinOp()->lhs);
    rhsi = emitSExpr(expr->asBinOp()->rhs);

    switch (expr->asBinOp()->opToken.type) {
        case TokenType::PLUS:
            for (int i = 0; i < lhsi + rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::MINUS:
            for (int i = 0; i < lhsi - rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::DIV:
            for (int i = 0; i < lhsi / rhsi; ++i) {
                code += "+";
            }
            break;
        case TokenType::MUL:
            for (int i = 0; i < lhsi * rhsi; ++i) {
                code += "+";
            }
            break;
    }
}

void CodeGen::emitDotimes(ExprPtr& expr, std::string& code) {
    int iterCount = emitSExpr(expr->asDotimes()->iterationCount->asVar()->value);
    bool hasPrint{false}, hasRecursive{false};

    if (expr->asDotimes()->statement) {
        hasPrint = expr->asDotimes()->statement->type() == TokenType::PRINT;
        hasRecursive = expr->asDotimes()->statement->type() == TokenType::DOTIMES;
    }

    // ++++[>++[-]<-]
    for (int i = 0; i < iterCount; ++i) {
        if (hasPrint) {
            code += ".";
        }
        code += "+";
    }

    code += "[";
    if (hasRecursive) {
        code += ">";
        emitDotimes(expr->asDotimes()->statement, code);
        code += "<";
    }
    code += "-]";
}

void CodeGen::emitPrint(ExprPtr& expr, std::string& code) {
    int sexpr = emitSExpr(expr->asPrint()->sexpr);

    for (int i = 0; i < sexpr; ++i) {
        code += "+";
    }
    code += ".";
}

void CodeGen::emitLet(ExprPtr& expr, std::string& code) {
    LetExpr* let = expr->asLet();

    for (int i = 0; i < let->variables.size(); ++i) {
        NumberExpr* var = let->variables[i]->asNumber();

        for (int j = 0; j < var->n; ++j) {
            code += "+";
        }

        if (let->variables.size() > 1 && i != let->variables.size() - 1)
            code += ">";
    }

    switch (let->sexpr->type()) {
        case TokenType::PLUS:
            // 3 + 2
            // +++>++[<+>-]
            break;
        case TokenType::MINUS:
            // 3 - 2
            // +++>++[<->-]
            break;
        case TokenType::DIV:
            break;
        case TokenType::MUL:
            // 3 * 2
            // +++>++-[<+++>-]
            break;
        case TokenType::PRINT:
            break;
        case TokenType::DOTIMES:
            break;
        case TokenType::VAR:
            break;
        case TokenType::LET:
            break;
    }
}
