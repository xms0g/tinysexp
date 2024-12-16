#include "parser.h"
#include "exceptions.hpp"

namespace {
constexpr const char* MISSING_PAREN_ERROR = "missing parenthesis";
constexpr const char* EXPECTED_NUMBER_ERROR = "expected int or float";
}

Parser::Parser(const char* fn, Lexer& lexer) : mFileName(fn), mLexer(lexer), mTokenIndex(-1) {}

ExprPtr Parser::parse() {
    ExprPtr root, currentExpr, prevExpr;
    advance();

    root = parseExpr();
    prevExpr = root;

    while (mCurrentToken.type != TokenType::EOF_) {
        currentExpr = parseExpr();
        prevExpr->child = currentExpr;
        prevExpr = std::move(currentExpr);
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
        case TokenType::EQUAL:
        case TokenType::NEQUAL:
        case TokenType::GREATER_THEN:
        case TokenType::LESS_THEN:
        case TokenType::GREATER_THEN_EQ:
        case TokenType::LESS_THEN_EQ:
        case TokenType::AND:
        case TokenType::OR:
        case TokenType::NOT:
            expr = parseSExpr();
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
        case TokenType::DEFVAR:
            expr = parseDefvar();
            break;
        case TokenType::DEFCONST:
            expr = parseDefconst();
            break;
        case TokenType::DEFUN:
            expr = parseDefun();
            break;
        case TokenType::VAR:
            expr = parseFuncCall();
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

    return std::make_shared<BinOpExpr>(left, right, token);
}

ExprPtr Parser::parseDotimes() {
    std::vector<ExprPtr> statements;
    ExprPtr name, value;

    advance();

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    name = parseAtom();
    value = parseAtom();

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
    std::vector<ExprPtr> body;

    advance();

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    while (mCurrentToken.type == TokenType::VAR) {
        name = parseAtom();
        value = std::make_shared<NILExpr>();

        variables.emplace_back(std::make_shared<VarExpr>(name, value));
    }

    while (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        name = parseAtom();
        value = parseAtom();

        variables.emplace_back(std::make_shared<VarExpr>(name, value));
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);


    while (mCurrentToken.type == TokenType::LPAREN) {
        body.emplace_back(parseExpr());
    }

    return std::make_shared<LetExpr>(variables, body);
}

ExprPtr Parser::parseSetq() {
    ExprPtr var, name, value;
    advance();

    name = parseAtom();

    if (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        value = parseSExpr();
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    } else {
        value = parseAtom();
    }

    var = std::make_shared<VarExpr>(name, value);
    return std::make_shared<SetqExpr>(var);
}

ExprPtr Parser::parseDefvar() {
    ExprPtr var, name, value;
    advance();

    name = parseAtom();

    if (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        value = parseSExpr();
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    } else {
        value = parseAtom();
    }

    var = std::make_shared<VarExpr>(name, value);
    return std::make_shared<DefvarExpr>(var);
}

ExprPtr Parser::parseDefconst() {
    ExprPtr var, name, value;
    advance();

    name = parseAtom();

    if (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
        value = parseSExpr();
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    } else {
        value = parseAtom();
    }

    var = std::make_shared<VarExpr>(name, value);
    return std::make_shared<DefconstExpr>(var);
}

ExprPtr Parser::parseDefun() {
    ExprPtr name;
    std::vector<ExprPtr> params;
    std::vector<ExprPtr> body;

    advance();

    name = parseAtom();

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    while (mCurrentToken.type == TokenType::VAR) {
        params.emplace_back(parseAtom());
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    while (mCurrentToken.type == TokenType::LPAREN) {
        body.emplace_back(parseExpr());
    }

    return std::make_shared<DefunExpr>(name, params, body);
}

ExprPtr Parser::parseFuncCall() {
    ExprPtr name;
    std::vector<ExprPtr> params;

    name = parseAtom();

    while (mCurrentToken.type == TokenType::INT ||
           mCurrentToken.type == TokenType::DOUBLE ||
           mCurrentToken.type == TokenType::VAR) {
        params.emplace_back(parseAtom());
    }

    return std::make_shared<FuncCallExpr>(name, params);
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
    Token token = mCurrentToken;
    advance();

    if (token.type == TokenType::INT) {
        return std::make_shared<IntExpr>(std::stoi(token.value));
    } else if (token.type == TokenType::DOUBLE) {
        return std::make_shared<DoubleExpr>(std::stof(token.value));
    }

    throw InvalidSyntaxError(mFileName, EXPECTED_NUMBER_ERROR, 0);
}

void Parser::consume(TokenType expected, const char* errorStr) {
    if (mCurrentToken.type != expected) {
        throw InvalidSyntaxError(mFileName, errorStr, 0);
    } else advance();
}

ExprPtr Parser::checkVarError(ExprPtr& var) {
//    std::string strvar = StringEvaluator::get(var);
//
//    if (!strvar.empty()) {
//        auto found = symbolTable.find(strvar);
//        if (found == symbolTable.end()) {
//            throw InvalidSyntaxError(mFileName, (strvar + VAR_NOT_DEFINED).c_str(), 0);
//        }
//        return found->second;
//    }
//
//    return nullptr;

}