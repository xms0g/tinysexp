#include "semantic.h"
#include "visitors.h"

void SemanticAnalyzer::analyze(ExprPtr& ast) {
    Analyzer::get(ast);
}

void Analyzer::visit(const BinOpExpr& binop) {

}

void Analyzer::visit(const DotimesExpr& dotimes) {

}

void Analyzer::visit(const LetExpr& let) {

}

void Analyzer::visit(const SetqExpr& setq) {

}

void Analyzer::visit(const DefvarExpr& defvar) {
    Symbol s = {StringEvaluator::get(defvar.var),
                nullptr,//NumberEvaluator::get(defvar.var),
                SymbolType::GLOBAL};
}

void Analyzer::visit(const DefconstExpr&) {

}

void Analyzer::visit(const DefunExpr&) {

}