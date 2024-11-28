#include "parser.h"
#include "exceptions.hpp"

namespace {
constexpr const char* MISSING_PAREN_ERROR = "missing parenthesis";
constexpr const char* EXPECTED_INT_ERROR = "expected int";
constexpr const char* VAR_NOT_DEFINED = " is not defined";
}

Parser::Parser(const char* fn, Lexer& lexer) : mFileName(fn), mLexer(lexer), mTokenIndex(-1) {}

ExprPtr Parser::parse() {
    ExprPtr root, currentExpr, prevExpr;

    advance();

    while (mCurrentToken.type != TokenType::EOF_) {
        currentExpr = parseExpr();
        if (prevExpr) {
            prevExpr->child = currentExpr;
            prevExpr = prevExpr->child;
        } else {
            prevExpr = currentExpr;
            root = prevExpr;
        }
    }
    return root;
}

Token Parser::advance() {
    ++mTokenIndex;

    if (mTokenIndex < mLexer.getTokenSize()) {
        mCurrentToken = mLexer.getToken(mTokenIndex);
    }

    return mCurrentToken;
}

ExprPtr Parser::parseExpr() {
    ExprPtr expr;

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);

    switch (mCurrentToken.type) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::DIV:
        case TokenType::MUL:
            expr = parseSExpr();
            break;
        case TokenType::PRINT:
            expr = parsePrint();
            break;
        case TokenType::READ:
            expr = parseRead();
            break;
        case TokenType::DOTIMES:
            expr = parseDotimes();
            break;
        case TokenType::LET:
            expr = parseLet();
            break;
        case TokenType::SETQ:
            expr = parseSetq();
            break;
        default:
            throw InvalidSyntaxError(mFileName, mCurrentToken.value.c_str(), 0);
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    return expr;
}

ExprPtr Parser::parseSExpr() {
    ExprPtr left, right;

    Token token = mCurrentToken;
    advance();

    left = parseAtom();
    strToVar(left);

    if (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        right = parseSExpr();
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    } else {
        right = parseAtom();
        strToVar(right);
    }

    return std::make_shared<BinOpExpr>(left, right, token);
}

ExprPtr Parser::parsePrint() {
    ExprPtr statement;

    advance();

    if (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        statement = parseSExpr();
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    } else {
        statement = parseAtom();
        strToVar(statement);
    }

    return std::make_shared<PrintExpr>(statement);
}

ExprPtr Parser::parseRead() {
    advance();
    return std::make_shared<ReadExpr>();;
}

ExprPtr Parser::parseDotimes() {
    std::vector<ExprPtr> statements;
    ExprPtr name, value;

    advance();

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    name = parseAtom();
    value = parseAtom();

    //TODO:fix print (* i i) replaces (* 10 10)
    symbolTable.emplace(name->asStr().str, value);
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    if (mCurrentToken.type == TokenType::LPAREN) {
        while (mCurrentToken.type == TokenType::LPAREN) {
            statements.emplace_back(parseExpr());
        }
    }

    return std::make_shared<DotimesExpr>(value, statements);
}

ExprPtr Parser::parseLet() {
    ExprPtr name, value;
    std::vector<ExprPtr> variables;
    std::vector<ExprPtr> sexprs;

    advance();

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    while (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        name = parseAtom();
        value = parseAtom();
        symbolTable.emplace(name->asStr().str, value);
        variables.emplace_back(std::make_shared<VarExpr>(name, value));
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    }

    while (mCurrentToken.type == TokenType::VAR) {
        name = parseAtom();
        value = std::make_shared<NILExpr>();
        symbolTable.emplace(name->asStr().str, value);
        variables.emplace_back(std::make_shared<VarExpr>(name, value));
    }

    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    if (mCurrentToken.type == TokenType::LPAREN) {
        while (mCurrentToken.type == TokenType::LPAREN) {
            sexprs.emplace_back(parseExpr());
        }
    }

    return std::make_shared<LetExpr>(sexprs, variables);

}

ExprPtr Parser::parseSetq() {
    ExprPtr var, name, value;
    advance();

    name = parseAtom();

    checkVarError(name);

    if (mCurrentToken.type == TokenType::LPAREN) {
        value = parseExpr();
    } else {
        value = parseAtom();
    }

    symbolTable[name->asStr().str] = value;

    var = std::make_shared<VarExpr>(name, value);
    return std::make_shared<SetqExpr>(var);
}

ExprPtr Parser::parseAtom() {
    if (mCurrentToken.type == TokenType::VAR) {
        Token token = mCurrentToken;
        advance();
        return std::make_shared<StringExpr>(token.value);
    } else {
        return parseNumber();
    }
}

ExprPtr Parser::parseNumber() {
    if (mCurrentToken.type == TokenType::INT) {
        Token token = mCurrentToken;
        advance();
        return std::make_shared<NumberExpr>(std::stoi(token.value));
    }

    throw InvalidSyntaxError(mFileName, EXPECTED_INT_ERROR, 0);
}

void Parser::consume(TokenType expected, const char* errorStr) {
    if (mCurrentToken.type != expected) {
        throw InvalidSyntaxError(mFileName, errorStr, 0);
    } else advance();
}

ExprPtr Parser::checkVarError(ExprPtr& expr) {
    std::string name = expr->asStr().str;
    auto found = symbolTable.find(name);
    if (found == symbolTable.end()) {
        throw InvalidSyntaxError(mFileName, (name + VAR_NOT_DEFINED).c_str(), 0);
    }
    return found->second;
}

void Parser::strToVar(ExprPtr& expr) {
    if (expr->type() == ExprType::STR) {
        if (ExprPtr value = checkVarError(expr)) {
            expr = std::make_shared<VarExpr>(expr, value);
        }
    }
}