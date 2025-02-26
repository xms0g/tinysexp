#include "register.h"
#include <cstring>

Register* RegisterAllocator::alloc(uint8_t rt) {
    if (rt == SSE) {
        return scan(priorityOrderSSE, 2);
    }

    return scan(priorityOrder, 3);
}

void RegisterAllocator::free(Register* reg) {
    reg->status = NO_USE;
}

const char* RegisterAllocator::nameFromReg(const Register* reg, uint32_t size) {
    return registerNames[reg->id][size];
}

const char* RegisterAllocator::nameFromID(uint32_t id, uint32_t size) {
    return registerNames[id][size];
}

Register* RegisterAllocator::regFromName(const char* name, uint32_t size) {
    for (int i = 0; i < REGISTER_COUNT; i++) {
        if (std::strcmp(name, registerNames[i][size]) == 0) {
            return &registers[i];
        }
    }

    return nullptr;
}

Register* RegisterAllocator::regFromID(uint32_t id) {
    return &registers[id];
}

Register* RegisterAllocator::scan(const uint32_t* priorityOrder, int size) {
    for (int i = 0; i < size; ++i) {
        for (auto& register_: registers) {
            if (priorityOrder[i] == register_.rType && register_.status >> NO_USE_IDX & 1) {
                register_.status &= ~NO_USE;
                register_.status |= INUSE;
                return &register_;
            }
        }
    }

    return nullptr;
}
