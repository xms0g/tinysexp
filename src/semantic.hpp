#pragma once
#include <variant>
#include "parser.hpp"
#include "scope.hpp"

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(std::string_view fn);

    void analyze(const ExprPtr& ast);

private:
    /* Name Resolutions */

    ExprPtr exprResolve(const ExprPtr& ast);

    ExprPtr binopResolve(BinOpExpr& binop);

    ExprPtr dotimesResolve(DotimesExpr& dotimes);

    ExprPtr loopResolve(const LoopExpr& loop);

    ExprPtr letResolve(const LetExpr& let);

    ExprPtr setqResolve(const SetqExpr& setq);

    ExprPtr defvarResolve(const DefvarExpr& defvar);

    ExprPtr defconstResolve(const DefconstExpr& defconst);

    ExprPtr defunResolve(const ExprPtr& defun);

	ExprPtr printResolve(PrintExpr& print);

	ExprPtr readResolve(const ReadExpr& read);

    ExprPtr funcCallResolve(FuncCallExpr& funcCall, bool isParam = false);

    void returnResolve(const ReturnExpr& return_);

    ExprPtr ifResolve(IfExpr& if_);

    ExprPtr whenResolve(WhenExpr& when);

    ExprPtr condResolve(CondExpr& cond);

    void checkConstantVar(const ExprPtr& var);

    void checkBool(const ExprPtr& var, TokenType ttype) const;

    void checkBitwiseOp(const ExprPtr& n, TokenType ttype) const;

    [[nodiscard]]
	std::variant<int, double> getValue(const ExprPtr& num) const;

    ExprPtr returnValue(const VarExpr& var);

    ExprPtr varResolve(ExprPtr& n, TokenType ttype);

    ExprPtr nodeResolve(ExprPtr& n, TokenType ttype);

    ExprPtr valueResolve(const ExprPtr& var, bool isConstant = false);

    static bool isPrimitive(const ExprPtr& var);

    void setType(VarExpr& var, const ExprPtr& value);

    ScopeTracker mSymbolTracker;

    struct TypeInferenceContext {
        bool isStarted{false};
        std::string_view entryPoint;
    };

    TypeInferenceContext mTfCtx;
    std::string_view mFileName;
};
