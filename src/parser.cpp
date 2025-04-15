#include "parser.h"
#include "exceptions.hpp"

Parser::Parser(const char* fn, Lexer& lexer) : lexer(lexer), tokenIndex(-1), fileName(fn) {
}

ExprPtr Parser::parse() {
    advance();

    ExprPtr root = parseExpr();
    ExprPtr prevExpr = root;

    while (currentToken.type != TokenType::EOF_) {
        ExprPtr currentExpr = parseExpr();
        prevExpr->child = currentExpr;
        prevExpr = std::move(currentExpr);
    }
    return root;
}

Token Parser::advance() {
    ++tokenIndex;

    if (tokenIndex < lexer.getTokenSize()) {
        currentToken = lexer.getToken(tokenIndex);
    }

    return currentToken;
}

ExprPtr Parser::parseExpr() {
    ExprPtr expr;

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    switch (currentToken.type) {
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
        case TokenType::LOGAND:
        case TokenType::LOGIOR:
        case TokenType::LOGXOR:
        case TokenType::LOGNOR:
            expr = parseSExpr();
            break;
        case TokenType::DOTIMES:
            expr = parseDotimes();
            break;
        case TokenType::LOOP:
            expr = parseLoop();
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
        case TokenType::IF:
            expr = parseIf();
            break;
        case TokenType::WHEN:
            expr = parseWhen();
            break;
        case TokenType::COND:
            expr = parseCond();
            break;
        case TokenType::VAR:
            expr = parseFuncCall();
            break;
        case TokenType::RETURN:
            expr = parseReturn();
            break;
        default:
            throw InvalidSyntaxError(fileName, currentToken.lexeme.c_str(), 0);
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    return expr;
}

ExprPtr Parser::parseSExpr() {
    ExprPtr left, right;

    Token token = currentToken;
    advance();

    if (currentToken.type == TokenType::LPAREN) {
        left = parseExpr();
    } else {
        left = parseAtom();
    }

    if (currentToken.type == TokenType::LPAREN) {
        right = parseExpr();
    } else {
        right = parseAtom();
    }

    if (token.type == TokenType::NOT && !cast::toUninitialized(right)) {
        throw InvalidSyntaxError(fileName, ERROR(OP_INVALID_NUMBER_OF_ARGS_ERROR, "NOT", 2), 0);
    }

    return std::make_shared<BinOpExpr>(left, right, token);
}

ExprPtr Parser::parseDotimes() {
    ExprPtr value;
    std::vector<ExprPtr> statements;

    advance();

    consume(TokenType::LPAREN, ERROR(EXPECTED_ELEMS_NUMBER_ERROR, "DOTIMES"));
    ExprPtr var = parseAtom();

    if (currentToken.type == TokenType::LPAREN) {
        value = parseExpr();
    } else {
        value = parseAtom();
    }

    cast::toVar(var)->value = std::move(value);
    cast::toVar(var)->sType = SymbolType::LOCAL;

    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);


    while (currentToken.type == TokenType::LPAREN) {
        statements.push_back(parseExpr());
    }

    return std::make_shared<DotimesExpr>(var, statements);
}

ExprPtr Parser::parseLoop() {
    std::vector<ExprPtr> sexprs;

    advance();

    while (currentToken.type == TokenType::LPAREN) {
        sexprs.push_back(parseExpr());
    }

    return std::make_shared<LoopExpr>(sexprs);
}

ExprPtr Parser::parseLet() {
    ExprPtr var, value;
    std::vector<ExprPtr> bindings;
    std::vector<ExprPtr> body;

    advance();

    consume(TokenType::LPAREN, ERROR(EXPECTED_ELEMS_NUMBER_ERROR, "LET"));
    for (;;) {
        // Check out (let (x))
        while (currentToken.type == TokenType::VAR) {
            var = parseAtom();
            cast::toVar(var)->sType = SymbolType::LOCAL;
            bindings.push_back(var);
        }

        // Check out (let ((x 11)) )
        while (currentToken.type == TokenType::LPAREN) {
            consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
            var = parseAtom();

            if (currentToken.type == TokenType::LPAREN) {
                value = parseExpr();
            } else {
                value = parseAtom();
            }

            cast::toVar(var)->value = std::move(value);
            cast::toVar(var)->sType = SymbolType::LOCAL;
            bindings.push_back(var);
            consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
        }

        if (currentToken.type == TokenType::RPAREN)
            break;
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    while (currentToken.type == TokenType::LPAREN) {
        body.push_back(parseExpr());
    }

    return std::make_shared<LetExpr>(bindings, body);
}

ExprPtr Parser::parseSetq() {
    ExprPtr var = createVar(SymbolType::UNKNOWN);
    return std::make_shared<SetqExpr>(var);
}

ExprPtr Parser::parseDefvar() {
    ExprPtr var = createVar(SymbolType::GLOBAL);
    return std::make_shared<DefvarExpr>(var);
}

ExprPtr Parser::parseDefconst() {
    ExprPtr var = createVar(SymbolType::GLOBAL, true);
    return std::make_shared<DefconstExpr>(var);
}

ExprPtr Parser::parseDefun() {
    std::vector<ExprPtr> args;
    std::vector<ExprPtr> forms;

    advance();

    ExprPtr name = parseAtom();

    // Parse params
    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    while (currentToken.type == TokenType::VAR) {
        ExprPtr arg = parseAtom();
        cast::toVar(arg)->sType = SymbolType::PARAM;
        args.push_back(arg);
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    // Parse body
    for (;;) {
        if (currentToken.type == TokenType::LPAREN) {
            forms.push_back(parseExpr());
        } else {
            forms.push_back(parseAtom());
        }

        if (currentToken.type == TokenType::RPAREN)
            break;
    }

    return std::make_shared<DefunExpr>(name, args, forms);
}

ExprPtr Parser::parseFuncCall() {
    std::vector<ExprPtr> args;

    ExprPtr name = parseAtom();

    for (;;) {
        if (currentToken.type == TokenType::LPAREN) {
            args.push_back(parseExpr());
        } else {
            ExprPtr arg = parseAtom();

            if (cast::toUninitialized(arg))
                break;

            args.push_back(arg);
        }

        if (currentToken.type == TokenType::RPAREN)
            break;
    }

    return std::make_shared<FuncCallExpr>(name, args);
}

ExprPtr Parser::parseReturn() {
    advance();

    ExprPtr arg = parseAtom();

    return std::make_shared<ReturnExpr>(arg);
}

ExprPtr Parser::parseIf() {
    ExprPtr test, then, else_;

    advance();

    if (currentToken.type == TokenType::LPAREN) {
        test = parseExpr();
    } else {
        test = parseAtom();
    }

    if (currentToken.type == TokenType::LPAREN) {
        then = parseExpr();
    } else {
        then = parseAtom();
    }

    if (currentToken.type == TokenType::LPAREN) {
        else_ = parseExpr();
    } else {
        else_ = parseAtom();
    }

    return std::make_shared<IfExpr>(test, then, else_);
}

ExprPtr Parser::parseWhen() {
    ExprPtr test;
    std::vector<ExprPtr> then;

    advance();

    if (currentToken.type == TokenType::LPAREN) {
        test = parseExpr();
    } else {
        test = parseAtom();
    }

    for (;;) {
        if (currentToken.type == TokenType::LPAREN) {
            then.push_back(parseExpr());
        } else {
            then.push_back(parseAtom());
        }

        if (currentToken.type == TokenType::RPAREN)
            break;
    }

    return std::make_shared<WhenExpr>(test, then);
}

ExprPtr Parser::parseCond() {
    ExprPtr test;
    std::vector<std::pair<ExprPtr, std::vector<ExprPtr> > > variants;

    advance();

    while (currentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);

        if (currentToken.type == TokenType::LPAREN) {
            test = parseExpr();
        } else {
            test = parseAtom();
        }

        std::vector<ExprPtr> statements;
        if (currentToken.type != TokenType::LPAREN) {
            statements.push_back(parseAtom());
        }

        while (currentToken.type == TokenType::LPAREN) {
            statements.push_back(parseExpr());
        }

        variants.emplace_back(test, statements);
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    }

    return std::make_shared<CondExpr>(variants);
}

ExprPtr Parser::parseAtom() {
    if (currentToken.type == TokenType::STRING) {
        Token token = currentToken;
        advance();
        return std::make_shared<StringExpr>(token.lexeme);
    }

    if (currentToken.type == TokenType::VAR) {
        Token token = currentToken;
        advance();
        ExprPtr name = std::make_shared<StringExpr>(token.lexeme);
        ExprPtr value = std::make_shared<Uninitialized>();
        return std::make_shared<VarExpr>(name, value);
    }

    if (currentToken.type == TokenType::NIL) {
        advance();
        return std::make_shared<NILExpr>();
    }

    if (currentToken.type == TokenType::T) {
        advance();
        return std::make_shared<TExpr>();
    }

    if (currentToken.type == TokenType::RPAREN) {
        return std::make_shared<Uninitialized>();
    }

    return parseNumber();
}

ExprPtr Parser::parseNumber() {
    const auto token = currentToken;
    advance();

    if (token.type == TokenType::INT) {
        return std::make_shared<IntExpr>(std::stoi(token.lexeme));
    }
    if (token.type == TokenType::DOUBLE) {
        return std::make_shared<DoubleExpr>(std::stof(token.lexeme));
    }

    throw InvalidSyntaxError(fileName, EXPECTED_NUMBER_ERROR, 0);
}

ExprPtr Parser::createVar(const SymbolType type, const bool isConstant) {
    ExprPtr value;
    advance();

    ExprPtr var = parseAtom();

    if (currentToken.type == TokenType::LPAREN) {
        if (isConstant)
            throw InvalidSyntaxError(fileName, ERROR(SEXPR_ERROR, "DEFCONSTANT"), 0);
        value = parseExpr();
    } else {
        value = parseAtom();

        if (isConstant && cast::toUninitialized(value)) {
            throw InvalidSyntaxError(fileName, ERROR(EXPECTED_ELEMS_NUMBER_ERROR, "DEFCONSTANT"), 0);
        }
    }

    cast::toVar(var)->sType = type;
    cast::toVar(var)->value = std::move(value);

    return var;
}

void Parser::consume(const TokenType expected, const char* errorStr) {
    expect(expected, errorStr);
    advance();
}

void Parser::expect(const TokenType expected, const char* errorStr) const {
    if (currentToken.type != expected)
        throw InvalidSyntaxError(fileName, errorStr, 0);
}
