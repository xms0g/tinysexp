#ifndef TINYSEXP_CODEGEN_H
#define TINYSEXP_CODEGEN_H

#include <string>
#include <unordered_map>
#include "parser.h"

static constexpr int REGISTER_COUNT = 32;
static constexpr int SIZE_COUNT = 5;

struct Register {
    uint32_t id;
    uint8_t rType;
    uint8_t status;
};

enum RegisterID : uint32_t {
    RAX, RDI, RSI,
    RDX, RCX, R8,
    R9, R10, R11,
    RBP, RSP, RBX,
    R12, R13, R14, R15,
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
};

enum RegisterTypeIndex {
    SSE_IDX = 0,
    SCRATCH_IDX,
    PRESERVED_IDX,
    PARAM_IDX,
};

enum RegisterStatus : uint8_t {
    NO_USE = 1 << 0,
    INUSE_FOR_PARAM = 1 << 1,
    INUSE = 1 << 2
};

enum RegisterStatusIndex {
    NO_USE_IDX = 0,
    INUSE_FOR_PARAM_IDX,
    INUSE_IDX,
};

class RegisterAllocator {
public:
    Register* alloc(uint8_t rt = 0);

    void free(Register* reg);

    const char* nameFromReg(const Register* reg, uint32_t size);

    const char* nameFromID(uint32_t id, uint32_t size);

    Register* regFromName(const char* name, uint32_t size);

    Register* regFromID(uint32_t id);

private:
    Register* scan(const uint32_t* priorityOrder, int size);

    Register registers[REGISTER_COUNT] = {
        {RAX, SCRATCH, NO_USE},
        {RDI, SCRATCH | PARAM, NO_USE},
        {RSI, SCRATCH | PARAM, NO_USE},
        {RDX, SCRATCH | PARAM, NO_USE},
        {RCX, SCRATCH | PARAM, NO_USE},
        {R8, SCRATCH | PARAM, NO_USE},
        {R9, SCRATCH | PARAM, NO_USE},
        {R10, SCRATCH, NO_USE},
        {R11, SCRATCH, NO_USE},
        {RBP, PRESERVED, INUSE},
        {RSP, PRESERVED, INUSE},
        {RBX, PRESERVED, NO_USE},
        {R12, PRESERVED, NO_USE},
        {R13, PRESERVED, NO_USE},
        {R14, PRESERVED, NO_USE},
        {R15, PRESERVED, NO_USE},
        {xmm0, SSE | PARAM, NO_USE},
        {xmm1, SSE | PARAM, NO_USE},
        {xmm2, SSE | PARAM, NO_USE},
        {xmm3, SSE | PARAM, NO_USE},
        {xmm4, SSE | PARAM, NO_USE},
        {xmm5, SSE | PARAM, NO_USE},
        {xmm6, SSE | PARAM, NO_USE},
        {xmm7, SSE | PARAM, NO_USE},
        {xmm8, SSE, NO_USE},
        {xmm9, SSE, NO_USE},
        {xmm10, SSE, NO_USE},
        {xmm11, SSE, NO_USE},
        {xmm12, SSE, NO_USE},
        {xmm13, SSE, NO_USE},
        {xmm14, SSE, NO_USE},
        {xmm15, SSE, NO_USE},
    };

    static constexpr const char* registerNames[REGISTER_COUNT][SIZE_COUNT] = {
        {"rax", "eax", "ax", "ah", "al"},
        {"rdi", "edi", "di", "", "dil"},
        {"rsi", "esi", "si", "", "sil"},
        {"rdx", "edx", "dx", "dh", "dl"},
        {"rcx", "ecx", "cx", "ch", "cl"},
        {"r8", "r8d", "r8w", "", "r8b"},
        {"r9", "r9d", "r9w", "", "r9b"},
        {"r10", "r10d", "r10w", "", "r10b"},
        {"r11", "r11d", "r11w", "", "r11b"},
        {"rbp", "ebp", "bp", "", "bpl"},
        {"rsp", "esp", "sp", "", "spl"},
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

    static constexpr uint32_t priorityOrder[3] = {SCRATCH, SCRATCH | PARAM, PRESERVED};

    static constexpr uint32_t priorityOrderSSE[2] = {SSE | PARAM, SSE};
};

class StackAllocator {
public:
    void alloc(uint32_t size);

    void dealloc(uint32_t size);

    [[nodiscard]] uint32_t getOffset() const { return stackOffset; }

    int pushStackFrame(const std::string& funcName, const std::string& varName, SymbolType stype);

private:
    struct StackFrame {
        int currentVarOffset{8}, currentParamOffset{16};
        std::unordered_map<std::string, int> offsets;
    };

    int updateStackFrame(StackFrame* sf, const std::string& varName, SymbolType stype);

    std::unordered_map<std::string, StackFrame> stack{};
    uint32_t stackOffset{0};
};

class CodeGen {
public:
    CodeGen() : currentLabelCount(0), currentScope("main") {
    }

    std::string emit(const ExprPtr& ast);

private:
    Register* emitAST(const ExprPtr& ast);

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

    template<typename T>
    void pushParamToRegister(std::string& paramName, uint32_t rid, T value);

    void pushParamOntoStack(const VarExpr& param, int& stackIdx);

    const char* getRegName(const Register* reg, uint32_t size);

    const char* getRegNameByID(uint32_t id, uint32_t size);

    std::string createLabel();

    void updateSections(const char* name, const std::pair<std::string, std::string>& data);

    std::string generatedCode;
    // Label
    int currentLabelCount;
    // Scope
    std::string currentScope;
    // Register
    RegisterAllocator registerAllocator;
    std::unordered_map<std::string, uint32_t> paramToRegisters;
    // Stack
    StackAllocator stackAllocator;
    // Sections
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> sections;
    // Functions
    std::vector<std::pair<void(CodeGen::*)(const DefunExpr&), const DefunExpr&>> functions;

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

    static constexpr int paramRegisters[] = {RDI, RSI, RDX, RCX, R8, R9};

    static constexpr int paramRegistersSSE[] = {xmm0, xmm1, xmm2,xmm3, xmm4, xmm5, xmm6, xmm7};
};

#endif
