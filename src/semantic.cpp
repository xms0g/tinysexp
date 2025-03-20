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
    Symbol sym{};
    std::stack<ScopeType> scopes;
    const size_t level = mSymbolTable.size();

    for (size_t i = 0; i < level; ++i) {
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
    for (size_t i = 0; i < currentLevel; ++i) {
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

void SemanticAnalyzer::analyze(const ExprPtr& ast) {
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
        if (const auto var = cast::toVar(ast)) {
            return var->value;
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
    symbolTracker.enter();
    checkConstantVar(dotimes.iterationCount);

    const auto var = cast::toVar(dotimes.iterationCount);
    // Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
    // If it's expr, resolve it.
    valueResolve(var);

    ExprPtr result;
    for (auto& statement: dotimes.statements) {
        result = exprResolve(statement);
    }
    symbolTracker.exit();

    return result;
}

ExprPtr SemanticAnalyzer::loopResolve(const LoopExpr& loop) {
    ExprPtr result;

    for (auto& sexpr: loop.sexprs) {
        result = exprResolve(sexpr);
    }

    return result;
}

ExprPtr SemanticAnalyzer::letResolve(const LetExpr& let) {
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

    ExprPtr result;
    for (auto& statement: let.body) {
        result = exprResolve(statement);
    }
    // Update the type of binding
    for (auto& var: let.bindings) {
        const auto var_ = cast::toVar(var);

        if (const std::string varName = cast::toString(var_->name)->data;
            tfCtx.symbolTypeTable.contains(varName)) {
            var_->value = tfCtx.symbolTypeTable.at(varName);
        }
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
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, varName), 0);
    }
    // Resolve the var scope.
    var->sType = sym.sType;
    // Check out the value of var.If it's another var, look up all scopes.If it's not defined, raise error.
    // If it's int or double, update sym->value and bind again.
    // If it's expr, resolve it.
    valueResolve(var);

    if (tfCtx.symbolTypeTable.contains(varName)) {
        return tfCtx.symbolTypeTable.at(varName);
    }

    return var->value;
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

ExprPtr SemanticAnalyzer::defunResolve(const ExprPtr& defun) {
    const auto func = cast::toDefun(defun);
    const auto var = cast::toVar(func->name);
    const std::string funcName = cast::toString(var->name)->data;

    symbolTracker.bind(funcName, {.name = funcName, .value = defun, .sType = SymbolType::GLOBAL});

    symbolTracker.enter();
    for (auto& arg: func->args) {
        const auto argVar = cast::toVar(arg);
        const std::string argName = cast::toString(argVar->name)->data;
        symbolTracker.bind(argName, {.name = argName, .value = arg, .sType = argVar->sType});
    }

    ExprPtr result;
    for (auto& statement: func->forms) {
        result = exprResolve(statement);
    }
    symbolTracker.exit();

    return result;
}

ExprPtr SemanticAnalyzer::funcCallResolve(FuncCallExpr& funcCall) {
    const auto var = cast::toVar(funcCall.name);
    const std::string funcName = cast::toString(var->name)->data;

    if (symbolTracker.level() == 1) {
        tfCtx.isStarted = true;
        tfCtx.entryPoint = funcName;
    }

    Symbol sym = symbolTracker.lookup(funcName);

    if (!sym.value || !cast::toDefun(sym.value)) {
        throw SemanticError(mFileName, ERROR(FUNC_UNDEFINED_ERROR, funcName), 0);
    }

    const auto func = cast::toDefun(sym.value);

    if (funcCall.args.size() != func->args.size()) {
        throw SemanticError(mFileName, ERROR(FUNC_INVALID_NUMBER_OF_ARGS_ERROR, funcName, funcCall.args.size()), 0);
    }

    // Match the param names to values
    if (!funcCall.args.empty()) {
        const auto fcarg = funcCall.args[0];
        const auto farg = cast::toVar(func->args[0]);
        const auto fcargVar = cast::toVar(fcarg);

        if ((fcargVar && cast::toString(farg->name)->data != cast::toString(fcargVar->name)->data) ||
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
        if (auto argVar = cast::toVar(arg)) {
            if (auto binop = cast::toBinop(argVar->value)) {
                auto value = binopResolve(*binop);
                setType(*argVar, value);
                continue;
            }

            if (auto fc = cast::toFuncCall(argVar->value)) {
                auto value = funcCallResolve(*fc);
                setType(*argVar, value);
                continue;
            }

            bool found{false};
            auto innerVar = argVar;
            do {
                const std::string argName = cast::toString(innerVar->name)->data;
                sym = symbolTracker.lookup(argName);
                if (sym.value) {
                    innerVar->sType = sym.sType;
                    // Loop sym value until finding a primitive. Update var.
                    if (auto sym_value = cast::toVar(sym.value); isPrimitive(sym_value->value)) {
                        innerVar->value = sym_value->value;
                        setType(*argVar, sym_value->value);
                        break;
                    } else if (auto innerValue = cast::toVar(sym_value->value)) {
                        do {
                            if (isPrimitive(innerValue->value)) {
                                innerVar->value = innerValue;
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
        func->args.clear();

        for (auto& arg: funcCall.args) {
            auto argVar = cast::toVar(arg);
            makeLocal(*argVar);
            func->args.push_back(arg);
        }
    }
    // Find the proper type of variables and the return type of the function
    funcCall.returnType = defunResolve(func);

    if (funcName == tfCtx.entryPoint)
        tfCtx.isStarted = false;

    return funcCall.returnType;
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

ExprPtr SemanticAnalyzer::ifResolve(IfExpr& if_) {
    if (const auto test = cast::toVar(if_.test)) {
        const std::string name = cast::toString(test->name)->data;

        const Symbol sym = symbolTracker.lookup(name);
        if (!sym.value) {
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
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
            throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
        }

        when.test = sym.value;
    } else {
        exprResolve(when.test);
    }

    ExprPtr result;
    for (auto& form: when.then) {
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
                throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
            }

            test = sym.value;
        } else {
            exprResolve(test);
        }

        for (auto& statement: statements) {
            result = exprResolve(statement);
        }
    }

    return result;
}

void SemanticAnalyzer::checkConstantVar(const ExprPtr& var) {
    const auto var_ = cast::toVar(var);
    const std::string varName = cast::toString(var_->name)->data;

    if (const Symbol sym = symbolTracker.lookup(varName); sym.isConstant) {
        throw SemanticError(mFileName, ERROR(CONSTANT_VAR_ERROR, varName), 0);
    }
}

void SemanticAnalyzer::checkBool(const ExprPtr& var, const TokenType ttype) const {
    if (ttype == TokenType::AND || ttype == TokenType::OR || ttype == TokenType::NOT)
        return;

    if (cast::toT(var)) {
        throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "t"), 0);
    }

    if (cast::toNIL(var)) {
        throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "nil"), 0);
    }
}

void SemanticAnalyzer::checkBitwiseOp(const ExprPtr& n, const TokenType ttype) {
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

ExprPtr SemanticAnalyzer::numberResolve(ExprPtr& n, const TokenType ttype) {
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
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
    }

    var->sType = sym.sType;

    // If we already know the type, don't check
    if (tfCtx.isStarted && tfCtx.symbolTypeTable.contains(name)) {
        ExprPtr name_ = cast::toString(var->name);
        ExprPtr value_ = tfCtx.symbolTypeTable.at(name);
        n = std::make_shared<VarExpr>(name_, value_, sym.sType);
        return cast::toVar(n)->value;
    }

    auto innerVar = cast::toVar(sym.value);
    do {
        checkBool(innerVar->value, ttype);
        // If the value is param
        if (cast::toUninitialized(innerVar->value)) {
            ExprPtr name_ = cast::toString(var->name);
            ExprPtr value_ = std::make_shared<DoubleExpr>(0.0);
            n = std::make_shared<VarExpr>(name_, value_, sym.sType);
            return cast::toVar(n)->value;
        }
        // Loop sym value until finding a primitive. Update var.
        if (isPrimitive(innerVar->value)) {
            if (cast::toDouble(innerVar->value)) {
                checkBitwiseOp(innerVar->value, ttype);
            }

            ExprPtr name_ = cast::toString(var->name);
            ExprPtr value_;

            if (cast::toInt(innerVar->value)) {
                value_ = std::make_shared<IntExpr>(0);
            } else if (cast::toDouble(innerVar->value)) {
                value_ = std::make_shared<DoubleExpr>(0.0);
            }
            var->value = value_;
            return value_;
        }
        innerVar = cast::toVar(innerVar->value);
    } while (innerVar);

    if (!innerVar) {
        throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
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

    return numberResolve(n, ttype);
}

void SemanticAnalyzer::valueResolve(const ExprPtr& var, const bool isConstant) {
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
        symbolTracker.bind(varName, {
                               .name = varName,
                               .value = var,
                               .sType = var_->sType,
                               .isConstant = isConstant
                           });
    } else if (isPrimitive(var_->value) || cast::toUninitialized(var_->value)) {
        symbolTracker.bind(varName, {
                               .name = varName,
                               .value = var,
                               .sType = var_->sType,
                               .isConstant = isConstant
                           });
    } else {
        ExprPtr name = var_->name;
        ExprPtr value_ = exprResolve(var_->value);

        symbolTracker.bind(varName, {
                               .name = varName,
                               .value = var,
                               .sType = var_->sType,
                               .isConstant = isConstant
                           });

        if (!tfCtx.isStarted) return;

        if (tfCtx.symbolTypeTable.contains(varName)) {
            tfCtx.symbolTypeTable[varName] = value_;
        } else {
            tfCtx.symbolTypeTable.emplace(varName, value_);
        }
    }
}

bool SemanticAnalyzer::isPrimitive(const ExprPtr& var) {
    return cast::toInt(var) ||
           cast::toDouble(var) ||
           cast::toNIL(var) ||
           cast::toT(var) ||
           cast::toString(var);
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
