#pragma once
#include <stack>
#include <unordered_map>
#include <variant>
#include "parser.hpp"

struct Symbol {
    std::string name;
    ExprPtr value;
    SymbolType sType;
    bool isConstant{};
};

class ScopeTracker {
public:
    void enter(std::string_view scopeName);

    void exit(bool isFunc = false);

    [[nodiscard]]
	std::string_view scopeName();

    [[nodiscard]]
	size_t level() const;

    void bind(std::string_view name, const Symbol& symbol);

    void update(std::string_view name, Symbol symbol);

    Symbol lookup(std::string_view name);

    Symbol lookupCurrent(std::string_view name);

private:
	struct StringHash {
		using is_transparent = void;

		static constexpr size_t operator()(const std::string_view value) noexcept {
			return std::hash<std::string_view>{}(value);
		}

	};

	struct StringEqual {
		using is_transparent = void;

		static constexpr bool operator()(const std::string_view lhs, const std::string_view rhs) noexcept {
			return lhs == rhs;
		}
	};

    using ScopeType = std::unordered_map<std::string, Symbol, StringHash, StringEqual>;
    std::stack<ScopeType> mSymbolTable;
    std::stack<std::string> mScopeNames;
};

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(std::string_view fn);

    void analyze(const ExprPtr& ast);

private:
    /* Name Resolutions */

    ExprPtr exprResolve(const ExprPtr& ast);

    ExprPtr binopResolve(BinOpExpr& binop);

    ExprPtr dotimesResolve(const DotimesExpr& dotimes);

    ExprPtr loopResolve(const LoopExpr& loop);

    ExprPtr letResolve(const LetExpr& let);

    ExprPtr setqResolve(const SetqExpr& setq);

    void defvarResolve(const DefvarExpr& defvar);

    void defconstResolve(const DefconstExpr& defconst);

    ExprPtr defunResolve(const ExprPtr& defun);

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
