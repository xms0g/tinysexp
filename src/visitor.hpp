#pragma once

struct NumberExpr;
struct StringExpr;
struct BinOpExpr;
struct DotimesExpr;
struct PrintExpr;
struct LetExpr;
struct VarExpr;

class ExprVisitor {
public:
    ExprVisitor() = default;

    virtual ~ExprVisitor() = default;

    virtual void visit(const NumberExpr&) {}

    virtual void visit(const StringExpr&) {}

    virtual void visit(const BinOpExpr&) {}

    virtual void visit(const DotimesExpr&) {}

    virtual void visit(const PrintExpr&) {}

    virtual void visit(const LetExpr&) {}

    virtual void visit(const VarExpr&) {}
};

template<typename VisitorImpl, typename VisitablePtr, typename RType>
class GenericVisitor {
public:
    static RType getResult(const VisitablePtr& n) {
        VisitorImpl visitor;
        n->accept(visitor);
        return visitor.mValue;
    }

    void store(RType value) {
        mValue = value;
    }

private:
    RType mValue;
};
