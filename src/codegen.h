#ifndef TINYSEXP_CODEGEN_H
#define TINYSEXP_CODEGEN_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include "parser.h"

enum class Register {
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
    xmm15
};

//TODO: Add Scratch, Preserved type
enum class RegisterType {
    GP, SSE
};

struct RegisterPair {
    std::pair<Register, const char*> pair;
    RegisterType rType;
};

class RegisterTracker {
public:
    RegisterPair alloc(RegisterType rtype);

    void free(Register reg);

private:
    std::unordered_set<Register> registersInUse;

    static constexpr std::pair<Register, const char*> scratchRegisters[9] = {
            {Register::RAX, "rax"},
            {Register::RCX, "rcx"},
            {Register::RDX, "rdx"},
            {Register::RDI, "rdi"},
            {Register::RSI, "rsi"},
            {Register::R8, "r8"},
            {Register::R9, "r9"},
            {Register::R10, "r10"},
            {Register::R11, "r11"},
    };

    static constexpr std::pair<Register, const char*> sseRegisters[16] = {
            {Register::xmm0, "xmm0"},
            {Register::xmm1, "xmm1"},
            {Register::xmm2, "xmm2"},
            {Register::xmm3, "xmm3"},
            {Register::xmm4, "xmm4"},
            {Register::xmm5, "xmm5"},
            {Register::xmm6, "xmm6"},
            {Register::xmm7, "xmm7"},
            {Register::xmm8, "xmm8"},
            {Register::xmm9, "xmm9"},
            {Register::xmm10, "xmm10"},
            {Register::xmm11, "xmm11"},
            {Register::xmm12, "xmm12"},
            {Register::xmm13, "xmm13"},
            {Register::xmm14, "xmm14"},
            {Register::xmm15, "xmm15"},
    };

    static constexpr std::pair<Register, const char*> preservedRegisters[5] = {
            {Register::RBX, "rbx"},
            {Register::R12, "r12"},
            {Register::R13, "r13"},
            {Register::R14, "r14"},
            {Register::R15, "r15"},

    };
};

class CodeGen {
public:
    CodeGen() : currentStackOffset(8), currentLabelCount(0) {}

    std::string emit(const ExprPtr& ast);

private:
    void emitAST(const ExprPtr& ast);

    RegisterPair emitBinop(const BinOpExpr& binop);

    void emitDotimes(const DotimesExpr& dotimes);

    void emitLoop(const LoopExpr& loop);

    void emitLet(const LetExpr& let);

    void emitSetq(const SetqExpr& setq);

    void emitDefvar(const DefvarExpr& defvar);

    void emitDefconst(const DefconstExpr& defconst);

    void emitDefun(const DefunExpr& defun);

    RegisterPair emitFuncCall(const FuncCallExpr& funcCall);

    void emitIf(const IfExpr& if_);

    void emitWhen(const WhenExpr& when);

    void emitCond(const CondExpr& cond);

    RegisterPair emitNumb(const ExprPtr& n);

    RegisterPair emitNode(const ExprPtr& node);

    RegisterPair emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op);

    void emitSection(const ExprPtr& value);

    void emitTest(const ExprPtr& test);

    RegisterPair emitSet(const ExprPtr& set);

    void handleAssignment(const ExprPtr& var);

    void handlePrimitive(const VarExpr& var, const std::string& varName, const char* instr, const std::string& value);

    void handleVariable(const VarExpr& var, const std::string& varName);

    RegisterPair emitLoadRegFromMem(const VarExpr& value, const std::string& valueName);

    void emitStoreMemFromReg(const std::string& varName, SymbolType stype, RegisterPair rp);

    std::string getAddr(SymbolType stype, const std::string& varName);

    std::string createLabel();

    std::string generatedCode;
    // Label
    int currentLabelCount;
    // Register
    RegisterTracker rtracker;
    // Stack
    int currentStackOffset;
    std::unordered_map<std::string, int> stackOffsets;
    // Sections
    std::unordered_map<std::string, std::string> sectionData;
    std::unordered_set<std::string> sectionBSS;
    // Jump instructions
    std::stack<std::string> jumps;
};


#endif