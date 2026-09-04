#include "stack.hpp"

void StackAllocator::alloc(const uint32_t size) {
	mStackOffset += size;
}

void StackAllocator::dealloc(const uint32_t size) {
	mStackOffset -= size;
}

int32_t StackAllocator::pushStackFrame(const std::string_view funcName,
                                       const std::string_view varName,
                                       const SymbolType stype) {
	StackFrame* sf = nullptr;

	if (const auto it = mStack.find(funcName); it != mStack.end()) {
		sf = &it->second;

		if (const auto it2 = sf->offsets.find(varName); it2 != sf->offsets.end()) {
			return it2->second;
		}
	}

	if (!sf) {
		StackFrame stackFrame;

		const int32_t offset = updateStackFrame(&stackFrame, varName, stype);
		mStack.emplace(funcName, stackFrame);

		return offset;
	}

	return updateStackFrame(sf, varName, stype);
}

uint32_t StackAllocator::calculateRequiredStackSize(const std::vector<ExprPtr>& args) const {
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

	uint32_t alignedSize = stackParamCount * 8;

	if (alignedSize % 16 != 0)
		alignedSize += 8;

	return alignedSize;
}

int StackAllocator::updateStackFrame(StackFrame* sf, const std::string_view varName, const SymbolType stype) {
	int32_t offset;

	if (stype == SymbolType::local) {
		offset = sf->currentVarOffset;
		sf->currentVarOffset += 8;
	} else {
		offset = sf->currentParamOffset;
		sf->currentParamOffset += 8;
	}

	sf->offsets.emplace(varName, offset);

	return offset;
}
