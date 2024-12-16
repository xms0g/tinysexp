#include "semantic.h"
#include <format>
#include "visitors.h"
#include "exceptions.hpp"

namespace {
constexpr const char* UNBOUND_VAR = "The variable '{}' is unbound";
constexpr const char* MULTIPLE_DECL = "The variable '{}' occurs more than once in {}";
constexpr const char* INVALID_NUMBER_OF_ARGS = "Invalid number of arguments: {}";

}

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
    std::variant<int, double> n;

    n = NumberEvaluator::get(var);

    if (!std::holds_alternative<int>(n) || !std::holds_alternative<double>(n)) {
        std::string name = StringEvaluator::get(var);
        Symbol sym = scopeLookup(name);

        if (!sym.value) {
            throw UnboundVariableError(fileName, std::format(UNBOUND_VAR, name).c_str(), 0);
        } else {
           var = sym.value;
           var->sType = sym.sType;
        }
    }
}

void Resolver::visit(BinOpExpr& binop) {
    varResolve(binop.lhs);
    varResolve(binop.rhs);
}

void Resolver::visit(const DotimesExpr& dotimes) {

}

void Resolver::visit(const LetExpr& let) {
    scopeEnter();
    for (auto& var: let.bindings) {
        std::string name = StringEvaluator::get(var);

        Symbol sym = scopeLookupCurrent(name);
        if (sym.value) {
            throw MultipleDeclarationError(fileName, std::format(MULTIPLE_DECL, name, "LET").c_str(), 0);
        }

        scopeBind(name, {name, var, SymbolType::LOCAL});
    }

    for (auto& statement: let.body) {
        Resolver::get(statement);
    }
    scopeExit();
}

void Resolver::visit(const SetqExpr& setq) {
    std::string name = StringEvaluator::get(setq.var);
    Symbol sym = scopeLookup(name);

    if (!sym.value) {
        throw UnboundVariableError(fileName, std::format(UNBOUND_VAR, name).c_str(), 0);
    }
}

void Resolver::visit(const DefvarExpr& defvar) {
    std::string name = StringEvaluator::get(defvar.var);

    scopeBind(name, {name, defvar.var, SymbolType::GLOBAL});
}

void Resolver::visit(const DefconstExpr& defconst) {
    std::string name = StringEvaluator::get(defconst.var);

    scopeBind(name, {name, defconst.var, SymbolType::GLOBAL});
}

void Resolver::visit(const DefunExpr& defun) {
    std::string name = StringEvaluator::get(defun.name);
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