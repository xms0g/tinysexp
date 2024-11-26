#pragma once

struct NumberExpr;
struct StringExpr;
struct NILExpr;
struct BinOpExpr;
struct DotimesExpr;
struct PrintExpr;
struct LetExpr;
struct SetqExpr;
struct VarExpr;
class ExprVisitor {
public:
    ExprVisitor() = default;

    virtual ~ExprVisitor() = default;

    virtual void visit(const NumberExpr&) {}

    virtual void visit(const StringExpr&) {}

    virtual void visit(const NILExpr&) {}

    virtual void visit(const BinOpExpr&) {}

    virtual void visit(const DotimesExpr&) {}

    virtual void visit(const PrintExpr&) {}

    virtual void visit(const PrintExpr&, int param) {}

    virtual void visit(const LetExpr&) {}

    virtual void visit(const SetqExpr&) {}

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

#define MAKE_VISITOR(NAME, RVALUE, METHOD_NUMBER, METHOD_STR, METHOD_BINOP, METHOD_DOTIMES, METHOD_PRINT, METHOD_LET, MAKE_VAR) \
        class NAME : public GenericVisitor<NAME, ExprPtr, RVALUE>, public ExprVisitor { \
        public:             \
            METHOD_NUMBER   \
            METHOD_STR      \
            METHOD_BINOP    \
            METHOD_DOTIMES  \
            METHOD_PRINT    \
            METHOD_LET      \
            MAKE_VAR        \
            };

#define MAKE_MTHD_NUMBER void visit(const NumberExpr& num) override;
#define MAKE_MTHD_STR void visit(const StringExpr& str) override;
#define MAKE_MTHD_BINOP void visit(const BinOpExpr& binop) override;
#define MAKE_MTHD_BINOP_PARAM void visit(const BinOpExpr& binop, int param) override;
#define MAKE_MTHD_DOTIMES void visit(const DotimesExpr& dotimes) override;
#define MAKE_MTHD_PRINT void visit(const PrintExpr& print) override;
#define MAKE_MTHD_PRINT_PARAM void visit(const PrintExpr& print, int param) override;
#define MAKE_MTHD_LET void visit(const LetExpr& let) override;
#define MAKE_MTHD_VAR void visit(const VarExpr& var) override;
#define NULL_