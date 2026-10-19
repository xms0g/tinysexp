#pragma once
#include <stack>
#include <string>
#include <unordered_map>
#include "parser.hpp"
#include "string_util.hpp"

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
	using ScopeType = std::unordered_map<std::string, std::shared_ptr<Symbol>, StringHash, StringEqual>;
	std::stack<ScopeType> mSymbolTable;
	std::stack<std::string> mScopeNames;
};
