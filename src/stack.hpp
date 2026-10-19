#pragma once
#include <string>
#include <unordered_map>
#include "parser.hpp"
#include "register.hpp"
#include "string_util.hpp"

class StackAllocator {
public:
	void alloc(uint32_t size);

	void dealloc(uint32_t size);

	int pushStackFrame(std::string_view funcName, std::string_view varName, SymbolType stype, int32_t size);

	[[nodiscard]]
	uint32_t calculateCallStackSize(const std::vector<ExprPtr>& args) const;

private:
	struct StackFrame;
	static int32_t updateStackFrame(StackFrame* sf, std::string_view varName, SymbolType stype, int32_t size);

	struct StackFrame {
		int32_t currentVarOffset{0};
		int32_t currentParamOffset{8};
		std::unordered_map<std::string, int32_t, StringHash, StringEqual> offsets;
	};

	std::unordered_map<std::string, StackFrame, StringHash, StringEqual> mStack{};
	uint32_t mStackOffset{8};
};
