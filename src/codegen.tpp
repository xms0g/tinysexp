#pragma once

template<typename T>
void CodeGen::pushParamToRegister(const RegisterID rid, const VarType vtype, const InitType itype, const T& value) {
	const Register* reg = mRegisterAllocator.regFromID(rid);
	auto regStr = mRegisterAllocator.nameFromReg(reg, RegisterSize::reg64);

	if constexpr (std::is_same_v<T, int>) {
		mov(regStr, value);
	} else if constexpr (std::is_same_v<T, double>) {
		auto n = value;
		uint64_t hex = *reinterpret_cast<uint64_t*>(&n);

		Register* regScr = regAlloc();
		auto regScrStr = mRegisterAllocator.nameFromReg(regScr, RegisterSize::reg64);

		mov(regScrStr, emitHex(hex));
		movq(regStr, regScrStr);
		regFree(regScr)
	} else if constexpr (std::is_same_v<T, const char *>) {
		if (reg->isSSE()) {
			movsd(regStr, value);
		} else {
			if (vtype == VarType::string) {
				switch (itype) {
					case InitType::constant:
						lea(regStr, value);
						break;
					case InitType::runtime:
						mov(regStr, value);
						break;
					case InitType::unknown:
						break;
				}
			} else {
				mov(regStr, value);
			}
		}
	}
}
