#include "register.h"
#include <cstring>

Register* RegisterAllocator::alloc(const uint8_t rt) {
    if (rt == SSE) {
        return scan(priorityOrderSSE, 2);
    }

    return scan(priorityOrder, 3);
}

void RegisterAllocator::free(Register* reg) {
    reg->status &= ~INUSE;
}

const char* RegisterAllocator::nameFromReg(const Register* reg, const uint32_t size) {
    return registerNames[reg->id][size];
}

const char* RegisterAllocator::nameFromID(const uint32_t id, const uint32_t size) {
    return registerNames[id][size];
}

Register* RegisterAllocator::regFromID(const uint32_t id) {
    return &registers[id];
}

Register* RegisterAllocator::scan(const uint32_t* priorityOrder, const int size) {
    for (int i = 0; i < size; ++i) {
        for (auto& register_: registers) {
            if (priorityOrder[i] == register_.rType && !isINUSE(register_.status)) {
                register_.status |= INUSE;
                return &register_;
            }
        }
    }

    return nullptr;
}
