#ifndef TINYSEXP_CODEGEN_H
#define TINYSEXP_CODEGEN_H

#include <string>
#include <unordered_map>
#include "parser.h"

static constexpr int REGISTER_COUNT = 30;
static constexpr int SIZE_COUNT = 5;

struct Register {
    uint32_t id;
    uint8_t rType;
    bool inUse;
};

enum RegisterID : uint32_t {
    RAX, RDI, RSI,
    RCX, RDX, R8,
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
    CHAIN = 1 << 4
};

class RegisterAllocator {
public:
    Register* alloc(uint8_t rtype);

    void free(Register* reg);

    [[nodiscard]] bool isInUse(const uint32_t id) const { return registers[id].inUse; }

    const char* nameFromReg(const Register* reg, int size);

    Register* regFromName(const char* name, int size);

    Register* regFromID(uint32_t id);


private:
    Register registers[REGISTER_COUNT] = {
        {RAX, SCRATCH, false},
        {RDI, SCRATCH | PARAM, false},
        {RSI, SCRATCH | PARAM, false},
        {RCX, SCRATCH | PARAM, false},
        {RDX, SCRATCH | PARAM, false},
        {R8, SCRATCH | PARAM, false},
        {R9, SCRATCH | PARAM, false},
        {R10, SCRATCH | CHAIN, false},
        {R11, SCRATCH, false},
        {RBX, PRESERVED, false},
        {R12, PRESERVED, false},
        {R13, PRESERVED, false},
        {R14, PRESERVED, false},
        {R15, PRESERVED, false},
        {xmm0, SSE | PARAM, false},
        {xmm1, SSE | PARAM, false},
        {xmm2, SSE | PARAM, false},
        {xmm3, SSE | PARAM, false},
        {xmm4, SSE | PARAM, false},
        {xmm5, SSE | PARAM, false},
        {xmm6, SSE | PARAM, false},
        {xmm7, SSE | PARAM, false},
        {xmm8, SSE, false},
        {xmm9, SSE, false},
        {xmm10, SSE, false},
        {xmm11, SSE, false},
        {xmm12, SSE, false},
        {xmm13, SSE, false},
        {xmm14, SSE, false},
        {xmm15, SSE, false},
    };

    static constexpr const char* registerNames[REGISTER_COUNT][SIZE_COUNT] = {
        {"rax", "eax", "ax", "ah", "al"},
        {"rdi", "edi", "di", "", "dil"},
        {"rsi", "esi", "si", "", "sil"},
        {"rcx", "ecx", "cx", "ch", "cl"},
        {"rdx", "edx", "dx", "dh", "dl"},
        {"r8", "r8d", "r8w", "", "r8b"},
        {"r9", "r9d", "r9w", "", "r9b"},
        {"r10", "r10d", "r10w", "", "r10b"},
        {"r11", "r11d", "r11w", "", "r11b"},
        {"rbx", "ebx", "bx", "bh", "bl"},
        {"r12", "r12d", "r12w", "", "r12b"},
        {"r13", "r13d", "r13w", "", "r13b"},
        {"r14", "r14d", "r14w", "", "r14b"},
        {"r15", "r15d", "r15w", "", "r15b"},
        {"xmm0", "", "", "", ""},
        {"xmm1", "", "", "", ""},
        {"xmm2", "", "", "", ""},
        {"xmm3", "", "", "", ""},
        {"xmm4", "", "", "", ""},
        {"xmm5", "", "", "", ""},
        {"xmm6", "", "", "", ""},
        {"xmm7", "", "", "", ""},
        {"xmm8", "", "", "", ""},
        {"xmm9", "", "", "", ""},
        {"xmm10", "", "", "", ""},
        {"xmm11", "", "", "", ""},
        {"xmm12", "", "", "", ""},
        {"xmm13", "", "", "", ""},
        {"xmm14", "", "", "", ""},
        {"xmm15", "", "", "", ""},
    };
};

class StackAllocator {
public:
    StackAllocator(): currentVarStackOffset(8), currentParamStackOffset(8) {}

    int alloc(const std::string& name, SymbolType stype);
private:
    int currentVarStackOffset, currentParamStackOffset;
    std::unordered_map<std::string, int> stackOffsets;
};

class CodeGen {
public:
    CodeGen() : currentStackOffset(8), currentLabelCount(0) {
    }

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

    Register* emitSetReg(const BinOpExpr& binop);

    void handleAssignment(const ExprPtr& var, uint32_t size);

    void handlePrimitive(const VarExpr& var, const char* instr, const std::string& value);

    void handleVariable(const VarExpr& var, uint32_t size);

    Register* emitLoadRegFromMem(const VarExpr& var, uint32_t size);

    void emitStoreMemFromReg(const std::string& varName, SymbolType stype, Register* rp, uint32_t size);

    std::string getAddr(const std::string& varName, SymbolType stype, uint32_t size);

    uint32_t getMemSize(const ExprPtr& var);

    const char* getRegName(const Register* reg, uint32_t size);

    std::string createLabel();

    void updateSections(const char* name, const std::pair<std::string, std::string>& data);

    std::string generatedCode;
    // Label
    int currentLabelCount;
    // Register
    RegisterAllocator registerAllocator;
    // Stack
    StackAllocator stackAllocator;
    // Sections
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string> > > sections;

    static constexpr const char* memorySize[SIZE_COUNT] = {
        "qword", "dword", "word", "byte", "byte"
    };

    static constexpr const char* dataSizeInitialized[SIZE_COUNT] = {
        "dq", "dd", "dw", "db", "db"
    };

    static constexpr const char* dataSizeUninitialized[SIZE_COUNT] = {
        "resq", "resd", "resw", "resb", "resb"
    };

    static constexpr int memorySizeInBytes[SIZE_COUNT] = {
        8, 4, 2, 1, 1
    };
};

#endif
