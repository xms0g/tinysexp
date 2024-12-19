#include "semantic.h"
#include <format>
#include "visitors.h"
#include "exceptions.hpp"

void SemanticAnalyzer::analyze(const char* fn, ExprPtr& ast) {
    fileName = fn;
    ExprPtr next = ast;

    scopeEnter();
    while (next != nullptr) {
        Resolver::get(next);
        next = next->child;
    }
    scopeExit();
}

void scopeEnter() {
    std::unordered_map<std::string, Symbol> scope;
    symbolTable.push(scope);
}

void scopeExit() {
    symbolTable.pop();
}

size_t scopeLevel() {
    return symbolTable.size();
}

void scopeBind(const std::string& name, const Symbol& symbol) {
    auto scope = symbolTable.top();
    symbolTable.pop();
    scope.emplace(name, symbol);
    symbolTable.push(scope);
}

Symbol scopeLookup(const std::string& name) {
    Symbol sym;
    std::stack<ScopeType> scopes;
    size_t level = symbolTable.size();

    for (int i = 0; i < level; ++i) {
        ScopeType scope = symbolTable.top();
        symbolTable.pop();
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
        symbolTable.push(scope);
    }

    return sym;
}

Symbol scopeLookupCurrent(const std::string& name) {
    ScopeType currentScope = symbolTable.top();

    if (auto elem = currentScope.find(name);elem != currentScope.end()) {
        return elem->second;
    }

    return {};
}

void varResolve(ExprPtr& var) {
    bool isT = TEval::get(var);
    bool isNil = NILEval::get(var);

    if (isT || isNil) {
        if (isT) {
            throw TypeError(fileName, T_VAR_ERROR, 0);
        }

        if (isNil) {
            throw TypeError(fileName, NIL_VAR_ERROR, 0);
        }
    }

    std::variant<int, double> n;

    n = NumberEval::get(var);

    if (!std::holds_alternative<int>(n) && !std::holds_alternative<double>(n)) {
        std::string name = StringEval::get(var);
        Symbol sym = scopeLookup(name);

        if (!sym.value) {
            throw UnboundVariableError(fileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
        }
    }
}

void Resolver::visit(BinOpExpr& binop) {
    varResolve(binop.lhs);
    varResolve(binop.rhs);
}

void Resolver::visit(const DotimesExpr& dotimes) {
    scopeEnter();
    std::string name = StringEval::get(dotimes.iterationCount);

    Symbol sym = scopeLookup(name);
    if (sym.isConstant) {
        throw TypeError(fileName, ERROR(CONSTANT_VAR_ERROR, name), 0);
    }
    scopeExit();
}

void Resolver::visit(const LetExpr& let) {
    scopeEnter();
    for (auto& var: let.bindings) {
        std::string name = StringEval::get(var);

        Symbol sym = scopeLookupCurrent(name);
        if (sym.value) {
            throw MultipleDeclarationError(fileName, ERROR(MULTIPLE_DECL_ERROR, name + " in LET"), 0);
        }

        scopeBind(name, {name, var, SymbolType::LOCAL});
    }

    for (auto& statement: let.body) {
        Resolver::get(statement);
    }
    scopeExit();
}

void Resolver::visit(const SetqExpr& setq) {
    std::string name = StringEval::get(setq.var);
    Symbol sym = scopeLookup(name);

    if (!sym.value) {
        throw UnboundVariableError(fileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
    } else {
        if (sym.isConstant) {
            throw TypeError(fileName, ERROR(CONSTANT_VAR_ERROR, name), 0);
        }
    }
}

void Resolver::visit(const DefvarExpr& defvar) {
    std::string name = StringEval::get(defvar.var);

    if (scopeLevel() > 1) {
        throw ScopeError(fileName, ERROR(GLOBAL_VAR_DECL_ERROR, name), 0);
    }

    scopeBind(name, {name, defvar.var, SymbolType::GLOBAL});
}

void Resolver::visit(const DefconstExpr& defconst) {
    std::string name = StringEval::get(defconst.var);

    if (scopeLevel() > 1) {
        throw ScopeError(fileName, ERROR(CONSTANT_VAR_DECL_ERROR, name), 0);
    }

    scopeBind(name, {name, defconst.var, SymbolType::GLOBAL, true});
}

void Resolver::visit(const DefunExpr& defun) {
    std::string name = StringEval::get(defun.name);

    if (scopeLevel() > 1) {
        throw ScopeError(fileName, ERROR(FUNC_DEF_ERROR, name), 0);
    }

    ExprPtr func = std::make_shared<DefunExpr>(defun);

    scopeBind(name, {name, func, SymbolType::GLOBAL});

    scopeEnter();
    for (auto& param: defun.params) {

    }

    for (auto& statement: defun.body) {

    }
    scopeExit();
}

void Resolver::visit(const FuncCallExpr& funcCall) {

}

void Resolver::visit(const IfExpr& ifExpr) {

}

void Resolver::visit(const WhenExpr& when) {

}

void Resolver::visit(const CondExpr& cond) {

}