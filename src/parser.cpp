#include "parser.h"
#include "exceptions.hpp"

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
            throw InvalidSyntaxError(mFileName, mCurrentToken.value.c_str(), 0);
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    return expr;
}

ExprPtr Parser::parseSExpr() {
    ExprPtr left, right;

    Token token = mCurrentToken;
    advance();

    if (mCurrentToken.type == TokenType::LPAREN) {
        left = parseExpr();
    } else {
        left = parseAtom();
    }

    if (mCurrentToken.type == TokenType::LPAREN) {
        right = parseExpr();
    } else {
        right = parseAtom();
    }

    if (token.type == TokenType::NOT && !cast::toNIL(right)) {
        throw InvalidSyntaxError(mFileName, ERROR(OP_INVALID_NUMBER_OF_ARGS_ERROR, "NOT", 2), 0);
    }

    return std::make_shared<BinOpExpr>(left, right, token);
}

ExprPtr Parser::parseDotimes() {
    std::vector<ExprPtr> statements;
    ExprPtr var, value;

    advance();

    consume(TokenType::LPAREN, ERROR(EXPECTED_ELEMS_NUMBER_ERROR, "DOTIMES"));
    var = parseAtom();

    if (mCurrentToken.type == TokenType::LPAREN) {
        value = parseExpr();
    } else {
        value = parseAtom();
    }

    cast::toVar(var)->value = std::move(value);
    cast::toVar(var)->sType = SymbolType::LOCAL;

    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);


    while (mCurrentToken.type == TokenType::LPAREN) {
        statements.emplace_back(parseExpr());
    }

    return std::make_shared<DotimesExpr>(var, statements);
}

ExprPtr Parser::parseLoop() {
    std::vector<ExprPtr> sexprs;

    advance();

    while (mCurrentToken.type == TokenType::LPAREN) {
        sexprs.emplace_back(parseExpr());
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
        while (mCurrentToken.type == TokenType::VAR) {
            var = parseAtom();
            cast::toVar(var)->sType = SymbolType::LOCAL;
            bindings.emplace_back(var);
        }

        if (mCurrentToken.type == TokenType::RPAREN)
            break;

        // Check out (let ((x 11)) )
        while (mCurrentToken.type == TokenType::LPAREN) {
            consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
            var = parseAtom();

            if (mCurrentToken.type == TokenType::LPAREN) {
                value = parseExpr();
            } else {
                value = parseAtom();
            }

            cast::toVar(var)->value = std::move(value);
            cast::toVar(var)->sType = SymbolType::LOCAL;
            bindings.emplace_back(var);
            consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
        }

        if (mCurrentToken.type == TokenType::RPAREN)
            break;
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    while (mCurrentToken.type == TokenType::LPAREN) {
        body.emplace_back(parseExpr());
    }

    return std::make_shared<LetExpr>(bindings, body);
}

ExprPtr Parser::parseSetq() {
    ExprPtr var = createVar();
    return std::make_shared<SetqExpr>(var);
}

ExprPtr Parser::parseDefvar() {
    ExprPtr var = createVar();
    return std::make_shared<DefvarExpr>(var);
}

ExprPtr Parser::parseDefconst() {
    ExprPtr var = createVar(true);
    return std::make_shared<DefconstExpr>(var);
}

ExprPtr Parser::parseDefun() {
    ExprPtr name;
    std::vector<ExprPtr> args;
    std::vector<ExprPtr> forms;

    advance();

    name = parseAtom();

    consume(TokenType::LPAREN, MISSING_PAREN_ERROR);
    while (mCurrentToken.type == TokenType::VAR) {
        args.emplace_back(parseAtom());
    }
    consume(TokenType::RPAREN, MISSING_PAREN_ERROR);

    while (mCurrentToken.type == TokenType::LPAREN) {
        forms.emplace_back(parseExpr());
    }

    return std::make_shared<DefunExpr>(name, args, forms);
}

ExprPtr Parser::parseFuncCall() {
    ExprPtr name;
    std::vector<ExprPtr> args;

    name = parseAtom();

    while (mCurrentToken.type == TokenType::INT ||
           mCurrentToken.type == TokenType::DOUBLE ||
           mCurrentToken.type == TokenType::VAR ||
           mCurrentToken.type == TokenType::T ||
           mCurrentToken.type == TokenType::NIL) {
        args.emplace_back(parseAtom());
    }

    return std::make_shared<FuncCallExpr>(name, args);
}

ExprPtr Parser::parseReturn() {
    ExprPtr arg;

    advance();

    arg = parseAtom();

    return std::make_shared<ReturnExpr>(arg);
}

ExprPtr Parser::parseIf() {
    ExprPtr test, then, else_;

    advance();

    if (mCurrentToken.type == TokenType::LPAREN) {
        test = parseExpr();
    } else {
        test = parseAtom();
    }

    if (mCurrentToken.type == TokenType::LPAREN) {
        then = parseExpr();
    } else {
        then = parseAtom();
    }

    if (mCurrentToken.type == TokenType::LPAREN) {
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

    if (mCurrentToken.type == TokenType::LPAREN) {
        test = parseExpr();
    } else {
        test = parseAtom();
    }

    for (;;) {
        if (mCurrentToken.type == TokenType::LPAREN) {
            then.push_back(parseExpr());
        } else {
            then.push_back(parseAtom());
        }

        if (mCurrentToken.type == TokenType::RPAREN)
            break;
    }

    return std::make_shared<WhenExpr>(test, then);
}

ExprPtr Parser::parseCond() {
    ExprPtr test;
    std::vector<std::pair<ExprPtr, std::vector<ExprPtr>>> variants;

    advance();

    while (mCurrentToken.type == TokenType::LPAREN) {
        consume(TokenType::LPAREN, MISSING_PAREN_ERROR);

        if (mCurrentToken.type == TokenType::LPAREN) {
            test = parseExpr();
        } else {
            test = parseAtom();
        }

        std::vector<ExprPtr> statements;
        if (mCurrentToken.type != TokenType::LPAREN) {
            statements.push_back(parseAtom());
        }

        while (mCurrentToken.type == TokenType::LPAREN) {
            statements.push_back(parseExpr());
        }

        variants.emplace_back(test, statements);
        consume(TokenType::RPAREN, MISSING_PAREN_ERROR);
    }

    return std::make_shared<CondExpr>(variants);
}

ExprPtr Parser::parseAtom() {
    if (mCurrentToken.type == TokenType::STRING) {
        Token token = mCurrentToken;
        advance();
        return std::make_shared<StringExpr>(token.value);
    } else if (mCurrentToken.type == TokenType::VAR) {
        Token token = mCurrentToken;
        advance();
        ExprPtr name = std::make_shared<StringExpr>(token.value);
        ExprPtr value = std::make_shared<Uninitialized>();
        return std::make_shared<VarExpr>(name, value);
    } else if (mCurrentToken.type == TokenType::NIL) {
        advance();
        return std::make_shared<NILExpr>();
    } else if (mCurrentToken.type == TokenType::T) {
        advance();
        return std::make_shared<TExpr>();
    } else if (mCurrentToken.type == TokenType::RPAREN) {
        return std::make_shared<Uninitialized>();
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

ExprPtr Parser::createVar(bool isConstant) {
    ExprPtr var, value;
    advance();

    var = parseAtom();

    if (mCurrentToken.type == TokenType::LPAREN) {
        value = parseExpr();
    } else {
        value = parseAtom();

        if (isConstant && cast::toNIL(value)) {
            throw InvalidSyntaxError(mFileName, ERROR(EXPECTED_ELEMS_NUMBER_ERROR, "DEFCONSTANT"), 0);
        }
    }

    cast::toVar(var)->value = std::move(value);
    return var;
}

void Parser::consume(TokenType expected, const char* errorStr) {
    expect(expected, errorStr);
    advance();
}

void Parser::expect(TokenType expected, const char* errorStr) {
    if (mCurrentToken.type != expected)
        throw InvalidSyntaxError(mFileName, errorStr, 0);
}
