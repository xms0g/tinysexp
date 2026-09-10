#pragma once
#include <string>
#include <unordered_map>
#include "parser.hpp"
#include "register.hpp"

class StackAllocator {
public:
	void alloc(uint32_t size);

	void dealloc(uint32_t size);

	int pushStackFrame(std::string_view funcName, std::string_view varName, SymbolType stype, int32_t size);

	[[nodiscard]]
	uint32_t calculateCallStackSize(const std::vector<ExprPtr>& args) const;

private:
	struct StackFrame;
	int32_t updateStackFrame(StackFrame* sf, std::string_view varName, SymbolType stype, int32_t size);

	struct StringHash {
		using is_transparent = void;

		static constexpr size_t operator()(const std::string_view value) noexcept {
			return std::hash<std::string_view>{}(value);
		}

	};

	struct StringEqual {
		using is_transparent = void;

		static constexpr bool operator()(const std::string_view lhs, const std::string_view rhs) noexcept {
			return lhs == rhs;
		}
	};

	struct StackFrame {
		int32_t currentVarOffset{0};
		int32_t currentParamOffset{8};
		std::unordered_map<std::string, int32_t, StringHash, StringEqual> offsets;
	};

	std::unordered_map<std::string, StackFrame, StringHash, StringEqual> mStack{};
	uint32_t mStackOffset{8};
};
