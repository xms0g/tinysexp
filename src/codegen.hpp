#pragma once
#include <string>
#include <unordered_map>
#include <format>
#include "parser.hpp"
#include "stack.hpp"
#include "register.hpp"

#define emitHex(n) std::format("0x{:X}", n)
#define emitLabel(label) mGeneratedCode += std::format("{}:\n", label)
#define emitInstr1op(op, d) mGeneratedCode += std::format("\t{} {}\n", op, d)
#define emitInstr2op(op, d, s) mGeneratedCode += std::format("\t{} {}, {}\n", op, d, s)
#define emitJump(jmp, label) emitInstr1op(jmp, label)
#define ret() mGeneratedCode += "\tret\n"
#define cqo() mGeneratedCode += "\tcqo\n"
#define syscall() mGeneratedCode += "\tsyscall\n"
#define lea(d, s) emitInstr2op("lea", d, s)
#define mov(d, s) emitInstr2op("mov", d, s)
#define movq(d, s) emitInstr2op("movq", d, s)
#define movsd(d, s) emitInstr2op("movsd", d, s)
#define movzx(d, s) emitInstr2op("movzx", d, s)
#define strDirective(s) std::format("db \"{}\", 10, 0", s)
#define alignDirective(a) std::format("align {}", a)
#define memDirective(d, n) std::format("{} {}", d, n)

#define stackAlloc(size) \
    if (size > 0) { \
        emitInstr2op("sub", "rsp", size); \
        mStackAllocator.alloc(size); \
    }

#define stackDealloc(size) \
    if (size > 0) { \
        emitInstr2op("add", "rsp", size); \
        mStackAllocator.dealloc(size); \
    }

#define push(v) \
    emitInstr1op("push", v); \
    mStackAllocator.alloc(8);

#define pop(rn) \
    emitInstr1op("pop", rn); \
    mStackAllocator.dealloc(8);

#define emitSet8L(op, reg) \
    emitInstr1op(op, mRegisterAllocator.nameFromReg(reg, RegisterSize::reg8l)); \
    movzx(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64), mRegisterAllocator.nameFromReg(reg, RegisterSize::reg8l));

#define regAlloc() ([&]() { \
    auto* reg = mRegisterAllocator.alloc(RegisterType::scratch); \
    if (reg && reg->isPreserved()) { \
        push(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64)) \
    } \
    return reg; \
    }())

#define regFree(reg) \
    if (reg) { \
        mRegisterAllocator.free(reg); \
        if (reg->isPreserved()) { \
            pop(mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64)) \
        } \
    }


class CodeGen {
public:
    CodeGen();

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

	void emitPrint(const PrintExpr& print);

    Register* emitFuncCall(const FuncCallExpr& funcCall);

    Register* emitIf(const IfExpr& if_);

    Register* emitWhen(const WhenExpr& when);

    Register* emitCond(const CondExpr& cond);

    Register* emitPrimitive(const ExprPtr& prim);

    Register* emitInt(const IntExpr& int_);

    Register* emitDouble(const DoubleExpr& double_);

    Register* emitNumb(const ExprPtr& n);

    Register* emitNode(const ExprPtr& node);

	struct OpcodePair {
		std::string_view op;
		std::string_view opSSE;
	};

    Register* emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, OpcodePair opcode);

    void emitSection(const ExprPtr& var, bool isConstant = false);

    void emitTest(const ExprPtr& test, std::string_view trueLabel, std::string_view elseLabel);

    void emitJmpTrueLabel(const Register* reg, TokenType type, std::string_view label);

    Register* emitSet(const ExprPtr& set);

    Register* emitLogOp(const BinOpExpr& binop, std::string_view op);

    Register* emitSetReg(const BinOpExpr& binop);

    Register* emitCmpZero(const ExprPtr& node);

    void handleAssignment(const ExprPtr& var, RegisterSize size);

    void handleVariable(const VarExpr& var, RegisterSize size);

    Register* emitLoadRegFromMem(const VarExpr& var, RegisterSize size);

    void emitStoreMemFromReg(std::string_view varName, VarType vtype, SymbolType stype, const Register* reg, RegisterSize size);

    std::string getAddr(std::string_view varName, VarType vtype, SymbolType stype, RegisterSize size);

    RegisterSize getMemSize(const ExprPtr& var);

	template <typename T>
    void pushParamToRegister(RegisterID rid, VarType vtype, InitType itype, const T& value);

    void pushParamOntoStack(std::string_view funcName, const VarExpr& param, int32_t& stackIdx);

    std::string createLabel();

	struct Section {
		std::string name;
		std::string data;
	};

    void updateSections(std::string_view sectionName, Section section);

    static bool isPrimitive(const ExprPtr& var);

    std::string mGeneratedCode;
    // Label
    int32_t mCurrentLabelCount{0};
    // Scope
    std::string mCurrentScope;
    // Register
    RegisterAllocator mRegisterAllocator;
    // Stack
    StackAllocator mStackAllocator;
    // Sections
    std::unordered_map<std::string, std::vector<Section>> mSections;
    // Functions
	struct Function {
		void(CodeGen::*func)(const DefunExpr&);
		const DefunExpr& defun;
	};
    std::vector<Function> mFunctions;

    static constexpr std::string_view mMemorySize[RegisterAllocator::SIZE_COUNT] = {"qword", "dword", "word", "byte", "byte"};

    static constexpr std::string_view mDataSizeInitialized[RegisterAllocator::SIZE_COUNT] = {"dq", "dd", "dw", "db", "db"};

    static constexpr std::string_view mDataSizeUninitialized[RegisterAllocator::SIZE_COUNT] = {"resq", "resd", "resw", "resb", "resb"};

    static constexpr int32_t mMemorySizeInBytes[RegisterAllocator::SIZE_COUNT] = {8, 4, 2, 1, 1};

    static constexpr RegisterID mParamRegisters[] = {RegisterID::rdi, RegisterID::rsi, RegisterID::rdx, RegisterID::rcx, RegisterID::r8, RegisterID::r9};

    static constexpr RegisterID mParamRegistersSSE[] = {RegisterID::xmm0, RegisterID::xmm1, RegisterID::xmm2, RegisterID::xmm3, RegisterID::xmm4, RegisterID::xmm5, RegisterID::xmm6, RegisterID::xmm7};
};

#include "codegen.tpp"
