#ifndef TINYSEXP_CODEGEN_H
#define TINYSEXP_CODEGEN_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include "parser.h"

enum Register {
    RAX, RBX, RCX,
    RDX, RDI, RSI,
    R8, R9, R10,
    R11, R12, R13,
    R14, R15,
    xmm0, xmm1, xmm2,
    xmm3, xmm4, xmm5,
    xmm6, xmm7, EOR
};

enum RegisterType {
    GP, SSE
};

struct RegisterPair {
    Register reg;
    RegisterType rType;
    const char* sreg;
};

struct OpcodePair {
    const char* gp;
    const char* sse;
};

class RegisterTracker {
public:
    RegisterPair alloc(int index = 0);

    void free(Register reg);

private:
    std::unordered_set<Register> registersInUse;
    static constexpr char* stringRepFromReg[22] = {
            "rax", "rbx", "rcx",
            "rdx", "rdi", "rsi",
            "r8", "r9", "r10",
            "r11", "r12", "r13",
            "r14", "r15",
            "xmm0", "xmm1", "xmm2",
            "xmm3", "xmm4", "xmm5",
            "xmm6", "xmm7"
    };
};

class CodeGen {
public:
    std::string emit(const ExprPtr& ast);

private:
    void emitAST(const ExprPtr& ast);

    void emitNumb(const ExprPtr& n, RegisterPair& rp);

    void emitRHS(const ExprPtr& rhs, RegisterPair& rp);

    void emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, OpcodePair op, RegisterPair& rp);

    void emitBinop(const BinOpExpr& binop, RegisterPair& rp);

    void emitDotimes(const DotimesExpr& dotimes);

    void emitLoop(const LoopExpr& loop);

    void emitLet(const LetExpr& let);

    void emitSetq(const SetqExpr& setq);

    void emitDefvar(const DefvarExpr& defvar);

    void emitDefconst(const DefconstExpr& defconst);

    void emitDefun(const DefunExpr& defun);

    void emitFuncCall(const FuncCallExpr& funcCall);

    void emitIf(const IfExpr& if_);

    void emitWhen(const WhenExpr& when);

    void emitCond(const CondExpr& cond);

    RegisterTracker rtracker;
    int stackOffset{8};
    std::string generatedCode;
    std::unordered_map<std::string, int> stackOffsets;
    std::unordered_map<std::string, std::string> sectionData;
    std::unordered_set<std::string> sectionBSS;
};


#endif