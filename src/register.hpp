#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

enum class RegisterID : uint32_t {
	rax, rdi, rsi,
	rdx, rcx, r8,
	r9, r10, r11,
	rbp, rsp, rbx,
	r12, r13, r14, r15,
	xmm0, xmm1, xmm2,
	xmm3, xmm4, xmm5,
	xmm6, xmm7, xmm8,
	xmm9, xmm10, xmm11,
	xmm12, xmm13, xmm14,
	xmm15
};

enum class RegisterSize: uint32_t {
	reg64, reg32, reg16, reg8h, reg8l, zero
};

enum class RegisterType : uint8_t {
	sse = 1 << 0,
	scratch = 1 << 1,
	preserved = 1 << 2,
	param = 1 << 3,
};

constexpr RegisterType operator|(const RegisterType lhs, const RegisterType rhs) {
	return static_cast<RegisterType>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

constexpr RegisterType operator&(const RegisterType lhs, const int32_t rhs) {
	return static_cast<RegisterType>(std::to_underlying(lhs) & rhs);
}

constexpr RegisterType operator>>(const RegisterType lhs, const int32_t rhs) {
	return static_cast<RegisterType>(std::to_underlying(lhs) >> rhs);
}


struct Register {
	RegisterID id;
	RegisterType rType: 4;
	uint8_t status: 1;

	[[nodiscard]]
	bool isInuse() const;

	[[nodiscard]]
	bool isSSE() const;

	[[nodiscard]]
	bool isScratch() const;

	[[nodiscard]]
	bool isPreserved() const;
};

class RegisterAllocator {
public:
	Register* alloc(RegisterType rt);

	void free(Register* reg);

	std::string_view nameFromReg(const Register* reg, RegisterSize size);

	std::string_view nameFromID(RegisterID id, RegisterSize size);

	Register* regFromID(RegisterID id);

	static constexpr int32_t REGISTER_COUNT = 32;
	static constexpr int32_t SIZE_COUNT = 5;

private:
	Register* scan(std::span<const RegisterType> order, int32_t size);

	std::array<Register, REGISTER_COUNT> mRegisters = {{
		{.id = RegisterID::rax, .rType = RegisterType::scratch, .status = 1},
		{.id = RegisterID::rdi, .rType = RegisterType::scratch | RegisterType::param, .status = 0},
		{.id = RegisterID::rsi, .rType = RegisterType::scratch | RegisterType::param, .status = 0},
		{.id = RegisterID::rdx, .rType = RegisterType::scratch | RegisterType::param, .status = 0},
		{.id = RegisterID::rcx, .rType = RegisterType::scratch | RegisterType::param, .status = 0},
		{.id = RegisterID::r8, .rType = RegisterType::scratch | RegisterType::param, .status = 0},
		{.id = RegisterID::r9, .rType = RegisterType::scratch | RegisterType::param, .status = 0},
		{.id = RegisterID::r10, .rType = RegisterType::scratch, .status = 0},
		{.id = RegisterID::r11, .rType = RegisterType::scratch, .status = 0},
		{.id = RegisterID::rbp, .rType = RegisterType::preserved, .status = 1},
		{.id = RegisterID::rsp, .rType = RegisterType::preserved, .status = 1},
		{.id = RegisterID::rbx, .rType = RegisterType::preserved, .status = 0},
		{.id = RegisterID::r12, .rType = RegisterType::preserved, .status = 0},
		{.id = RegisterID::r13, .rType = RegisterType::preserved, .status = 0},
		{.id = RegisterID::r14, .rType = RegisterType::preserved, .status = 0},
		{.id = RegisterID::r15, .rType = RegisterType::preserved, .status = 0},
		{.id = RegisterID::xmm0, .rType = RegisterType::sse | RegisterType::param, .status = 1},
		{.id = RegisterID::xmm1, .rType = RegisterType::sse | RegisterType::param, .status = 0},
		{.id = RegisterID::xmm2, .rType = RegisterType::sse | RegisterType::param, .status = 0},
		{.id = RegisterID::xmm3, .rType = RegisterType::sse | RegisterType::param, .status = 0},
		{.id = RegisterID::xmm4, .rType = RegisterType::sse | RegisterType::param, .status = 0},
		{.id = RegisterID::xmm5, .rType = RegisterType::sse | RegisterType::param, .status = 0},
		{.id = RegisterID::xmm6, .rType = RegisterType::sse | RegisterType::param, .status = 0},
		{.id = RegisterID::xmm7, .rType = RegisterType::sse | RegisterType::param, .status = 0},
		{.id = RegisterID::xmm8, .rType = RegisterType::sse, .status = 0},
		{.id = RegisterID::xmm9, .rType = RegisterType::sse, .status = 0},
		{.id = RegisterID::xmm10, .rType = RegisterType::sse, .status = 0},
		{.id = RegisterID::xmm11, .rType = RegisterType::sse, .status = 0},
		{.id = RegisterID::xmm12, .rType = RegisterType::sse, .status = 0},
		{.id = RegisterID::xmm13, .rType = RegisterType::sse, .status = 0},
		{.id = RegisterID::xmm14, .rType = RegisterType::sse, .status = 0},
		{.id = RegisterID::xmm15, .rType = RegisterType::sse, .status = 0},
	}};

	static constexpr auto mRegisterNames = std::to_array<std::array<std::string_view, SIZE_COUNT> >({
		{"rax", "eax", "ax", "ah", "al"},
		{"rdi", "edi", "di", "", "dil"},
		{"rsi", "esi", "si", "", "sil"},
		{"rdx", "edx", "dx", "dh", "dl"},
		{"rcx", "ecx", "cx", "ch", "cl"},
		{"r8", "r8d", "r8w", "", "r8b"},
		{"r9", "r9d", "r9w", "", "r9b"},
		{"r10", "r10d", "r10w", "", "r10b"},
		{"r11", "r11d", "r11w", "", "r11b"},
		{"rbp", "ebp", "bp", "", "bpl"},
		{"rsp", "esp", "sp", "", "spl"},
		{"rbx", "ebx", "bx", "bh", "bl"},
		{"r12", "r12d", "r12w", "", "r12b"},
		{"r13", "r13d", "r13w", "", "r13b"},
		{"r14", "r14d", "r14w", "", "r14b"},
		{"r15", "r15d", "r15w", "", "r15b"},
		{"xmm0", "", "", "", ""},
		{"xmm1", "", "", "", ""},
		{"xmm2", "", "", "", ""},
		{"xmm3", "", "", "", ""},
		{"xmm4", "", "", "", ""},
		{"xmm5", "", "", "", ""},
		{"xmm6", "", "", "", ""},
		{"xmm7", "", "", "", ""},
		{"xmm8", "", "", "", ""},
		{"xmm9", "", "", "", ""},
		{"xmm10", "", "", "", ""},
		{"xmm11", "", "", "", ""},
		{"xmm12", "", "", "", ""},
		{"xmm13", "", "", "", ""},
		{"xmm14", "", "", "", ""},
		{"xmm15", "", "", "", ""},
	});

	static constexpr std::array<RegisterType, 3> mPriorityOrder = {
		RegisterType::scratch | RegisterType::param, RegisterType::scratch, RegisterType::preserved
	};

	static constexpr std::array<RegisterType, 2> mPriorityOrderSSE = {
		RegisterType::sse | RegisterType::param, RegisterType::sse
	};
};
