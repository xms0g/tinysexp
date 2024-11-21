#include "parser.h"
#include "exceptions.hpp"

namespace {
constexpr const char* MISSING_PAREN_ERROR = "Missing Parenthesis";
}

Parser::Parser(const char* fn, Lexer& lexer) : mFileName(fn), mLexer(lexer), mTokenIndex(-1) {}

ExprPtr Parser::parse() {
    advance();
    return parseExpr();
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
        case TokenType::DOTIMES:
            expr = parseDotimes();
            break;
        case TokenType::LET:
            expr = parseLet();
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

    if (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        right = parseSExpr();
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    } else {
        right = parseAtom();
    }

    return std::make_unique<BinOpExpr>(left, right, token);
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
    }

    return std::make_unique<PrintExpr>(statement);
}

ExprPtr Parser::parseDotimes() {
    ExprPtr statement, iterationCount;
    advance();

    iterationCount = parseVar();

    if (mCurrentToken.type != TokenType::RPAREN)
        statement = parseExpr();

    return std::make_unique<DotimesExpr>(iterationCount, statement);
}

ExprPtr Parser::parseLet() {
    ExprPtr sexpr;
    std::vector<ExprPtr> variables;

    advance();
    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    while (mCurrentToken.type == TokenType::LPAREN) {
        variables.push_back(parseVar());
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    sexpr = parseSExpr();
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    return std::make_unique<LetExpr>(sexpr, variables);

}

ExprPtr Parser::parseVar() {
    ExprPtr var, num;

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    var = parseAtom();
    num = parseAtom();
    dynamic_cast<VarExpr*>(var.get())->value = std::move(num);
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    return var;
}

ExprPtr Parser::parseAtom() {
    if (mCurrentToken.type == TokenType::VAR) {
        Token token = mCurrentToken;
        advance();
        return std::make_unique<VarExpr>(token.value);
    } else {
        return parseNumber();
    }
}

ExprPtr Parser::parseNumber() {
    if (mCurrentToken.type == TokenType::INT) {
        Token token = mCurrentToken;
        advance();
        return std::make_unique<NumberExpr>(std::stoi(token.value));
    }

    throw InvalidSyntaxError(mFileName, "Expected INT", 0);
}

void Parser::consume(TokenType expected, const char* errorStr) {
    if (mCurrentToken.type != expected) {
        throw InvalidSyntaxError(mFileName, errorStr, 0);
    } else advance();
}