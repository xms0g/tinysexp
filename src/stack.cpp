#include "stack.h"

void StackAllocator::alloc(const uint32_t size) {
    stackOffset += size;
}

void StackAllocator::dealloc(const uint32_t size) {
    stackOffset -= size;
}

int StackAllocator::pushStackFrame(const std::string& funcName, const std::string& varName, const SymbolType stype) {
    StackFrame* sf = nullptr;

    if (stack.contains(funcName)) {
        sf = &stack.at(funcName);

        if (sf->offsets.contains(varName)) {
            return sf->offsets.at(varName);
        }
    }

    if (!sf) {
        StackFrame stackFrame;

        const int offset = updateStackFrame(&stackFrame, varName, stype);
        stack.emplace(funcName, stackFrame);

        return offset;
    }

    return updateStackFrame(sf, varName, stype);
}

uint32_t StackAllocator::calculateRequiredStackSize(const std::vector<ExprPtr>& args) const {
    int sseCount = 0;
    int stackParamCount = 0;

    for (const auto& arg: args) {
        const auto param = cast::toVar(arg);
        sseCount += param->vType == VarType::DOUBLE;
    }

    if (args.size() > 6) {
        if (args.size() == sseCount) {
            stackParamCount = args.size() - 8;
        } else {
            stackParamCount = args.size() - 6 - sseCount;
        }
    }

    uint32_t alignedSize = stackOffset + (stackParamCount > 0 ? stackParamCount * 8 : 0);

    if (alignedSize % 16 != 0)
        alignedSize += 8;

    return alignedSize - stackOffset;
}

int StackAllocator::updateStackFrame(StackFrame* sf, const std::string& varName, const SymbolType stype) {
    int offset;

    if (stype == SymbolType::LOCAL) {
        offset = sf->currentVarOffset;
        sf->currentVarOffset += 8;
    } else {
        offset = sf->currentParamOffset;
        sf->currentParamOffset += 8;
    }

    sf->offsets.emplace(varName, offset);

    return offset;
}
