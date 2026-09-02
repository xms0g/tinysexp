#include "stack.hpp"

void StackAllocator::alloc(const uint32_t size) {
	mStackOffset += size;
}

void StackAllocator::dealloc(const uint32_t size) {
	mStackOffset -= size;
}

int32_t StackAllocator::pushStackFrame(const std::string_view funcName, const std::string_view varName, const SymbolType stype) {
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
	int32_t sseCount = 0;
	int32_t stackParamCount = 0;

	for (const auto& arg: args) {
		const auto param = cast::toVar(arg);
		sseCount += param->vType == VarType::double_;
	}

	if (args.size() > 6) {
		if (args.size() == sseCount) {
			stackParamCount = static_cast<int32_t>(args.size()) - 8;
		} else {
			stackParamCount = static_cast<int32_t>(args.size()) - 6 - sseCount;
		}
	}

	uint32_t alignedSize = mStackOffset + (stackParamCount > 0 ? stackParamCount * 8 : 0);

	if (alignedSize % 16 != 0)
		alignedSize += 8;

	return alignedSize - mStackOffset;
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
