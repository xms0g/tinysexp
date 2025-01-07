#include "semantic.h"
#include "exceptions.hpp"

void ScopeTracker::enter() {
    std::unordered_map<std::string, Symbol> scope;
    mSymbolTable.push(scope);
}

void ScopeTracker::exit() {
    mSymbolTable.pop();
}

size_t ScopeTracker::level() {
    return mSymbolTable.size();
}

void ScopeTracker::bind(const std::string& name, const Symbol& symbol) {
    auto scope = mSymbolTable.top();
    mSymbolTable.pop();

    if (scope.contains(name)) {
        scope[name] = symbol;
    } else {
        scope.emplace(name, symbol);
    }

    mSymbolTable.push(scope);
}

Symbol ScopeTracker::lookup(const std::string& name) {
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

Symbol ScopeTracker::lookupCurrent(const std::string& name) {
    ScopeType currentScope = mSymbolTable.top();

    if (auto elem = currentScope.find(name);elem != currentScope.end()) {
        return elem->second;
    }

    return {};
}

SemanticAnalyzer::SemanticAnalyzer(const char* fn) : mFileName(fn) {}

void SemanticAnalyzer::analyze(ExprPtr& ast) {
    auto next = ast;

    stracker.enter();
    while (next != nullptr) {
        exprResolve(next);
        next = next->child;
    }
    stracker.exit();
}

ExprPtr SemanticAnalyzer::exprResolve(const ExprPtr& ast) {
    if (auto binop = cast::toBinop(ast)) {
        return binopResolve(*binop);
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
    return nullptr;
}

ExprPtr SemanticAnalyzer::binopResolve(BinOpExpr& binop) {
    ExprPtr lhs = varResolve(binop.lhs);
    ExprPtr rhs = varResolve(binop.rhs);

    // If one of both is double, return it.Otherwise, result is int
    if (auto var = cast::toVar(lhs); cast::toDouble(var->value)) {
        return lhs;
    }

    if (auto var = cast::toVar(rhs); cast::toDouble(var->value)) {
        return rhs;
    }

    return lhs;
}

ExprPtr SemanticAnalyzer::varResolve(ExprPtr& var) {
    checkNotNumber(var);

    if (cast::toInt(var) || cast::toDouble(var)) return nullptr;

    if (auto binop = cast::toBinop(var)) {
        return binopResolve(*binop);
    } else {
        const auto var_ = cast::toVar(var);
        const std::string name = cast::toString(var_->name)->data;

        Symbol sym = stracker.lookup(name);

        if (!sym.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
        }

        auto innerVar = cast::toVar(sym.value);
        do {
            checkNotNumber(innerVar);
            // Check out if the sym value is int,double or var. Update var.
            if (cast::toInt(innerVar->value) || cast::toDouble(innerVar->value)) {
                ExprPtr name_ = cast::toString(var_->name);
                ExprPtr value_ = innerVar->value;
                var = std::make_shared<VarExpr>(name_, value_, sym.sType);
                return var;
            } else {
                innerVar = cast::toVar(innerVar->value);
            }
        } while (cast::toVar(innerVar));
    }
}

void SemanticAnalyzer::dotimesResolve(const DotimesExpr& dotimes) {
    stracker.enter();
    checkConstantVar(dotimes.iterationCount);
    stracker.exit();
}

void SemanticAnalyzer::loopResolve(const LoopExpr& loop) {
    for (auto& sexpr: loop.sexprs) {
        exprResolve(sexpr);
    }
}

void SemanticAnalyzer::letResolve(const LetExpr& let) {
    stracker.enter();
    for (auto& var: let.bindings) {
        const auto var_ = cast::toVar(var);
        const std::string varName = cast::toString(var_->name)->data;

        // Check out the var in the current scope, if it's already defined, raise error
        Symbol sym = stracker.lookupCurrent(varName);
        if (sym.value) {
            throw SemanticError(mFileName, ERROR(MULTIPLE_DECL_ERROR, varName), 0);
        }

        // Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
        // If it's expr, resolve it.
        if (const auto value = cast::toVar(var_->value)) {
            const std::string valueName = cast::toString(value->name)->data;
            Symbol sym_ = stracker.lookup(valueName);

            if (!sym_.value) {
                throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, valueName), 0);
            }
            // Update
            var_->value = sym_.value;
            stracker.bind(varName, {varName, var, SymbolType::LOCAL});
        } else if (cast::toInt(var_->value) || cast::toDouble(var_->value)) {
            ExprPtr name_ = var_->name;
            ExprPtr value_ = var_->value;
            ExprPtr new_var = std::make_shared<VarExpr>(name_, value_, SymbolType::LOCAL);
            stracker.bind(varName, {varName, new_var, SymbolType::LOCAL});
        } else {
            ExprPtr name_ = var_->name;
            ExprPtr value_ = exprResolve(var_->value);
            ExprPtr new_var = std::make_shared<VarExpr>(name_, value_, SymbolType::LOCAL);
            stracker.bind(varName, {varName, new_var, SymbolType::LOCAL});
        }
    }

    for (auto& statement: let.body) {
        exprResolve(statement);
    }
    stracker.exit();
}

void SemanticAnalyzer::setqResolve(const SetqExpr& setq) {
    checkConstantVar(setq.pair);

    const auto var = cast::toVar(setq.pair);
    const std::string varName = cast::toString(var->name)->data;

    // Check out the var.If it's not defined, raise error.
    Symbol sym = stracker.lookup(varName);

    if (!sym.value) {
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, varName), 0);
    }
    // Resolve the var scope.
    var->sType = sym.sType;
    // Check out the value of var.If it's another var, look up all scopes.If it's not defined, raise error.
    // If it's int or double, update sym->value and bind again.
    // If it's expr, resolve it.
    if (const auto value = cast::toVar(var->value)) {
        const std::string valueName = cast::toString(value->name)->data;

        Symbol sym_ = stracker.lookup(valueName);

        if (!sym_.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, valueName), 0);
        }
        var->value = sym_.value;

        stracker.bind(varName, {varName, var, var->sType});
    } else if (cast::toInt(var->value) || cast::toDouble(var->value)) {
        ExprPtr name_ = var->name;
        ExprPtr value_ = var->value;
        sym.value = std::make_shared<VarExpr>(name_, value_, sym.sType);
        stracker.bind(varName, sym);
    } else {
        ExprPtr name_ = var->name;
        ExprPtr value_ = exprResolve(var->value);
        sym.value = std::make_shared<VarExpr>(name_, value_, sym.sType);
        stracker.bind(varName, sym);
    }
}

void SemanticAnalyzer::defvarResolve(const DefvarExpr& defvar) {
    const auto var = cast::toVar(defvar.pair);
    const std::string varName = cast::toString(var->name)->data;

    if (stracker.level() > 1) {
        throw SemanticError(mFileName, ERROR(GLOBAL_VAR_DECL_ERROR, varName), 0);
    }

    stracker.bind(varName, {varName, defvar.pair, SymbolType::GLOBAL});
}

void SemanticAnalyzer::defconstResolve(const DefconstExpr& defconst) {
    const auto var = cast::toVar(defconst.pair);
    const std::string varName = cast::toString(var->name)->data;

    if (stracker.level() > 1) {
        throw SemanticError(mFileName, ERROR(CONSTANT_VAR_DECL_ERROR, varName), 0);
    }

    stracker.bind(varName, {varName, defconst.pair, SymbolType::GLOBAL, true});
}

void SemanticAnalyzer::defunResolve(const DefunExpr& defun) {
    const std::string funcName = cast::toString(defun.name)->data;

    if (stracker.level() > 1) {
        throw SemanticError(mFileName, ERROR(FUNC_DEF_ERROR, funcName), 0);
    }

    ExprPtr func = std::make_shared<DefunExpr>(defun);

    stracker.bind(funcName, {funcName, func, SymbolType::GLOBAL});

    stracker.enter();
    for (auto& arg: defun.args) {
        const auto sarg = cast::toString(arg)->data;
        stracker.bind(sarg, {sarg, arg, SymbolType::PARAM});
    }

    for (auto& statement: defun.forms) {
        exprResolve(statement);
    }
    stracker.exit();
}

void SemanticAnalyzer::funcCallResolve(const FuncCallExpr& funcCall) {
    const std::string funcName = cast::toString(funcCall.name)->data;

    Symbol sym = stracker.lookup(funcName);

    if (!sym.value) {
        throw SemanticError(mFileName, ERROR(FUNC_UNDEFINED_ERROR, funcName), 0);
    }

    const auto func = cast::toDefun(sym.value);
    if (funcCall.args.size() != func->args.size()) {
        throw SemanticError(mFileName, ERROR(FUNC_INVALID_NUMBER_OF_ARGS_ERROR, funcName, funcCall.args.size()), 0);
    }
}

void SemanticAnalyzer::ifResolve(const IfExpr& if_) {
    if (const auto test = cast::toString(if_.test)) {
        Symbol sym = stracker.lookup(test->data);

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
            Symbol sym = stracker.lookup(test->data);

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
    const std::string varName = cast::toString(var_->name)->data;

    Symbol sym = stracker.lookup(varName);

    if (sym.isConstant) {
        throw SemanticError(mFileName, ERROR(CONSTANT_VAR_ERROR, varName), 0);
    }
}

void SemanticAnalyzer::checkNotNumber(const ExprPtr& var) {
    if (cast::toT(var)) {
        throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "t"), 0);
    }

    if (cast::toNIL(var)) {
        throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "nil"), 0);
    }
}