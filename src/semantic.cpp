#include "semantic.h"
#include "exceptions.hpp"

void ScopeTracker::enter(const std::string& scopeName) {
    std::unordered_map<std::string, Symbol> scope;
    symbolTable.push(scope);

    if (!scopeName.empty()) {
        scopeNames.push(scopeName);
    }
}

void ScopeTracker::exit(const bool isFunc) {
    symbolTable.pop();

    if (isFunc) {
        scopeNames.pop();
    }
}

std::string& ScopeTracker::scopeName() {
    return scopeNames.top();
}

size_t ScopeTracker::level() const {
    return symbolTable.size();
}

void ScopeTracker::bind(const std::string& name, const Symbol& symbol) {
    if (lookup(name).value) {
        update(name, symbol);
    } else {
        auto currentScope = symbolTable.top();
        symbolTable.pop();

        currentScope.emplace(name, symbol);
        symbolTable.push(currentScope);
    }
}

void ScopeTracker::update(const std::string& name, const Symbol& symbol) {
    std::stack<ScopeType> scopes;
  
    while (!symbolTable.empty()) {
        ScopeType scope = symbolTable.top();
        symbolTable.pop();
        
        if (scope.contains(name)) {
            scope[name] = symbol;
            scopes.push(scope);
            break;
        }
        // If the symbol is not found, push the scope back to the stack
        // and continue searching in the next scope
        scopes.push(scope);
    }

    // reconstruct the scopes
    while (!scopes.empty()) {
        symbolTable.push(scopes.top());
        scopes.pop();
    }
}

Symbol ScopeTracker::lookup(const std::string& name) {
    Symbol sym{};
    std::stack<ScopeType> scopes;

    while (!symbolTable.empty()) {
        ScopeType scope = symbolTable.top();
        symbolTable.pop();
        scopes.push(scope);

        if (scope.contains(name)) {
            sym = scope[name];
            break;
        }
        
    }
    // reconstruct the scopes
    while (!scopes.empty()) {
        symbolTable.push(scopes.top());
        scopes.pop();
    }

    return sym;
}

Symbol ScopeTracker::lookupCurrent(const std::string& name) {
    if (ScopeType currentScope = symbolTable.top(); currentScope.contains(name)) {
        return currentScope[name];
    }

    return {};
}

SemanticAnalyzer::SemanticAnalyzer(const char* fn) : fileName(fn) {
}

void SemanticAnalyzer::analyze(const ExprPtr& ast) {
    auto next = ast;

    symbolTracker.enter("global");
    while (next != nullptr) {
        exprResolve(next);
        next = next->child;
    }
    symbolTracker.exit();
}

ExprPtr SemanticAnalyzer::exprResolve(const ExprPtr& ast) {
    if (auto binop = cast::toBinop(ast)) {
        return binopResolve(*binop);
    } else if (const auto dotimes = cast::toDotimes(ast)) {
        return dotimesResolve(*dotimes);
    } else if (const auto loop = cast::toLoop(ast)) {
        return loopResolve(*loop);
    } else if (const auto let = cast::toLet(ast)) {
        return letResolve(*let);
    } else if (const auto setq = cast::toSetq(ast)) {
        return setqResolve(*setq);
    } else if (const auto defvar = cast::toDefvar(ast)) {
        defvarResolve(*defvar);
    } else if (const auto defconst = cast::toDefconstant(ast)) {
        defconstResolve(*defconst);
    } else if (cast::toDefun(ast)) {
        return defunResolve(ast);
    } else if (const auto funcCall = cast::toFuncCall(ast)) {
        return funcCallResolve(*funcCall);
    } else if (const auto return_ = cast::toReturn(ast)) {
        returnResolve(*return_);
    } else if (const auto if_ = cast::toIf(ast)) {
        return ifResolve(*if_);
    } else if (const auto when = cast::toWhen(ast)) {
        return whenResolve(*when);
    } else if (const auto cond = cast::toCond(ast)) {
        return condResolve(*cond);
    } else if (cast::toInt(ast) || cast::toDouble(ast) || cast::toVar(ast)) {
        if (cast::toVar(ast)) {
            return varResolve(const_cast<ExprPtr&>(ast), TokenType::VAR);
        }

        return ast;
    }

    return nullptr;
}

ExprPtr SemanticAnalyzer::binopResolve(BinOpExpr& binop) {
    ExprPtr lhs = nodeResolve(binop.lhs, binop.opToken.type);
    ExprPtr rhs = nodeResolve(binop.rhs, binop.opToken.type);

    if (cast::toDouble(lhs)) {
        return lhs;
    }

    if (cast::toDouble(rhs)) {
        return rhs;
    }

    return lhs;
}

ExprPtr SemanticAnalyzer::dotimesResolve(const DotimesExpr& dotimes) {
    symbolTracker.enter("");
    checkConstantVar(dotimes.iterationCount);

    const auto var = cast::toVar(dotimes.iterationCount);
    // Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
    // If it's expr, resolve it.
    valueResolve(var);

    ExprPtr result;
    for (const auto& statement: dotimes.statements) {
        result = exprResolve(statement);
    }
    symbolTracker.exit();

    return result;
}

ExprPtr SemanticAnalyzer::loopResolve(const LoopExpr& loop) {
    ExprPtr result;

    for (const auto& sexpr: loop.sexprs) {
        result = exprResolve(sexpr);
    }

    return result;
}

ExprPtr SemanticAnalyzer::letResolve(const LetExpr& let) {
    symbolTracker.enter("");
    for (const auto& var: let.bindings) {
        const auto var_ = cast::toVar(var);
        const std::string varName = cast::toString(var_->name)->data;

        // Check out the var in the current scope, if it's already defined, raise error
        if (const Symbol sym = symbolTracker.lookupCurrent(varName); sym.value) {
            throw SemanticError(fileName, ERROR(MULTIPLE_DECL_ERROR, varName), 0);
        }

        // Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
        // If it's expr, resolve it.
        valueResolve(var_);
    }

    ExprPtr result;
    for (const auto& statement: let.body) {
        result = exprResolve(statement);
    }

    symbolTracker.exit();

    return result;
}

ExprPtr SemanticAnalyzer::setqResolve(const SetqExpr& setq) {
    checkConstantVar(setq.pair);

    const auto var = cast::toVar(setq.pair);
    const std::string varName = cast::toString(var->name)->data;

    // Check out the var.If it's not defined, raise error.
    const Symbol sym = symbolTracker.lookup(varName);

    if (!sym.value) {
        throw SemanticError(fileName, ERROR(UNBOUND_VAR_ERROR, varName), 0);
    }
    // Resolve the var scope.
    var->sType = sym.sType;
    // Check out the value of var.If it's another var, look up all scopes.If it's not defined, raise error.
    // If it's int or double, update sym->value and bind again.
    // If it's expr, resolve it.
    return valueResolve(var);
}

void SemanticAnalyzer::defvarResolve(const DefvarExpr& defvar) {
    const auto var = cast::toVar(defvar.pair);
    const std::string varName = cast::toString(var->name)->data;

    if (symbolTracker.level() > 1) {
        throw SemanticError(fileName, ERROR(GLOBAL_VAR_DECL_ERROR, varName), 0);
    }

    valueResolve(var);
}

void SemanticAnalyzer::defconstResolve(const DefconstExpr& defconst) {
    const auto var = cast::toVar(defconst.pair);
    const std::string varName = cast::toString(var->name)->data;

    if (symbolTracker.level() > 1) {
        throw SemanticError(fileName, ERROR(CONSTANT_VAR_DECL_ERROR, varName), 0);
    }

    valueResolve(var, true);
}

ExprPtr SemanticAnalyzer::defunResolve(const ExprPtr& defun) {
    const auto func = cast::toDefun(defun);
    const auto var = cast::toVar(func->name);
    const std::string funcName = cast::toString(var->name)->data;

    symbolTracker.bind(funcName, {.name = funcName, .value = defun, .sType = SymbolType::GLOBAL});

    symbolTracker.enter(funcName);
    for (const auto& arg: func->args) {
        const auto argVar = cast::toVar(arg);
        const std::string argName = cast::toString(argVar->name)->data;
        symbolTracker.bind(argName, {.name = argName, .value = arg, .sType = argVar->sType});
    }

    ExprPtr result;
    for (const auto& statement: func->forms) {
        result = exprResolve(statement);
    }
    symbolTracker.exit(true);

    return result;
}

ExprPtr SemanticAnalyzer::funcCallResolve(FuncCallExpr& funcCall, bool isParam) {
    const auto var = cast::toVar(funcCall.name);
    const std::string funcName = cast::toString(var->name)->data;

    if (!isParam && symbolTracker.level() == 1) {
        tfCtx.isStarted = true;
        tfCtx.entryPoint = funcName;
    }

    Symbol sym = symbolTracker.lookup(funcName);

    if (!sym.value || !cast::toDefun(sym.value)) {
        throw SemanticError(fileName, ERROR(FUNC_UNDEFINED_ERROR, funcName), 0);
    }

    const auto func = cast::toDefun(sym.value);

    if (funcCall.args.size() != func->args.size()) {
        throw SemanticError(fileName, ERROR(FUNC_INVALID_NUMBER_OF_ARGS_ERROR, funcName, funcCall.args.size()), 0);
    }

    // Match the param names to values
    if (!funcCall.args.empty()) {
        const auto fcarg = funcCall.args[0];
        const auto fcargVar = cast::toVar(fcarg);

        if ((fcargVar && cast::toUninitialized(fcargVar->value)) ||
            isPrimitive(fcarg) || cast::toBinop(fcarg) || cast::toFuncCall(fcarg)) {
            for (size_t i = 0; i < func->args.size(); ++i) {
                const auto fArg = cast::toVar(func->args[i]);

                ExprPtr name = fArg->name;
                ExprPtr value = funcCall.args[i];

                funcCall.args[i] = std::make_shared<VarExpr>(name, value, fArg->sType);
            }
        }
    }
    // Check out if the params are already resolved
    for (const auto& arg: funcCall.args) {
        auto argVar = cast::toVar(arg);

        if (isPrimitive(argVar->value)) {
            setType(*argVar, argVar->value);
            continue;
        }

        if (auto binop = cast::toBinop(argVar->value)) {
            auto value = binopResolve(*binop);
            setType(*argVar, value);
            continue;
        }

        if (auto fc = cast::toFuncCall(argVar->value)) {
            auto value = funcCallResolve(*fc, true);
            setType(*argVar, value);
            continue;
        }

        if (auto innerVar = cast::toVar(argVar->value)) {
            bool found{false};

            do {
                const std::string innerVarName = cast::toString(innerVar->name)->data;

                sym = symbolTracker.lookup(innerVarName);

                if (sym.value) {
                    auto sym_value = cast::toVar(sym.value);
                    innerVar->value = sym_value->value;
                    innerVar->sType = sym_value->sType;
                    // Loop sym value until finding a primitive. Update var.
                    if (isPrimitive(sym_value->value)) {
                        setType(*innerVar, sym_value->value);
                        setType(*argVar, sym_value->value);
                        break;
                    }

                    if (auto innerValue = cast::toVar(sym_value->value)) {
                        do {
                            if (isPrimitive(innerValue->value)) {
                                setType(*innerVar, innerValue->value);
                                setType(*argVar, innerValue->value);
                                found = true;
                                break;
                            }
                            innerValue = cast::toVar(innerValue->value);
                        } while (innerValue);
                    }
                }
                if (found)
                    break;
                innerVar = cast::toVar(innerVar->value);
            } while (innerVar);
        }
    }
    // Make the arg type local because we'll keep them onto stack inside function
    int scratchIdx = 0, sseIdx = 0;
    auto makeLocal = [&](VarExpr& arg) {
        // The params beyond 6 for scratch and beyond 7 for SSE are already onto stack
        if (arg.vType == VarType::INT && scratchIdx < 6) {
            arg.sType = SymbolType::LOCAL;
            scratchIdx++;
        } else if (arg.vType == VarType::DOUBLE && sseIdx < 8) {
            arg.sType = SymbolType::LOCAL;
            sseIdx++;
        }
    };

    if (tfCtx.isStarted) {
        for (size_t i = 0; i < funcCall.args.size(); ++i) {
            auto arg = cast::toVar(funcCall.args[i]);
            makeLocal(*arg);
            func->args[i] = funcCall.args[i];
        }
        // Find the proper type of variables and the return type of the function
        if (std::string& currentScope = symbolTracker.scopeName(); currentScope != funcName) {
            funcCall.returnType = defunResolve(func);

            if (funcName == tfCtx.entryPoint)
                tfCtx.isStarted = false;
        }
    }

    return funcCall.returnType;
}

void SemanticAnalyzer::returnResolve(const ReturnExpr& return_) {
    if (cast::toT(return_.arg) || cast::toNIL(return_.arg)) return;

    const auto arg = cast::toVar(return_.arg);
    const std::string argName = cast::toString(arg->name)->data;

    // Check out the var.If it's not defined, raise error.
    if (const Symbol sym = symbolTracker.lookup(argName); !sym.value) {
        throw SemanticError(fileName, ERROR(UNBOUND_VAR_ERROR, argName), 0);
    }
}

ExprPtr SemanticAnalyzer::ifResolve(IfExpr& if_) {
    if (const auto test = cast::toVar(if_.test)) {
        const std::string name = cast::toString(test->name)->data;

        const Symbol sym = symbolTracker.lookup(name);
        if (!sym.value) {
            throw SemanticError(fileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
        }

        if_.test = sym.value;
    } else {
        exprResolve(if_.test);
    }

    ExprPtr result = exprResolve(if_.then);

    if (!cast::toUninitialized(if_.else_)) {
        result = exprResolve(if_.else_);
    }

    return result;
}

ExprPtr SemanticAnalyzer::whenResolve(WhenExpr& when) {
    if (const auto test = cast::toVar(when.test)) {
        const std::string name = cast::toString(test->name)->data;

        const Symbol sym = symbolTracker.lookup(name);
        if (!sym.value) {
            throw SemanticError(fileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
        }

        when.test = sym.value;
    } else {
        exprResolve(when.test);
    }

    ExprPtr result;
    for (const auto& form: when.then) {
        result = exprResolve(form);
    }

    return result;
}

ExprPtr SemanticAnalyzer::condResolve(CondExpr& cond) {
    ExprPtr result;

    for (auto& [test, statements]: cond.variants) {
        if (const auto test_ = cast::toVar(test)) {
            const std::string name = cast::toString(test_->name)->data;

            const Symbol sym = symbolTracker.lookup(name);
            if (!sym.value) {
                throw SemanticError(fileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
            }

            test = sym.value;
        } else {
            exprResolve(test);
        }

        for (const auto& statement: statements) {
            result = exprResolve(statement);
        }
    }

    return result;
}

void SemanticAnalyzer::checkConstantVar(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (const Symbol sym = symbolTracker.lookup(varName); sym.isConstant) {
        throw SemanticError(fileName, ERROR(CONSTANT_VAR_ERROR, varName), 0);
    }
}

void SemanticAnalyzer::checkBool(const ExprPtr& var, const TokenType ttype) const {
    if (ttype == TokenType::AND || ttype == TokenType::OR || ttype == TokenType::NOT)
        return;

    if (cast::toT(var)) {
        throw SemanticError(fileName, ERROR(NOT_NUMBER_ERROR, "t"), 0);
    }

    if (cast::toNIL(var)) {
        throw SemanticError(fileName, ERROR(NOT_NUMBER_ERROR, "nil"), 0);
    }
}

void SemanticAnalyzer::checkBitwiseOp(const ExprPtr& n, const TokenType ttype) {
    if (ttype == TokenType::LOGAND ||
        ttype == TokenType::LOGIOR ||
        ttype == TokenType::LOGXOR ||
        ttype == TokenType::LOGNOR) {
        throw SemanticError(fileName, ERROR(NOT_INT_ERROR, std::get<double>(getValue(n))), 0);
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

ExprPtr SemanticAnalyzer::returnValue(const VarExpr& var) {
    if (var.vType == VarType::INT) {
        return std::make_shared<IntExpr>(0);
    }

    if (var.vType == VarType::DOUBLE) {
        return std::make_shared<DoubleExpr>(0.0);
    }

    if (var.vType == VarType::STRING) {
        return std::make_shared<StringExpr>();
    }

    if (var.vType == VarType::NIL) {
        return std::make_shared<NILExpr>();
    }

    if (var.vType == VarType::T) {
        return std::make_shared<TExpr>();
    }
}

ExprPtr SemanticAnalyzer::varResolve(ExprPtr& n, const TokenType ttype) {
    if (isPrimitive(n) || cast::toUninitialized(n)) {
        if (cast::toDouble(n)) {
            checkBitwiseOp(n, ttype);
        }

        return n;
    }

    // If var is t/nil and token type is different from and/or/not raise error.
    checkBool(n, ttype);

    const auto var = cast::toVar(n);
    const std::string name = cast::toString(var->name)->data;

    const Symbol sym = symbolTracker.lookup(name);

    if (!sym.value) {
        throw SemanticError(fileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
    }

    var->sType = sym.sType;

    auto innerVar = cast::toVar(sym.value);

    // If we already know the type, return it.
    if (innerVar->vType != VarType::UNKNOWN) {
        var->vType = innerVar->vType;
        var->value = innerVar->value;
        return returnValue(*innerVar);
    }
    // Loop sym value until finding a primitive. Update var.
    do {
        checkBool(innerVar->value, ttype);

        if (isPrimitive(innerVar->value)) {
            if (innerVar->vType == VarType::DOUBLE) {
                checkBitwiseOp(innerVar->value, ttype);
            }

            ExprPtr value_;
            if (innerVar->vType == VarType::INT) {
                value_ = std::make_shared<IntExpr>(0);
            } else if (innerVar->vType == VarType::DOUBLE) {
                value_ = std::make_shared<DoubleExpr>(0.0);
            }
            var->value = value_;
            var->vType = innerVar->vType;
            return value_;
        }

        if (auto binop = cast::toBinop(innerVar->value)) {
            const auto value = binopResolve(*binop);
            setType(*var, value);
            var->value = innerVar->value;
            return returnValue(*var);
        }

        if (const auto fc = cast::toFuncCall(innerVar->value)) {
            const auto value = funcCallResolve(*fc);
            setType(*var, value);
            var->value = innerVar->value;
            return returnValue(*var);
        }
        // If the value is param
        if (cast::toUninitialized(innerVar->value)) {
            var->value = std::make_shared<DoubleExpr>(0.0);;
            return var->value;
        }

        innerVar = cast::toVar(innerVar->value);
    } while (innerVar);

    if (!innerVar) {
        throw SemanticError(fileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
    }

    return nullptr;
}

ExprPtr SemanticAnalyzer::nodeResolve(ExprPtr& n, const TokenType ttype) {
    if (const auto binop = cast::toBinop(n)) {
        return binopResolve(*binop);
    }

    if (const auto funcCall = cast::toFuncCall(n)) {
        return funcCallResolve(*funcCall);
    }

    return varResolve(n, ttype);
}

ExprPtr SemanticAnalyzer::valueResolve(const ExprPtr& var, const bool isConstant) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (isPrimitive(var_->value) || cast::toUninitialized(var_->value)) {
        setType(*var_, var_->value);

        symbolTracker.bind(varName, {
                               .name = varName,
                               .value = var,
                               .sType = var_->sType,
                               .isConstant = isConstant
                           });
        return var_->value;
    }

    if (const auto value = cast::toVar(var_->value)) {
        const std::string valueName = cast::toString(value->name)->data;
        const Symbol sym = symbolTracker.lookup(valueName);

        if (!sym.value) {
            throw SemanticError(fileName, ERROR(UNBOUND_VAR_ERROR, varName), 0);
        }
        // Update value
        var_->value = sym.value;
        var_->vType = cast::toVar(sym.value)->vType;

        symbolTracker.bind(varName, {
                               .name = varName,
                               .value = var,
                               .sType = var_->sType,
                               .isConstant = isConstant
                           });
        return var_->value;
    }

    ExprPtr name = var_->name;
    ExprPtr value_ = exprResolve(var_->value);
    var_->vType = cast::toInt(value_) ? VarType::INT : VarType::DOUBLE;

    symbolTracker.bind(varName, {
                           .name = varName,
                           .value = var,
                           .sType = var_->sType,
                           .isConstant = isConstant
                       });

    return value_;
}

void SemanticAnalyzer::setType(VarExpr& var, const ExprPtr& value) {
    if (cast::toInt(value)) {
        var.vType = VarType::INT;
    } else if (cast::toDouble(value)) {
        var.vType = VarType::DOUBLE;
    } else if (cast::toString(value)) {
        var.vType = VarType::STRING;
    } else if (cast::toT(value)) {
        var.vType = VarType::T;
    } else if (cast::toNIL(value)) {
        var.vType = VarType::NIL;
    }
}
