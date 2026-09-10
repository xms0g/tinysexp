#include "stack.hpp"

#include "register.hpp"

void StackAllocator::alloc(const uint32_t size) {
	mStackOffset += size;
}

void StackAllocator::dealloc(const uint32_t size) {
	mStackOffset -= size;
}

int32_t StackAllocator::pushStackFrame(const std::string_view funcName,
                                       const std::string_view varName,
                                       const SymbolType stype,
                                       const int32_t size) {
	StackFrame* sf = nullptr;

	if (const auto it = mStack.find(funcName); it != mStack.end()) {
		sf = &it->second;

		if (const auto it2 = sf->offsets.find(varName); it2 != sf->offsets.end()) {
			return it2->second;
		}
	}

	if (!sf) {
		StackFrame stackFrame;

		const int32_t offset = updateStackFrame(&stackFrame, varName, stype, size);
		mStack.emplace(funcName, stackFrame);

		return offset;
	}

	return updateStackFrame(sf, varName, stype, size);
}

uint32_t StackAllocator::calculateCallStackSize(const std::vector<ExprPtr>& args) const {
	int32_t sseRegCount{0};
	int32_t intRegCount{0};
	int32_t stackParamCount{0};

	for (const auto& arg: args) {
		const auto param = cast::toVar(arg);

		if (!param)
			continue;

		if (param->vType == VarType::double_) {
			if (sseRegCount < 8)
				++sseRegCount;
			else
				++stackParamCount;
		} else if (param->vType == VarType::int_) {
			if (intRegCount < 6)
				++intRegCount;
			else
				++stackParamCount;
		}
	}

	const uint32_t argSize = stackParamCount * 8;
	uint32_t total = mStackOffset + argSize;

	if (total % 16 != 0)
		total += 8;

	return total - mStackOffset;
}

int StackAllocator::updateStackFrame(StackFrame* sf, const std::string_view varName, const SymbolType stype, const int32_t size) {
	int32_t offset;

	if (stype == SymbolType::local) {
		sf->currentVarOffset += size;
		offset = sf->currentVarOffset;
	} else {
		sf->currentParamOffset += size;
		offset = sf->currentParamOffset;
	}

	sf->offsets.emplace(varName, offset);

	return offset;
}
