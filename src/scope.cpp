#include "scope.hpp"

void ScopeTracker::enter(const std::string_view scopeName) {
	const std::unordered_map<std::string, Symbol, StringHash, StringEqual> scope;
	mSymbolTable.push(scope);

	if (!scopeName.empty()) {
		mScopeNames.emplace(scopeName);
	}
}

void ScopeTracker::exit(const bool isFunc) {
	mSymbolTable.pop();

	if (isFunc) {
		mScopeNames.pop();
	}
}

std::string_view ScopeTracker::scopeName() {
	return mScopeNames.top();
}

size_t ScopeTracker::level() const {
	return mSymbolTable.size();
}

void ScopeTracker::bind(const std::string_view name, Symbol symbol) {
	if (Symbol* foundSymbol = lookup(name)) {
		if (!cast::toDefun(foundSymbol->value)) {
			const auto var = cast::toVar(foundSymbol->value);
			var->vType = cast::toVar(symbol.value)->vType;
		} else {
			*foundSymbol = std::move(symbol);
		}
	} else {
		auto currentScope = mSymbolTable.top();
		mSymbolTable.pop();

		currentScope.emplace(name, symbol);
		mSymbolTable.push(currentScope);
	}
}

Symbol* ScopeTracker::lookup(const std::string_view name) {
	Symbol* sym{nullptr};
	std::stack<ScopeType> scopes;

	while (!mSymbolTable.empty()) {
		ScopeType scope = mSymbolTable.top();
		mSymbolTable.pop();
		scopes.push(scope);

		if (const auto it = scope.find(name); it != scope.end()) {
			sym = &it->second;
			break;
		}
	}
	// reconstruct the scopes
	while (!scopes.empty()) {
		mSymbolTable.push(scopes.top());
		scopes.pop();
	}

	return sym;
}

const Symbol* ScopeTracker::lookupCurrent(const std::string_view name) {
	ScopeType currentScope = mSymbolTable.top();

	if (const auto it = currentScope.find(name); it != currentScope.end()) {
		return &it->second;
	}

	return nullptr;
}
