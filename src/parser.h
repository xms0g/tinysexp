#pragma once

#include <format>
#include <utility>
#include <memory>
#include "lexer.h"

class NumberExpr;
class BinOpExpr;
class DotimesExpr;
class PrintExpr;
class LetExpr;
class VarExpr;

struct IExpr {
    virtual ~IExpr() = default;

    virtual TokenType type() = 0;

    virtual NumberExpr* asNumber() { return nullptr; }

    virtual BinOpExpr* asBinOp() { return nullptr; }

    virtual DotimesExpr* asDotimes() { return nullptr; }

    virtual PrintExpr* asPrint() { return nullptr; }

    virtual LetExpr* asLet() { return nullptr; }

    virtual VarExpr* asVar() { return nullptr; }
};

using ExprPtr = std::unique_ptr<IExpr>;

struct NumberExpr : public IExpr {
    int n;

    explicit NumberExpr(int n) : n(n) {}

    TokenType type() override { return TokenType::INT; }

    NumberExpr* asNumber() override { return this; }

    friend std::ostream& operator<<(std::ostream& os, const NumberExpr& nn) {
        os << std::format("{}", nn.n);
        return os;
    }
};

struct BinOpExpr : public IExpr {
    ExprPtr lhs;
    ExprPtr rhs;
    Token opToken;

    BinOpExpr(ExprPtr& ln, ExprPtr& rn, Token opTok) :
            lhs(std::move(ln)),
            rhs(std::move(rn)),
            opToken(std::move(opTok)) {}

    TokenType type() override { return opToken.type; }

    BinOpExpr* asBinOp() override { return this; }
};

struct DotimesExpr : public IExpr {
    ExprPtr iterationCount;
    ExprPtr statement;

    DotimesExpr(ExprPtr& iterCount, ExprPtr& statement) :
            iterationCount(std::move(iterCount)),
            statement(std::move(statement)) {}

    TokenType type() override { return TokenType::DOTIMES; }

    DotimesExpr* asDotimes() override { return this; }
};

struct PrintExpr : public IExpr {
    ExprPtr sexpr;

    explicit PrintExpr(ExprPtr& expr) : sexpr(std::move(expr)) {}

    TokenType type() override { return TokenType::PRINT; }

    PrintExpr* asPrint() override { return this; }
};

struct LetExpr : public IExpr {
    ExprPtr sexpr;
    std::vector<ExprPtr> variables;

    LetExpr(ExprPtr& expr, std::vector<ExprPtr>& variables) :
            sexpr(std::move(expr)),
            variables(std::move(variables)) {}

    TokenType type() override { return TokenType::LET; }

    LetExpr* asLet() override { return this; }
};

struct VarExpr : public IExpr {
    std::string name;
    ExprPtr value;

    explicit VarExpr(std::string& name) : name(std::move(name)) {}

    TokenType type() override { return TokenType::VAR; }

    VarExpr* asVar() override { return this; }
};

class Parser {
public:
    Parser(const char* mFileName, Lexer& lexer);

    ExprPtr parse();

private:
    Token advance();

    ExprPtr parseExpr();

    ExprPtr parseSExpr();

    ExprPtr parsePrint();

    ExprPtr parseDotimes();

    ExprPtr parseLet();

    ExprPtr parseVar();

    ExprPtr parseAtom();

    ExprPtr parseNumber();

    void consume(TokenType expected, const char* errorStr);

    Lexer& mLexer;
    Token mCurrentToken{};
    int mTokenIndex;
    const char* mFileName;
};
