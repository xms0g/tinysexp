#include "semantic.h"
#include <format>
#include "visitors.h"
#include "exceptions.hpp"

namespace {
constexpr const char* UNBOUND_VAR = "The variable {} is unbound";
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

void Resolver::visit(const BinOpExpr& binop) {
    std::variant<int, double> lhs, rhs;

    lhs = NumberEvaluator::get(binop.lhs);
    rhs = NumberEvaluator::get(binop.rhs);

    if (!std::holds_alternative<int>(lhs) || !std::holds_alternative<double>(lhs)) {
        std::string name = StringEvaluator::get(binop.lhs);
        Symbol sym = scopeLookup(name);

        if (!sym.value) {
            throw UnboundVariableError(fileName, std::format(UNBOUND_VAR, name).c_str(), 0);
        }
    }

    if (!std::holds_alternative<int>(rhs) || !std::holds_alternative<double>(rhs)) {
        std::string name = StringEvaluator::get(binop.rhs);
        Symbol sym = scopeLookup(name);

        if (!sym.value) {
            throw UnboundVariableError(fileName, std::format(UNBOUND_VAR, name).c_str(), 0);
        }
    }
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

        Symbol s = {name, var, SymbolType::LOCAL};
        scopeBind(name, s);
    }

    for (auto& statement: let.body) {
        Resolver::get(statement);
    }
    scopeExit();
}

void Resolver::visit(const SetqExpr& setq) {

}

void Resolver::visit(const DefvarExpr& defvar) {
    Symbol s = {StringEvaluator::get(defvar.var),
                defvar.var,
                SymbolType::GLOBAL};

    scopeBind(StringEvaluator::get(defvar.var), s);

}

void Resolver::visit(const DefconstExpr& defconst) {
    Symbol s = {StringEvaluator::get(defconst.var),
                defconst.var,
                SymbolType::GLOBAL};

    scopeBind(StringEvaluator::get(defconst.var), s);
}

void Resolver::visit(const DefunExpr& defun) {
    std::string name = StringEvaluator::get(defun.name);
    ExprPtr func = std::make_shared<DefunExpr>(defun);

    Symbol s = {name,
                func,
                SymbolType::GLOBAL};

    scopeBind(name, s);

    scopeEnter();
    for (auto& param: defun.params) {

    }

    for (auto& statement: defun.body) {

    }
    scopeExit();
}

void Resolver::visit(const FuncCallExpr& funcCall) {

}