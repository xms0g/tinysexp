#ifndef TINYSEXP_VISITOR_H
#define TINYSEXP_VISITOR_H

struct IntExpr;
struct DoubleExpr;
struct StringExpr;
struct NILExpr;
struct TExpr;
struct BinOpExpr;
struct DotimesExpr;
struct LetExpr;
struct SetqExpr;
struct DefvarExpr;
struct DefconstExpr;
struct DefunExpr;
struct FuncCallExpr;
struct IfExpr;
struct WhenExpr;
struct CondExpr;
struct VarExpr;

class ExprVisitor {
public:
    ExprVisitor() = default;

    virtual ~ExprVisitor() = default;

    virtual void visit(const IntExpr&) {}

    virtual void visit(const DoubleExpr&) {}

    virtual void visit(const StringExpr&) {}

    virtual void visit(const NILExpr&) {}

    virtual void visit(const TExpr&) {}

    virtual void visit(BinOpExpr&) {}

    virtual void visit(const DotimesExpr&) {}

    virtual void visit(const LetExpr&) {}

    virtual void visit(const SetqExpr&) {}

    virtual void visit(const DefvarExpr&) {}

    virtual void visit(const DefconstExpr&) {}

    virtual void visit(const DefunExpr&) {}

    virtual void visit(const FuncCallExpr&) {}

    virtual void visit(const IfExpr&) {}

    virtual void visit(const WhenExpr&) {}

    virtual void visit(const CondExpr&) {}

    virtual void visit(const VarExpr&) {}
};

template<typename VisitorImpl, typename VisitablePtr, typename RType>
class Getter {
public:
    static RType get(const VisitablePtr& expr) {
        VisitorImpl visitor;
        expr->accept(visitor);
        return visitor.mValue;
    }

    void set(RType value) {
        mValue = value;
    }

private:
    RType mValue;
};

#define MAKE_VISITOR(NAME, RVALUE, ...) \
        class NAME : public Getter<NAME, ExprPtr, RVALUE>, public ExprVisitor { \
        public:         \
            __VA_ARGS__ \
        };

#define MAKE_MTHD_INT void visit(const IntExpr& num) override;
#define MAKE_MTHD_DOUBLE void visit(const DoubleExpr& num) override;
#define MAKE_MTHD_STR void visit(const StringExpr& str) override;
#define MAKE_MTHD_BINOP void visit(BinOpExpr& binop) override;
#define MAKE_MTHD_BINOP_PARAM void visit(const BinOpExpr& binop, int param) override;
#define MAKE_MTHD_DOTIMES void visit(const DotimesExpr& dotimes) override;
#define MAKE_MTHD_LET void visit(const LetExpr& let) override;
#define MAKE_MTHD_VAR void visit(const VarExpr& var) override;

#endif