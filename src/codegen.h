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
    xmm6, xmm7, xmm8,
    xmm9, xmm10, xmm11,
    xmm12, xmm13, xmm14,
    xmm15, EOR
};

enum RegisterType {
    GP, SSE
};

struct RegisterPair {
    Register reg;
    const char* sreg;
    RegisterType rType;
};

class RegisterTracker {
public:
    RegisterPair alloc(int index = 0);

    void free(Register reg);

private:
    std::unordered_set<Register> registersInUse;
    static constexpr char* stringRepFromReg[30] = {
            "rax", "rbx", "rcx",
            "rdx", "rdi", "rsi",
            "r8", "r9", "r10",
            "r11", "r12", "r13",
            "r14", "r15",
            "xmm0", "xmm1", "xmm2",
            "xmm3", "xmm4", "xmm5",
            "xmm6", "xmm7", "xmm8",
            "xmm9", "xmm10", "xmm11",
            "xmm12", "xmm13", "xmm14",
            "xmm15"
    };
};

class CodeGen {
public:
    CodeGen() : currentStackOffset(8) {}

    std::string emit(const ExprPtr& ast);

private:
    void emitAST(const ExprPtr& ast);

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

    RegisterPair emitNumb(const ExprPtr& n);

    RegisterPair emitRHS(const ExprPtr& rhs);

    void emitSection(const ExprPtr& value);

    void emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op, RegisterPair& rp);

    void handleAssignment(const ExprPtr& var);

    void handlePrimitive(const VarExpr& var, const std::string& varName, const char* instr, const std::string& value);

    void handleVariable(const VarExpr& var, const std::string& varName);

    RegisterPair emitLoadInstruction(const VarExpr& value, const std::string& valueName);

    void emitStoreInstruction(const std::string& varName, const ExprPtr& value, SymbolType stype, RegisterPair reg);

    std::string getAddr(SymbolType stype, const std::string& varName);

    std::string generatedCode;
    // Register
    RegisterTracker rtracker;
    // Stack
    int currentStackOffset;
    std::unordered_map<std::string, int> stackOffsets;
    // Sections
    std::unordered_map<std::string, std::string> sectionData;
    std::unordered_set<std::string> sectionBSS;
};


#endif