#ifndef CODEGEN_H
#define CODEGEN_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include "parser.h"
#include "stack.h"
#include "register.h"

class CodeGen {
public:
    CodeGen() : currentScope("main") {
    }

    std::string emit(const ExprPtr& ast);

private:
    Register* emitAST(const ExprPtr& ast);

    Register* emitBinop(const BinOpExpr& binop);

    Register* emitDotimes(const DotimesExpr& dotimes);

    Register* emitLoop(const LoopExpr& loop);

    Register* emitLet(const LetExpr& let);

    void emitSetq(const SetqExpr& setq);

    void emitDefvar(const DefvarExpr& defvar);

    void emitDefconst(const DefconstExpr& defconst);

    void emitDefun(const DefunExpr& defun);

    Register* emitFuncCall(const FuncCallExpr& funcCall);

    Register* emitIf(const IfExpr& if_);

    Register* emitWhen(const WhenExpr& when);

    Register* emitCond(const CondExpr& cond);

    Register* emitNumb(const ExprPtr& n);

    Register* emitNode(const ExprPtr& node);

    Register* emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, std::pair<const char*, const char*> op);

    void emitSection(const ExprPtr& value);

    void emitTest(const ExprPtr& test, std::string& trueLabel, std::string& elseLabel);

    Register* emitSet(const ExprPtr& set);

    Register* emitLogOp(const BinOpExpr& binop, const char* op);

    Register* emitSetReg(const BinOpExpr& binop);

    Register* emitCmpZero(const ExprPtr& node);

    void handleAssignment(const ExprPtr& var, uint32_t size);

    void handleVariable(const VarExpr& var, uint32_t size);

    Register* emitLoadRegFromMem(const VarExpr& var, uint32_t size);

    void emitStoreMemFromReg(const std::string& varName, SymbolType stype, const Register* reg, uint32_t size);

    std::string getAddr(const std::string& varName, SymbolType stype, uint32_t size);

    uint32_t getMemSize(const ExprPtr& var);

    template<typename T>
    void pushParamToRegister(const std::string& paramName, uint32_t rid, T value);

    void pushParamOntoStack(VarExpr& param, int& stackIdx);

    const char* getRegName(const Register* reg, uint32_t size);

    const char* getRegNameByID(uint32_t id, uint32_t size);

    std::string createLabel();

    void updateSections(const char* name, const std::pair<std::string, std::string>& data);

    std::string generatedCode;
    // Label
    int currentLabelCount{0};
    // Scope
    std::string currentScope;
    // Register
    RegisterAllocator registerAllocator;
    // Stack
    StackAllocator stackAllocator;
    // Sections
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string> > > sections;
    // Functions
    std::unordered_set<std::string> calledFunctions;
    std::vector<std::pair<void(CodeGen::*)(const DefunExpr&), const DefunExpr&> > functions;

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

    static constexpr int paramRegistersSSE[] = {xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7};
};

#endif
