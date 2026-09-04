#include "register.hpp"

bool Register::isInuse() const {
	return (status & 1) != 0;
}

bool Register::isSSE() const {
	return rType == (RegisterType::sse | RegisterType::param);
}

bool Register::isScratch() const {
	return rType == (RegisterType::scratch | RegisterType::param);
}

bool Register::isPreserved() const {
	return rType == RegisterType::preserved;
}

Register* RegisterAllocator::alloc(const RegisterType rt) {
    if (rt == RegisterType::sse) {
        return scan(mPriorityOrderSSE, 2);
    }

    return scan(mPriorityOrder, 3);
}

void RegisterAllocator::free(Register* reg) {
    reg->status &= ~1;
}

std::string_view RegisterAllocator::nameFromReg(const Register* reg, const RegisterSize size) {
    return mRegisterNames[std::to_underlying(reg->id)][std::to_underlying(size)];
}

std::string_view RegisterAllocator::nameFromID(const RegisterID id, const RegisterSize size) {
    return mRegisterNames[std::to_underlying(id)][std::to_underlying(size)];
}

Register* RegisterAllocator::regFromID(const RegisterID id) {
    return &mRegisters[std::to_underlying(id)];
}

Register* RegisterAllocator::scan(const std::span<const RegisterType> order, const int32_t size) {
    for (int i = 0; i < size; ++i) {
        for (auto& reg: mRegisters) {
            if (order[i] == reg.rType && !reg.isInuse()) {
                reg.status |= 1;
                return &reg;
            }
        }
    }

    return nullptr;
}
