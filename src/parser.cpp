#include "parser.h"
#include "exceptions.hpp"
#include "visitors.hpp"

namespace {
constexpr const char* MISSING_PAREN_ERROR = "missing parenthesis";
constexpr const char* EXPECTED_INT_ERROR = "expected int";
constexpr const char* VAR_NOT_DEFINED = " is not defined";
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

    if (int value = checkVarError(left)) {
        left = std::make_unique<NumberExpr>(value);
    }

    if (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        right = parseSExpr();
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    } else {
        right = parseAtom();
        if (int value = checkVarError(right)) {
            right = std::make_unique<NumberExpr>(value);
        }
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
        if (int value = checkVarError(statement)) {
            statement = std::make_unique<NumberExpr>(value);
        }
    }

    return std::make_unique<PrintExpr>(statement);
}

ExprPtr Parser::parseDotimes() {
    ExprPtr statement, name, value;
    advance();

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    name = parseAtom();
    value = parseAtom();

    //TODO:fix print (* i i) replaces (* 10 10)
    symbolTable.emplace(StringEvaluator::getResult(name), IntEvaluator::getResult(value));
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    if (mCurrentToken.type == TokenType::LPAREN)
        statement = parseExpr();

    return std::make_unique<DotimesExpr>(value, statement);
}

ExprPtr Parser::parseLet() {
    ExprPtr sexpr;
    std::vector<ExprPtr> variables;

    advance();
    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    while (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        ExprPtr name = parseAtom();
        ExprPtr value = parseAtom();

        symbolTable.emplace(StringEvaluator::getResult(name), IntEvaluator::getResult(value));

        variables.emplace_back(std::make_unique<VarExpr>(name, value));
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    if (mCurrentToken.type == TokenType::LPAREN)
        sexpr = parseExpr();

    return std::make_unique<LetExpr>(sexpr, variables);

}

ExprPtr Parser::parseAtom() {
    if (mCurrentToken.type == TokenType::VAR) {
        Token token = mCurrentToken;
        advance();
        return std::make_unique<StringExpr>(token.value);
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

    throw InvalidSyntaxError(mFileName, EXPECTED_INT_ERROR, 0);
}

void Parser::consume(TokenType expected, const char* errorStr) {
    if (mCurrentToken.type != expected) {
        throw InvalidSyntaxError(mFileName, errorStr, 0);
    } else advance();
}

int Parser::checkVarError(ExprPtr& var) {
    std::string strvar = StringEvaluator::getResult(var);

    if (!strvar.empty()) {
        auto found = symbolTable.find(strvar);
        if (found == symbolTable.end()) {
            throw InvalidSyntaxError(mFileName, (strvar + VAR_NOT_DEFINED).c_str(), 0);
        }
        return found->second;
    }

    return 0;

}