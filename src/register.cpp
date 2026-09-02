#include "register.hpp"

Register* RegisterAllocator::alloc(const uint8_t rt) {
    if (rt == SSE) {
        return scan(mPriorityOrderSSE, 2);
    }

    return scan(mPriorityOrder, 3);
}

void RegisterAllocator::free(Register* reg) {
    reg->status &= ~INUSE;
}

const char* RegisterAllocator::nameFromReg(const Register* reg, const uint32_t size) {
    return mRegisterNames[reg->id][size];
}

const char* RegisterAllocator::nameFromID(const uint32_t id, const uint32_t size) {
    return mRegisterNames[id][size];
}

Register* RegisterAllocator::regFromID(const uint32_t id) {
    return &mRegisters[id];
}

Register* RegisterAllocator::scan(const std::span<const uint32_t> order, const int32_t size) {
    for (int i = 0; i < size; ++i) {
        for (auto& reg: mRegisters) {
            if (order[i] == reg.rType && !isINUSE(reg.status)) {
                reg.status |= INUSE;
                return &reg;
            }
        }
    }

    return nullptr;
}
