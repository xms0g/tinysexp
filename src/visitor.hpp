#pragma once

struct IntExpr;
struct DoubleExpr;
struct StringExpr;
struct NILExpr;
struct BinOpExpr;
struct DotimesExpr;
struct PrintExpr;
struct ReadExpr;
struct LetExpr;
struct SetqExpr;
struct DefvarExpr;
struct VarExpr;

class ExprVisitor {
public:
    ExprVisitor() = default;

    virtual ~ExprVisitor() = default;

    virtual void visit(const IntExpr&) {}

    virtual void visit(const DoubleExpr&) {}

    virtual void visit(const StringExpr&) {}

    virtual void visit(const NILExpr&) {}

    virtual void visit(const BinOpExpr&) {}

    virtual void visit(const DotimesExpr&) {}

    virtual void visit(const PrintExpr&) {}

    virtual void visit(const ReadExpr&) {}

    virtual void visit(const LetExpr&) {}

    virtual void visit(const SetqExpr&) {}

    virtual void visit(const DefvarExpr&) {}

    virtual void visit(const VarExpr&) {}
};

template<typename VisitorImpl, typename VisitablePtr, typename RType>
class ValueGetter {
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

#define MAKE_VISITOR(NAME, RVALUE, ...) \
        class NAME : public ValueGetter<NAME, ExprPtr, RVALUE>, public ExprVisitor { \
        public:         \
            __VA_ARGS__ \
        };

#define MAKE_MTHD_INT void visit(const IntExpr& num) override;
#define MAKE_MTHD_DOUBLE void visit(const DoubleExpr& num) override;
#define MAKE_MTHD_STR void visit(const StringExpr& str) override;
#define MAKE_MTHD_BINOP void visit(const BinOpExpr& binop) override;
#define MAKE_MTHD_BINOP_PARAM void visit(const BinOpExpr& binop, int param) override;
#define MAKE_MTHD_DOTIMES void visit(const DotimesExpr& dotimes) override;
#define MAKE_MTHD_PRINT void visit(const PrintExpr& print) override;
#define MAKE_MTHD_READ void visit(const ReadExpr& print) override;
#define MAKE_MTHD_LET void visit(const LetExpr& let) override;
#define MAKE_MTHD_VAR void visit(const VarExpr& var) override;

MAKE_VISITOR(StringEvaluator, std::string, MAKE_MTHD_VAR MAKE_MTHD_STR)

