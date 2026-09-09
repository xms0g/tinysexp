#pragma once
#include <stack>
#include <string>
#include <unordered_map>
#include "parser.hpp"

struct Symbol {
	std::string name;
	ExprPtr value;
	SymbolType sType;
	bool isConstant{};
};

class ScopeTracker {
public:
	void enter(std::string_view scopeName);

	void exit(bool isFunc = false);

	[[nodiscard]]
	std::string_view scopeName();

	[[nodiscard]]
	size_t level() const;

	void bind(std::string_view name, Symbol symbol);

	Symbol* lookup(std::string_view name);

	const Symbol* lookupCurrent(std::string_view name);

private:
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

	using ScopeType = std::unordered_map<std::string, Symbol, StringHash, StringEqual>;
	std::stack<ScopeType> mSymbolTable;
	std::stack<std::string> mScopeNames;
};
