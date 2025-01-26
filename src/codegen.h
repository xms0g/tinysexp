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

enum RegisterSize {
    REG64, REG32, REG16, REG8H, REG8L
};

enum RegisterType : uint8_t {
    SSE = 1 << 0,
    SCRATCH = 1 << 1,
    PRESERVED = 1 << 2,
    PARAM = 1 << 3,
    CHAIN = 1 << 4
};

struct RegisterPair {
    Register reg;
    const char* sreg[5];
    uint8_t rType;
};

class RegisterTracker {
public:
    RegisterPair alloc(uint8_t rtype);

    void free(Register reg);

private:
    std::unordered_set<Register> registersInUse;

    static constexpr RegisterPair registers[30] = {
            {Register::RAX, {"rax", "eax",  "ax",   "ah", "al"},   SCRATCH},
            {Register::RCX, {"rcx", "ecx",  "cx",   "ch", "cl"},   SCRATCH | PARAM},
            {Register::RDX, {"rdx", "edx",  "dx",   "dh", "dl"},   SCRATCH | PARAM},
            {Register::RDI, {"rdi", "edi",  "di",   "",   "dil"},  SCRATCH | PARAM},
            {Register::RSI, {"rsi", "esi",  "si",   "",   "sil"},  SCRATCH | PARAM},
            {Register::R8,  {"r8",  "r8d",  "r8w",  "",   "r8b"},  SCRATCH | PARAM},
            {Register::R9,  {"r9",  "r9d",  "r9w",  "",   "r9b"},  SCRATCH | PARAM},
            {Register::R10, {"r10", "r10d", "r10w", "",   "r10b"}, SCRATCH | CHAIN},
            {Register::R11, {"r11", "r11d", "r11w", "",   "r11b"}, SCRATCH},
            {Register::RBX, {"rbx", "ebx",  "bx",   "bh", "bl"},   PRESERVED},
            {Register::R12, {"r12", "r12d", "r12w", "",   "r12b"}, PRESERVED},
            {Register::R13, {"r13", "r13d", "r13w", "",   "r13b"}, PRESERVED},
            {Register::R14, {"r14", "r14d", "r14w", "",   "r14b"}, PRESERVED},
            {Register::R15, {"r15", "r15d", "r15w", "",   "r15b"}, PRESERVED},
            {Register::xmm0,  {"xmm0",  "", "", "", ""}, SSE | PARAM},
            {Register::xmm1,  {"xmm1",  "", "", "", ""}, SSE | PARAM},
            {Register::xmm2,  {"xmm2",  "", "", "", ""}, SSE | PARAM},
            {Register::xmm3,  {"xmm3",  "", "", "", ""}, SSE | PARAM},
            {Register::xmm4,  {"xmm4",  "", "", "", ""}, SSE | PARAM},
            {Register::xmm5,  {"xmm5",  "", "", "", ""}, SSE | PARAM},
            {Register::xmm6,  {"xmm6",  "", "", "", ""}, SSE | PARAM},
            {Register::xmm7,  {"xmm7",  "", "", "", ""}, SSE | PARAM},
            {Register::xmm8,  {"xmm8",  "", "", "", ""}, SSE},
            {Register::xmm9,  {"xmm9",  "", "", "", ""}, SSE},
            {Register::xmm10, {"xmm10", "", "", "", ""}, SSE},
            {Register::xmm11, {"xmm11", "", "", "", ""}, SSE},
            {Register::xmm12, {"xmm12", "", "", "", ""}, SSE},
            {Register::xmm13, {"xmm13", "", "", "", ""}, SSE},
            {Register::xmm14, {"xmm14", "", "", "", ""}, SSE},
            {Register::xmm15, {"xmm15", "", "", "", ""}, SSE},
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

    void emitTest(const ExprPtr& test, std::string& label);

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
};


#endif