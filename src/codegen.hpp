#pragma once
#include <string>
#include <unordered_map>
#include <format>
#include "parser.hpp"
#include "stack.hpp"
#include "register.hpp"
#include "macros.hpp"

class CodeGen {
public:
	CodeGen();

	std::string emit(const ExprPtr& ast);

private:
	Register* emitAST(const ExprPtr& ast, bool discardResult);

	Register* emitBinop(const BinOpExpr& binop);

	Register* emitDotimes(const DotimesExpr& dotimes);

	Register* emitLoop(const LoopExpr& loop, bool discardResult);

	Register* emitLet(const LetExpr& let, bool discardResult);

	Register* emitSetq(const SetqExpr& setq, bool discardResult);

	Register* emitDefvar(const DefvarExpr& defvar);

	Register* emitDefconst(const DefconstExpr& defconst);

	void emitDefun(const DefunExpr& defun);

	Register* emitPrint(const PrintExpr& print);

	Register* emitRead(const ReadExpr& read);

	Register* emitFuncCall(const FuncCallExpr& funcCall);

	Register* emitIf(const IfExpr& if_, bool discardResult);

	Register* emitWhen(const WhenExpr& when, bool discardResult);

	Register* emitCond(const CondExpr& cond, bool discardResult);

	Register* emitPrimitive(const ExprPtr& prim);

	Register* emitInt(const IntExpr& int_);

	Register* emitDouble(DoubleExpr& double_);

	Register* emitNumb(const ExprPtr& n);

	Register* emitNode(const ExprPtr& node);

	struct OpcodePair {
		std::string_view op;
		std::string_view opSSE;
	};

	Register* emitExpr(const ExprPtr& lhs, const ExprPtr& rhs, OpcodePair opcode);

	Register* emitSection(const ExprPtr& var, bool isConstant = false, bool discardResult = true);

	void emitTest(const ExprPtr& test, std::string_view trueLabel, std::string_view elseLabel);

	void emitJmpTrueLabel(const Register* reg, TokenType type, std::string_view label);

	Register* emitSet(const ExprPtr& set);

	Register* emitLogOp(const BinOpExpr& binop, std::string_view op);

	Register* emitSetReg(const BinOpExpr& binop);

	Register* emitCmpZero(const ExprPtr& node);

	Register* emitAssignment(const ExprPtr& var, RegisterSize size, bool discardResult);

	Register* emitLoadRegFromMem(const VarExpr& var, RegisterSize size);

	void emitStoreMemFromReg(std::string_view varName,
	                         VarType vtype,
	                         SymbolType stype,
	                         const Register* reg,
	                         RegisterSize size);

	std::string getAddr(std::string_view varName, VarType vtype, SymbolType stype, RegisterSize size);

	RegisterSize getMemSize(const ExprPtr& var);

	template<typename T>
	void pushParamToRegister(RegisterID rid, VarType vtype, InitType itype, const T& value);

	void pushParamOntoStack(std::string_view funcName, const ExprPtr& param);

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
	std::unordered_map<std::string, std::vector<Section> > mSections;

	// Functions
	struct Function {
		void (CodeGen::* func)(const DefunExpr&);

		const DefunExpr& defun;
	};

	std::vector<Function> mFunctions;

	static constexpr std::string_view mMemorySize[RegisterAllocator::SIZE_COUNT] = {
		"qword", "dword", "word", "byte", "byte"
	};

	static constexpr std::string_view mDataSizeInitialized[RegisterAllocator::SIZE_COUNT] = {
		"dq", "dd", "dw", "db", "db"
	};

	static constexpr std::string_view mDataSizeUninitialized[RegisterAllocator::SIZE_COUNT] = {
		"resq", "resd", "resw", "resb", "resb"
	};

	static constexpr int32_t mMemorySizeInBytes[RegisterAllocator::SIZE_COUNT] = {8, 4, 2, 1, 1};

	static constexpr RegisterID mParamRegisters[] = {
		RegisterID::rdi, RegisterID::rsi, RegisterID::rdx, RegisterID::rcx, RegisterID::r8, RegisterID::r9
	};

	static constexpr RegisterID mParamRegistersSSE[] = {
		RegisterID::xmm0, RegisterID::xmm1, RegisterID::xmm2, RegisterID::xmm3, RegisterID::xmm4, RegisterID::xmm5,
		RegisterID::xmm6, RegisterID::xmm7
	};
};

#include "codegen.tpp"
