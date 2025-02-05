#ifndef TINYSEXP_CODEGEN_H
#define TINYSEXP_CODEGEN_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include "parser.h"

struct Register {
    uint32_t id;
    uint8_t rType;
    bool inUse;
};

enum RegisterID : uint32_t {
    RAX, RCX, RDX,
    RDI, RSI, R8,
    R9, R10, R11,
    RBX, R12, R13,
    R14, R15,
    xmm0, xmm1, xmm2,
    xmm3, xmm4, xmm5,
    xmm6, xmm7, xmm8,
    xmm9, xmm10, xmm11,
    xmm12, xmm13, xmm14,
    xmm15
};

enum RegisterSize: uint32_t {
    REG64, REG32, REG16, REG8H, REG8L
};

enum RegisterType : uint8_t {
    SSE = 1 << 0,
    SCRATCH = 1 << 1,
    PRESERVED = 1 << 2,
    PARAM = 1 << 3,
    CHAIN = 1 << 4,
    INUSE = 1 << 5
};

class RegisterTracker {
public:
    Register* alloc(uint8_t rtype);

    void free(Register* reg);

    const char* name(uint32_t id, int size);

private:
    static constexpr int REGISTER_COUNT = 30;

    Register registers[REGISTER_COUNT] = {
            {RAX,   SCRATCH,         false},
            {RCX,   SCRATCH | PARAM, false},
            {RDX,   SCRATCH | PARAM, false},
            {RDI,   SCRATCH | PARAM, false},
            {RSI,   SCRATCH | PARAM, false},
            {R8,    SCRATCH | PARAM, false},
            {R9,    SCRATCH | PARAM, false},
            {R10,   SCRATCH | CHAIN, false},
            {R11,   SCRATCH,         false},
            {RBX,   PRESERVED,       false},
            {R12,   PRESERVED,       false},
            {R13,   PRESERVED,       false},
            {R14,   PRESERVED,       false},
            {R15,   PRESERVED,       false},
            {xmm0,  SSE | PARAM,     false},
            {xmm1,  SSE | PARAM,     false},
            {xmm2,  SSE | PARAM,     false},
            {xmm3,  SSE | PARAM,     false},
            {xmm4,  SSE | PARAM,     false},
            {xmm5,  SSE | PARAM,     false},
            {xmm6,  SSE | PARAM,     false},
            {xmm7,  SSE | PARAM,     false},
            {xmm8,  SSE,             false},
            {xmm9,  SSE,             false},
            {xmm10, SSE,             false},
            {xmm11, SSE,             false},
            {xmm12, SSE,             false},
            {xmm13, SSE,             false},
            {xmm14, SSE,             false},
            {xmm15, SSE,             false},
    };

    static constexpr const char* registerNames[REGISTER_COUNT][5] = {
            {"rax",   "eax",  "ax",   "ah", "al"},
            {"rcx",   "ecx",  "cx",   "ch", "cl"},
            {"rdx",   "edx",  "dx",   "dh", "dl"},
            {"rdi",   "edi",  "di",   "",   "dil"},
            {"rsi",   "esi",  "si",   "",   "sil"},
            {"r8",    "r8d",  "r8w",  "",   "r8b"},
            {"r9",    "r9d",  "r9w",  "",   "r9b"},
            {"r10",   "r10d", "r10w", "",   "r10b"},
            {"r11",   "r11d", "r11w", "",   "r11b"},
            {"rbx",   "ebx",  "bx",   "bh", "bl"},
            {"r12",   "r12d", "r12w", "",   "r12b"},
            {"r13",   "r13d", "r13w", "",   "r13b"},
            {"r14",   "r14d", "r14w", "",   "r14b"},
            {"r15",   "r15d", "r15w", "",   "r15b"},
            {"xmm0",  "",     "",     "",   ""},
            {"xmm1",  "",     "",     "",   ""},
            {"xmm2",  "",     "",     "",   ""},
            {"xmm3",  "",     "",     "",   ""},
            {"xmm4",  "",     "",     "",   ""},
            {"xmm5",  "",     "",     "",   ""},
            {"xmm6",  "",     "",     "",   ""},
            {"xmm7",  "",     "",     "",   ""},
            {"xmm8",  "",     "",     "",   ""},
            {"xmm9",  "",     "",     "",   ""},
            {"xmm10", "",     "",     "",   ""},
            {"xmm11", "",     "",     "",   ""},
            {"xmm12", "",     "",     "",   ""},
            {"xmm13", "",     "",     "",   ""},
            {"xmm14", "",     "",     "",   ""},
            {"xmm15", "",     "",     "",   ""},
    };
};

class CodeGen {
public:
    CodeGen() : currentStackOffset(8), currentLabelCount(0) {}

    std::string emit(const ExprPtr& ast);

private:
    void emitAST(const ExprPtr& ast);

    Register* emitBinop(const BinOpExpr& binop);

    void emitDotimes(const DotimesExpr& dotimes);

    void emitLoop(const LoopExpr& loop);

    void emitLet(const LetExpr& let);

    void emitSetq(const SetqExpr& setq);

    void emitDefvar(const DefvarExpr& defvar);

    void emitDefconst(const DefconstExpr& defconst);

    void emitDefun(const DefunExpr& defun);

    Register* emitFuncCall(const FuncCallExpr& funcCall);

    void emitIf(const IfExpr& if_);

    void emitWhen(const WhenExpr& when);

    void emitCond(const CondExpr& cond);

    Register* emitNumb(const ExprPtr& n);

    Register* emitNode(const ExprPtr& node);

    Register* emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op);

    void emitSection(const ExprPtr& value);

    void emitTest(const ExprPtr& test, std::string& trueLabel, std::string& elseLabel);

    Register* emitSet(const ExprPtr& set);

    Register* emitLogAO(const BinOpExpr& binop, const char* op);

    void handleAssignment(const ExprPtr& var, uint32_t size);

    void handlePrimitive(const VarExpr& var, const std::string& varName, const char* instr, const std::string& value);

    void handleVariable(const VarExpr& var, const std::string& varName, uint32_t size);

    Register* emitLoadRegFromMem(const VarExpr& value, const std::string& valueName, uint32_t size);

    void emitStoreMemFromReg(const std::string& varName, SymbolType stype, Register* rp, uint32_t size);

    std::string getAddr(const std::string& varName, SymbolType stype, uint32_t size);

    uint32_t getMemSize(const ExprPtr& var);

    const char* getRegName(uint32_t id, uint32_t size);

    std::string createLabel();

    void updateSections(const char* name, const std::pair<std::string, std::string>& data);

    std::string generatedCode;
    // Label
    int currentLabelCount;
    // Register
    RegisterTracker rtracker;
    // Stack
    int currentStackOffset;
    std::unordered_map<std::string, int> stackOffsets;
    // Sections
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> sections;

    static constexpr const char* memorySize[5] = {
            "qword", "dword", "word", "byte", "byte"
    };

    static constexpr const char* dataSizeInitialized[5] = {
            "dq", "dd", "dw", "db", "db"
    };

    static constexpr const char* dataSizeUninitialized[5] = {
            "resq", "resd", "resw", "resb", "resb"
    };
};


#endif