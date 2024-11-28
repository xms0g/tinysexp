#pragma once

#include <format>
#include <utility>
#include <memory>
#include "lexer.h"

struct NumberExpr;
struct StringExpr;
struct BinOpExpr;
struct DotimesExpr;
struct PrintExpr;
struct ReadExpr;
struct LetExpr;
struct SetqExpr;
struct VarExpr;

enum class ExprType {
    BINOP,
    DOTIMES,
    PRINT,
    READ,
    LET,
    SETQ,
    VAR,
    INT,
    STR
};

struct IExpr {
    std::shared_ptr<IExpr> child;

    virtual ~IExpr() = default;

    virtual ExprType type() = 0;
    virtual NumberExpr& asNum() {}
    virtual StringExpr& asStr() {}
    virtual BinOpExpr& asBinop() {}
    virtual DotimesExpr& asDotimes() {}
    virtual PrintExpr& asPrint() {}
    virtual ReadExpr& asRead() {}
    virtual LetExpr& asLet() {}
    virtual SetqExpr& asSetq() {}
    virtual VarExpr& asVar() {}
};

using ExprPtr = std::shared_ptr<IExpr>;

struct NumberExpr : IExpr {
    uint8_t n;

    explicit NumberExpr(uint8_t n) : n(n) {}

    ExprType type() override {
        return ExprType::INT;
    }

    NumberExpr& asNum() override {
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const NumberExpr& nn) {
        os << std::format("{}", nn.n);
        return os;
    }
};

struct StringExpr : IExpr {
    std::string str;

    StringExpr() = default;

    explicit StringExpr(std::string& str) : str(str) {}

    ExprType type() override {
        return ExprType::STR;
    }

    StringExpr& asStr() override {
        return *this;
    }
};

struct NILExpr : IExpr {
    NILExpr() = default;
    ExprType type() override {}
};

struct BinOpExpr : IExpr {
    ExprPtr lhs;
    ExprPtr rhs;
    Token opToken;

    BinOpExpr(ExprPtr& ln, ExprPtr& rn, Token opTok) :
            lhs(std::move(ln)),
            rhs(std::move(rn)),
            opToken(std::move(opTok)) {}

    ExprType type() override {
        return ExprType::BINOP;
    }

    BinOpExpr& asBinop() override {
        return *this;
    }
};

struct DotimesExpr : IExpr {
    ExprPtr iterationCount;
    std::vector<ExprPtr> statements;

    DotimesExpr(ExprPtr& iterCount, std::vector<ExprPtr>& statements) :
            iterationCount(std::move(iterCount)),
            statements(std::move(statements)) {}

    ExprType type() override {
        return ExprType::DOTIMES;
    }

    DotimesExpr& asDotimes() override {
        return *this;
    }
};

struct PrintExpr : IExpr {
    ExprPtr sexpr;

    explicit PrintExpr(ExprPtr& expr) : sexpr(std::move(expr)) {}

    ExprType type() override {
        return ExprType::PRINT;
    }

    PrintExpr& asPrint() override {
        return *this;
    }
};

struct ReadExpr : IExpr {
    ReadExpr() = default;

    ExprType type() override {
        return ExprType::READ;
    }

    ReadExpr& asRead() override {
        return *this;
    }
};

struct LetExpr : IExpr {
    std::vector<ExprPtr> sexprs;
    std::vector<ExprPtr> variables;

    LetExpr(std::vector<ExprPtr>& sexprs, std::vector<ExprPtr>& variables) :
            sexprs(std::move(sexprs)),
            variables(std::move(variables)) {}

    ExprType type() override {
        return ExprType::LET;
    }

    LetExpr& asLet() override {
        return *this;
    }
};

struct SetqExpr : IExpr {
    ExprPtr var;

    SetqExpr(ExprPtr& var) :
            var(std::move(var)) {}

    ExprType type() override {
        return ExprType::SETQ;
    }

    SetqExpr& asSetq() override {
        return *this;
    }
};

struct VarExpr : IExpr {
    ExprPtr name;
    ExprPtr value;

    explicit VarExpr(ExprPtr& name, ExprPtr& value) : name(std::move(name)), value(std::move(value)) {}

    ExprType type() override {
        return ExprType::VAR;
    }

    VarExpr& asVar() override {
        return *this;
    }
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

    ExprPtr parseRead();

    ExprPtr parseDotimes();

    ExprPtr parseLet();

    ExprPtr parseSetq();

    ExprPtr parseAtom();

    ExprPtr parseNumber();

    void consume(TokenType expected, const char* errorStr);

    ExprPtr checkVarError(ExprPtr& expr);

    Lexer& mLexer;
    Token mCurrentToken{};
    int mTokenIndex;
    const char* mFileName;
    std::unordered_map<std::string, ExprPtr> symbolTable;
};
