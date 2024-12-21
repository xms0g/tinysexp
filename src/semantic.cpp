#include "semantic.h"
#include "exceptions.hpp"

SemanticAnalyzer::SemanticAnalyzer(const char* fn) : mFileName(fn) {}

void SemanticAnalyzer::analyze(ExprPtr& ast) {
    auto next = ast;

    scopeEnter();
    while (next != nullptr) {
        exprResolve(next);
        next = next->child;
    }
    scopeExit();
}

void SemanticAnalyzer::exprResolve(const ExprPtr& ast) {
    if (auto binop = cast::toBinop(ast)) {
        binopResolve(*binop);
    } else if (auto dotimes = cast::toDotimes(ast)) {
        dotimesResolve(*dotimes);
    } else if (auto loop = cast::toLoop(ast)) {
        loopResolve(*loop);
    } else if (auto let = cast::toLet(ast)) {
        letResolve(*let);
    } else if (auto setq = cast::toSetq(ast)) {
        setqResolve(*setq);
    } else if (auto defvar = cast::toDefvar(ast)) {
        defvarResolve(*defvar);
    } else if (auto defconst = cast::toDefconstant(ast)) {
        defconstResolve(*defconst);
    } else if (auto defun = cast::toDefun(ast)) {
        defunResolve(*defun);
    } else if (auto funcCall = cast::toFuncCall(ast)) {
        funcCallResolve(*funcCall);
    } else if (auto if_ = cast::toIf(ast)) {
        ifResolve(*if_);
    } else if (auto when = cast::toWhen(ast)) {
        whenResolve(*when);
    } else if (auto cond = cast::toCond(ast)) {
        condResolve(*cond);
    }
}

void SemanticAnalyzer::binopResolve(BinOpExpr& binop) {
    varResolve(binop.lhs);
    varResolve(binop.rhs);
}

void SemanticAnalyzer::varResolve(ExprPtr& var) {
    if (cast::toT(var)) {
        throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "t"), 0);
    }

    if (cast::toNIL(var)) {
        throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "nil"), 0);
    }

    if (!cast::toInt(var) && !cast::toDouble(var)) {
        const auto name = cast::toString(var)->data;
        Symbol sym = scopeLookup(name);

        if (!sym.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
        }

        var = sym.value;
    }
}

void SemanticAnalyzer::dotimesResolve(const DotimesExpr& dotimes) {
    scopeEnter();
    checkConstantVar(dotimes.iterationCount);
    scopeExit();
}

void SemanticAnalyzer::loopResolve(const LoopExpr& loop) {
    for (auto& sexpr: loop.sexprs) {
        exprResolve(sexpr);
    }
}

void SemanticAnalyzer::letResolve(const LetExpr& let) {
    scopeEnter();
    for (auto& var: let.bindings) {
        const auto var_ = cast::toVar(var);
        const auto varName = cast::toString(var_->name)->data;

        // Check out the var in the current scope, if it's already defined, raise error
        Symbol sym = scopeLookupCurrent(varName);
        if (sym.value) {
            throw SemanticError(mFileName, ERROR(MULTIPLE_DECL_ERROR, varName), 0);
        }

        // Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
        // If it's expr, resolve it.
        if (const auto value = cast::toString(var_->value)) {
            sym = scopeLookup(value->data);

            if (!sym.value) {
                throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, value->data), 0);
            }
        } else {
            bool isExpr = (!cast::toInt(var_->value) && !cast::toDouble(var_->value));
            if (isExpr) {
                exprResolve(var_->value);
            }
        }

        scopeBind(varName, {varName, var, SymbolType::LOCAL});
    }

    for (auto& statement: let.body) {
        exprResolve(statement);
    }
    scopeExit();
}

void SemanticAnalyzer::setqResolve(const SetqExpr& setq) {
    const auto var_ = cast::toVar(setq.pair);
    const auto name = cast::toString(var_->name)->data;

    // Check out the var.If it's not defined, raise error.
    Symbol sym = scopeLookup(name);

    if (!sym.value) {
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
    }

    checkConstantVar(setq.pair);

    // Check the value of var.If it's another var, look up all scopes.If it's not defined, raise error.
    // If it's expr, resolve it.
    if (const auto value = cast::toString(var_->value)) {
        sym = scopeLookup(value->data);

        if (!sym.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, value->data), 0);
        }
    } else {
        bool isExpr = (!cast::toInt(var_->value) && !cast::toDouble(var_->value));
        if (isExpr) {
            exprResolve(var_->value);
        }
    }
}

void SemanticAnalyzer::defvarResolve(const DefvarExpr& defvar) {
    const auto var = cast::toVar(defvar.pair);
    const auto varName = cast::toString(var->name)->data;

    if (scopeLevel() > 1) {
        throw SemanticError(mFileName, ERROR(GLOBAL_VAR_DECL_ERROR, varName), 0);
    }

    scopeBind(varName, {varName, defvar.pair, SymbolType::GLOBAL});
}

void SemanticAnalyzer::defconstResolve(const DefconstExpr& defconst) {
    const auto var = cast::toVar(defconst.pair);
    const auto varName = cast::toString(var->name)->data;

    if (scopeLevel() > 1) {
        throw SemanticError(mFileName, ERROR(CONSTANT_VAR_DECL_ERROR, varName), 0);
    }

    scopeBind(varName, {varName, defconst.pair, SymbolType::GLOBAL, true});
}

void SemanticAnalyzer::defunResolve(const DefunExpr& defun) {
    const auto funcName = cast::toString(defun.name)->data;

    if (scopeLevel() > 1) {
        throw SemanticError(mFileName, ERROR(FUNC_DEF_ERROR, funcName), 0);
    }

    ExprPtr func = std::make_shared<DefunExpr>(defun);

    scopeBind(funcName, {funcName, func, SymbolType::GLOBAL});

    scopeEnter();
    for (auto& arg: defun.args) {
        const auto sarg = cast::toString(arg)->data;
        scopeBind(sarg, {sarg, arg, SymbolType::LOCAL});
    }

    for (auto& statement: defun.forms) {
        exprResolve(statement);
    }
    scopeExit();
}

void SemanticAnalyzer::funcCallResolve(const FuncCallExpr& funcCall) {
    const auto funcName = cast::toString(funcCall.name)->data;

    Symbol sym = scopeLookup(funcName);

    if (!sym.value) {
        throw SemanticError(mFileName, ERROR(FUNC_UNDEFINED_ERROR, funcName), 0);
    }

    const auto func = cast::toDefun(sym.value);
    if (funcCall.args.size() != func->args.size()) {
        throw SemanticError(mFileName, ERROR(FUNC_INVALID_NUMBER_OF_ARGS_ERROR, funcName), 0);
    }
}

void SemanticAnalyzer::ifResolve(const IfExpr& if_) {
    if (const auto test = cast::toString(if_.test)) {
        Symbol sym = scopeLookup(test->data);

        if (!sym.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, test->data), 0);
        }
    } else {
        exprResolve(if_.test);
    }

    exprResolve(if_.then);

    if (if_.else_) {
        exprResolve(if_.else_);
    }
}

void SemanticAnalyzer::whenResolve(const WhenExpr& when) {
    ifResolve(when);
}

void SemanticAnalyzer::condResolve(const CondExpr& cond) {
    for (auto& variant: cond.variants) {
        if (auto test = cast::toString(variant.first)) {
            Symbol sym = scopeLookup(test->data);

            if (!sym.value) {
                throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, test->data), 0);
            }
        } else {
            exprResolve(variant.first);
        }

        for (auto& statement: variant.second) {
            exprResolve(statement);
        }
    }
}

void SemanticAnalyzer::checkConstantVar(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);
    const auto varName = cast::toString(var_->name)->data;

    Symbol sym = scopeLookup(varName);

    if (sym.isConstant) {
        throw SemanticError(mFileName, ERROR(CONSTANT_VAR_ERROR, varName), 0);
    }
}

void SemanticAnalyzer::scopeEnter() {
    std::unordered_map<std::string, Symbol> scope;
    mSymbolTable.push(scope);
}

void SemanticAnalyzer::scopeExit() {
    mSymbolTable.pop();
}

size_t SemanticAnalyzer::scopeLevel() {
    return mSymbolTable.size();
}

void SemanticAnalyzer::scopeBind(const std::string& name, const Symbol& symbol) {
    auto scope = mSymbolTable.top();
    mSymbolTable.pop();
    scope.emplace(name, symbol);
    mSymbolTable.push(scope);
}

Symbol SemanticAnalyzer::scopeLookup(const std::string& name) {
    Symbol sym;
    std::stack<ScopeType> scopes;
    size_t level = mSymbolTable.size();

    for (int i = 0; i < level; ++i) {
        ScopeType scope = mSymbolTable.top();
        mSymbolTable.pop();
        scopes.push(scope);

        if (auto elem = scope.find(name);elem != scope.end()) {
            sym = elem->second;
            break;
        }
    }

    // reconstruct the scopes
    size_t currentLevel = scopes.size();
    for (int i = 0; i < currentLevel; ++i) {
        ScopeType scope = scopes.top();
        scopes.pop();
        mSymbolTable.push(scope);
    }

    return sym;
}

Symbol SemanticAnalyzer::scopeLookupCurrent(const std::string& name) {
    ScopeType currentScope = mSymbolTable.top();

    if (auto elem = currentScope.find(name);elem != currentScope.end()) {
        return elem->second;
    }

    return {};
}