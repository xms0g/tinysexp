#include "semantic.h"
#include "exceptions.hpp"

void ScopeTracker::enter() {
    std::unordered_map<std::string, Symbol> scope;
    mSymbolTable.push(scope);
}

void ScopeTracker::exit() {
    mSymbolTable.pop();
}

size_t ScopeTracker::level() const {
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
    const size_t level = mSymbolTable.size();

    for (int i = 0; i < level; ++i) {
        ScopeType scope = mSymbolTable.top();
        mSymbolTable.pop();
        scopes.push(scope);

        if (auto elem = scope.find(name); elem != scope.end()) {
            sym = elem->second;
            break;
        }
    }

    // reconstruct the scopes
    const size_t currentLevel = scopes.size();
    for (int i = 0; i < currentLevel; ++i) {
        ScopeType scope = scopes.top();
        scopes.pop();
        mSymbolTable.push(scope);
    }

    return sym;
}

Symbol ScopeTracker::lookupCurrent(const std::string& name) {
    ScopeType currentScope = mSymbolTable.top();

    if (const auto elem = currentScope.find(name); elem != currentScope.end()) {
        return elem->second;
    }

    return {};
}

SemanticAnalyzer::SemanticAnalyzer(const char* fn) : mFileName(fn) {
}

void SemanticAnalyzer::analyze(ExprPtr& ast) {
    auto next = ast;

    symbolTracker.enter();
    while (next != nullptr) {
        exprResolve(next);
        next = next->child;
    }
    symbolTracker.exit();
}

ExprPtr SemanticAnalyzer::exprResolve(const ExprPtr& ast) {
    if (auto binop = cast::toBinop(ast)) {
        return binopResolve(*binop);
    }
    if (auto dotimes = cast::toDotimes(ast)) {
        dotimesResolve(*dotimes);
    } else if (const auto loop = cast::toLoop(ast)) {
        loopResolve(*loop);
    } else if (const auto let = cast::toLet(ast)) {
        letResolve(*let);
    } else if (const auto setq = cast::toSetq(ast)) {
        setqResolve(*setq);
    } else if (const auto defvar = cast::toDefvar(ast)) {
        defvarResolve(*defvar);
    } else if (const auto defconst = cast::toDefconstant(ast)) {
        defconstResolve(*defconst);
    } else if (const auto defun = cast::toDefun(ast)) {
        defunResolve(*defun);
    } else if (const auto funcCall = cast::toFuncCall(ast)) {
        funcCallResolve(*funcCall);
    } else if (const auto return_ = cast::toReturn(ast)) {
        returnResolve(*return_);
    } else if (const auto if_ = cast::toIf(ast)) {
        ifResolve(*if_);
    } else if (const auto when = cast::toWhen(ast)) {
        whenResolve(*when);
    } else if (const auto cond = cast::toCond(ast)) {
        condResolve(*cond);
    }
    return nullptr;
}

ExprPtr SemanticAnalyzer::binopResolve(BinOpExpr& binop) {
    ExprPtr lhs = numberResolve(binop.lhs, binop.opToken.type);
    ExprPtr rhs = numberResolve(binop.rhs, binop.opToken.type);

    if (checkDouble(lhs)) {
        return lhs;
    }

    if (checkDouble(rhs)) {
        return rhs;
    }

    return lhs;
}

void SemanticAnalyzer::dotimesResolve(const DotimesExpr& dotimes) {
    symbolTracker.enter();
    checkConstantVar(dotimes.iterationCount);

    const auto var = cast::toVar(dotimes.iterationCount);
    // Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
    // If it's expr, resolve it.
    valueResolve(var);

    for (auto& statement: dotimes.statements) {
        exprResolve(statement);
    }
    symbolTracker.exit();
}

void SemanticAnalyzer::loopResolve(const LoopExpr& loop) {
    for (auto& sexpr: loop.sexprs) {
        exprResolve(sexpr);
    }
}

void SemanticAnalyzer::letResolve(const LetExpr& let) {
    symbolTracker.enter();
    for (auto& var: let.bindings) {
        const auto var_ = cast::toVar(var);
        const std::string varName = cast::toString(var_->name)->data;

        // Check out the var in the current scope, if it's already defined, raise error
        if (const Symbol sym = symbolTracker.lookupCurrent(varName); sym.value) {
            throw SemanticError(mFileName, ERROR(MULTIPLE_DECL_ERROR, varName), 0);
        }

        // Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
        // If it's expr, resolve it.
        valueResolve(var_);
    }

    for (auto& statement: let.body) {
        exprResolve(statement);
    }
    symbolTracker.exit();
}

void SemanticAnalyzer::setqResolve(const SetqExpr& setq) {
    checkConstantVar(setq.pair);

    const auto var = cast::toVar(setq.pair);
    const std::string varName = cast::toString(var->name)->data;

    // Check out the var.If it's not defined, raise error.
    const Symbol sym = symbolTracker.lookup(varName);

    if (!sym.value) {
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, varName), 0);
    }
    // Resolve the var scope.
    var->sType = sym.sType;
    // Check out the value of var.If it's another var, look up all scopes.If it's not defined, raise error.
    // If it's int or double, update sym->value and bind again.
    // If it's expr, resolve it.
    valueResolve(var);
}

void SemanticAnalyzer::defvarResolve(const DefvarExpr& defvar) {
    const auto var = cast::toVar(defvar.pair);
    const std::string varName = cast::toString(var->name)->data;

    if (symbolTracker.level() > 1) {
        throw SemanticError(mFileName, ERROR(GLOBAL_VAR_DECL_ERROR, varName), 0);
    }

    valueResolve(var);
}

void SemanticAnalyzer::defconstResolve(const DefconstExpr& defconst) {
    const auto var = cast::toVar(defconst.pair);
    const std::string varName = cast::toString(var->name)->data;

    if (symbolTracker.level() > 1) {
        throw SemanticError(mFileName, ERROR(CONSTANT_VAR_DECL_ERROR, varName), 0);
    }

    valueResolve(var, true);
}

void SemanticAnalyzer::defunResolve(const DefunExpr& defun) {
    const auto var = cast::toVar(defun.name);
    const std::string funcName = cast::toString(var->name)->data;

    if (symbolTracker.level() > 1) {
        throw SemanticError(mFileName, ERROR(FUNC_DEF_ERROR, funcName), 0);
    }

    const ExprPtr func = std::make_shared<DefunExpr>(defun);

    symbolTracker.bind(funcName, {funcName, func, SymbolType::GLOBAL});

    symbolTracker.enter();
    for (auto& arg: defun.args) {
        const auto argVar = cast::toVar(arg);
        const std::string argName = cast::toString(argVar->name)->data;
        symbolTracker.bind(argName, {argName, arg, argVar->sType});
    }

    for (auto& statement: defun.forms) {
        exprResolve(statement);
    }
    symbolTracker.exit();
}

void SemanticAnalyzer::funcCallResolve(const FuncCallExpr& funcCall) {
    const std::string funcName = cast::toString(funcCall.name)->data;

    const Symbol sym = symbolTracker.lookup(funcName);

    if (!sym.value) {
        throw SemanticError(mFileName, ERROR(FUNC_UNDEFINED_ERROR, funcName), 0);
    }

    if (const auto func = cast::toDefun(sym.value); funcCall.args.size() != func->args.size()) {
        throw SemanticError(mFileName, ERROR(FUNC_INVALID_NUMBER_OF_ARGS_ERROR, funcName, funcCall.args.size()), 0);
    }
    //TODO: resolve expression param type
}

void SemanticAnalyzer::returnResolve(const ReturnExpr& return_) {
    if (cast::toT(return_.arg) || cast::toNIL(return_.arg)) return;

    const auto arg = cast::toVar(return_.arg);
    const std::string argName = cast::toString(arg->name)->data;

    // Check out the var.If it's not defined, raise error.
    if (const Symbol sym = symbolTracker.lookup(argName); !sym.value) {
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, argName), 0);
    }
}

void SemanticAnalyzer::ifResolve(const IfExpr& if_) {
    if (const auto test = cast::toVar(if_.test)) {
        const std::string name = cast::toString(test->name)->data;

        if (const Symbol sym = symbolTracker.lookup(name); !sym.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
        }
    } else {
        exprResolve(if_.test);
    }

    exprResolve(if_.then);

    if (!cast::toUninitialized(if_.else_)) {
        exprResolve(if_.else_);
    }
}

void SemanticAnalyzer::whenResolve(const WhenExpr& when) {
    if (const auto test = cast::toVar(when.test)) {
        const std::string name = cast::toString(test->name)->data;

        if (const Symbol sym = symbolTracker.lookup(name); !sym.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
        }
    } else {
        exprResolve(when.test);
    }

    for (auto& form: when.then) {
        exprResolve(form);
    }
}

void SemanticAnalyzer::condResolve(const CondExpr& cond) {
    for (const auto& [test, statements]: cond.variants) {
        if (const auto test_ = cast::toVar(test)) {
            const std::string name = cast::toString(test_->name)->data;

            if (const Symbol sym = symbolTracker.lookup(name); !sym.value) {
                throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
            }
        } else {
            exprResolve(test);
        }

        for (auto& statement: statements) {
            exprResolve(statement);
        }
    }
}

void SemanticAnalyzer::checkConstantVar(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (const Symbol sym = symbolTracker.lookup(varName); sym.isConstant) {
        throw SemanticError(mFileName, ERROR(CONSTANT_VAR_ERROR, varName), 0);
    }
}

void SemanticAnalyzer::checkBool(const ExprPtr& var, TokenType ttype) const {
    if (ttype == TokenType::AND || ttype == TokenType::OR || ttype == TokenType::NOT)
        return;

    if (cast::toT(var)) {
        throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "t"), 0);
    }

    if (cast::toNIL(var)) {
        throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "nil"), 0);
    }
}

bool SemanticAnalyzer::checkDouble(const ExprPtr& n) {
    if (cast::toDouble(n))
        return true;
    if (const auto var = cast::toVar(n)) {
        if (cast::toDouble(var->value))
            return true;
    }

    return false;
}

void SemanticAnalyzer::checkBitwiseOp(const ExprPtr& n, TokenType ttype) {
    if (ttype == TokenType::LOGAND ||
        ttype == TokenType::LOGIOR ||
        ttype == TokenType::LOGXOR ||
        ttype == TokenType::LOGNOR) {
        throw SemanticError(mFileName, ERROR(NOT_INT_ERROR, std::get<double>(getValue(n))), 0);
    }
}

std::variant<int, double> SemanticAnalyzer::getValue(const ExprPtr& num) {
    auto getPrimitive = [&](const ExprPtr& n) -> std::variant<int, double> {
        if (const auto double_ = cast::toDouble(n))
            return double_->n;

        if (const auto int_ = cast::toInt(n)) {
            return int_->n;
        }
        return {};
    };

    if (cast::toInt(num) || cast::toDouble(num))
        return getPrimitive(num);

    if (const auto var = cast::toVar(num)) {
        return getPrimitive(var->value);
    }

    return {};
}

ExprPtr SemanticAnalyzer::numberResolve(ExprPtr& n, TokenType ttype) {
    if (cast::toInt(n) || cast::toDouble(n) || cast::toUninitialized(n)) {
        if (cast::toDouble(n)) {
            checkBitwiseOp(n, ttype);
        }

        return n;
    }

    // If var is t/nil and token type is different from and/or/not raise error.
    checkBool(n, ttype);

    if (auto binop = cast::toBinop(n)) {
        return binopResolve(*binop);
    }
    const auto var = cast::toVar(n);
    const std::string name = cast::toString(var->name)->data;

    const Symbol sym = symbolTracker.lookup(name);

    if (!sym.value) {
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
    }

    var->sType = sym.sType;
    auto innerVar = cast::toVar(sym.value);
    do {
        // If the value is param
        if (cast::toUninitialized(innerVar->value)) {
            ExprPtr name_ = cast::toString(var->name);
            ExprPtr value_ = std::make_shared<DoubleExpr>(0.0);
            n = std::make_shared<VarExpr>(name_, value_, sym.sType);
            return n;
        }

        checkBool(innerVar->value, ttype);
        // loop sym value until finding a primitive. Update var.
        if (cast::toInt(innerVar->value) ||
            cast::toDouble(innerVar->value) ||
            cast::toNIL(innerVar->value) ||
            cast::toT(innerVar->value)) {
            if (cast::toDouble(innerVar->value)) {
                checkBitwiseOp(innerVar->value, ttype);
            }

            ExprPtr name_ = cast::toString(var->name);
            ExprPtr value_ = innerVar->value;
            n = std::make_shared<VarExpr>(name_, value_, sym.sType);
            return n;
        }
        innerVar = cast::toVar(innerVar->value);
    } while (cast::toVar(innerVar));

    if (!innerVar) {
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
    }
    return nullptr;
}

void SemanticAnalyzer::valueResolve(const ExprPtr& var, bool isConstant) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (const auto value = cast::toVar(var_->value)) {
        const std::string valueName = cast::toString(value->name)->data;
        const Symbol sym = symbolTracker.lookup(valueName);

        if (!sym.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, varName), 0);
        }
        // Update value
        var_->value = sym.value;
        symbolTracker.bind(varName, {varName, var, var_->sType, isConstant});
    } else if (cast::toInt(var_->value) ||
               cast::toDouble(var_->value) ||
               cast::toNIL(var_->value) ||
               cast::toT(var_->value) ||
               cast::toString(var_->value) ||
               cast::toUninitialized(var_->value)) {
        ExprPtr name_ = var_->name;
        ExprPtr value_ = var_->value;
        const ExprPtr new_var = std::make_shared<VarExpr>(name_, value_, var_->sType);
        symbolTracker.bind(varName, {varName, new_var, var_->sType, isConstant});
    } else {
        ExprPtr name_ = var_->name;
        ExprPtr value_ = exprResolve(var_->value);
        const ExprPtr new_var = std::make_shared<VarExpr>(name_, value_, var_->sType);
        symbolTracker.bind(varName, {varName, new_var, var_->sType, isConstant});
    }
}
