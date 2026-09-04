#include "semantic.hpp"
#include "exceptions.hpp"

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

void ScopeTracker::bind(const std::string_view name, const Symbol& symbol) {
	if (lookup(name).value) {
		update(name, symbol);
	} else {
		auto currentScope = mSymbolTable.top();
		mSymbolTable.pop();

		currentScope.emplace(name, symbol);
		mSymbolTable.push(currentScope);
	}
}

void ScopeTracker::update(const std::string_view name, Symbol symbol) {
	std::stack<ScopeType> scopes;

	while (!mSymbolTable.empty()) {
		ScopeType scope = mSymbolTable.top();
		mSymbolTable.pop();

		if (auto it = scope.find(name); it != scope.end()) {
			it->second = std::move(symbol);
			scopes.push(scope);
			break;
		}
		// If the symbol is not found, push the scope back to the stack
		// and continue searching in the next scope
		scopes.push(scope);
	}

	// reconstruct the scopes
	while (!scopes.empty()) {
		mSymbolTable.push(scopes.top());
		scopes.pop();
	}
}

Symbol ScopeTracker::lookup(const std::string_view name) {
	Symbol sym{};
	std::stack<ScopeType> scopes;

	while (!mSymbolTable.empty()) {
		ScopeType scope = mSymbolTable.top();
		mSymbolTable.pop();
		scopes.push(scope);

		if (const auto it = scope.find(name); it != scope.end()) {
			sym = it->second;
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

Symbol ScopeTracker::lookupCurrent(const std::string_view name) {
	ScopeType currentScope = mSymbolTable.top();

	if (const auto it = currentScope.find(name); it != currentScope.end()) {
		return it->second;
	}

	return {};
}

SemanticAnalyzer::SemanticAnalyzer(const std::string_view fn)
	: mFileName(fn) {
}

void SemanticAnalyzer::analyze(const ExprPtr& ast) {
	auto next = ast;

	mSymbolTracker.enter("global");
	while (next != nullptr) {
		exprResolve(next);
		next = next->child;
	}
	mSymbolTracker.exit();
}

ExprPtr SemanticAnalyzer::exprResolve(const ExprPtr& ast) {
	if (const auto binop = cast::toBinop(ast)) {
		return binopResolve(*binop);
	}
	if (const auto dotimes = cast::toDotimes(ast)) {
		return dotimesResolve(*dotimes);
	}
	if (const auto loop = cast::toLoop(ast)) {
		return loopResolve(*loop);
	}
	if (const auto let = cast::toLet(ast)) {
		return letResolve(*let);
	}
	if (const auto setq = cast::toSetq(ast)) {
		return setqResolve(*setq);
	}
	if (const auto defvar = cast::toDefvar(ast)) {
		defvarResolve(*defvar);
	} else if (const auto defconst = cast::toDefconstant(ast)) {
		defconstResolve(*defconst);
	} else if (cast::toDefun(ast)) {
		return defunResolve(ast);
	} else if (const auto print = cast::toPrint(ast)) {
		printResolve(ast);
	} else if (const auto funcCall = cast::toFuncCall(ast)) {
		return funcCallResolve(*funcCall);
	} else if (const auto return_ = cast::toReturn(ast)) {
		returnResolve(*return_);
	} else if (const auto if_ = cast::toIf(ast)) {
		return ifResolve(*if_);
	} else if (const auto when = cast::toWhen(ast)) {
		return whenResolve(*when);
	} else if (const auto cond = cast::toCond(ast)) {
		return condResolve(*cond);
	} else if (cast::toInt(ast) || cast::toDouble(ast) || cast::toVar(ast)) {
		if (cast::toVar(ast)) {
			return varResolve(const_cast<ExprPtr&>(ast), TokenType::var);
		}

		return ast;
	}

	return nullptr;
}

ExprPtr SemanticAnalyzer::binopResolve(BinOpExpr& binop) {
	ExprPtr lhs = nodeResolve(binop.lhs, binop.opToken.type);
	ExprPtr rhs = nodeResolve(binop.rhs, binop.opToken.type);

	if (cast::toDouble(lhs)) {
		return lhs;
	}

	if (cast::toDouble(rhs)) {
		return rhs;
	}

	return lhs;
}

ExprPtr SemanticAnalyzer::dotimesResolve(const DotimesExpr& dotimes) {
	mSymbolTracker.enter("");
	checkConstantVar(dotimes.iterationCount);

	const auto var = cast::toVar(dotimes.iterationCount);
	// Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
	// If it's expr, resolve it.
	valueResolve(var);

	ExprPtr result;
	for (const auto& statement: dotimes.statements) {
		result = exprResolve(statement);
	}
	mSymbolTracker.exit();

	return result;
}

ExprPtr SemanticAnalyzer::loopResolve(const LoopExpr& loop) {
	ExprPtr result;

	for (const auto& sexpr: loop.sexprs) {
		result = exprResolve(sexpr);
	}

	return result;
}

ExprPtr SemanticAnalyzer::letResolve(const LetExpr& let) {
	mSymbolTracker.enter("");
	for (const auto& var: let.bindings) {
		const auto var_ = cast::toVar(var);
		const std::string_view varName = cast::toString(var_->name)->data;

		// Check out the var in the current scope, if it's already defined, raise error
		if (const Symbol sym = mSymbolTracker.lookupCurrent(varName); sym.value) {
			throw SemanticError(mFileName, ERROR(MULTIPLE_DECL_ERROR, varName), 0);
		}

		// Check the value.If it's another var, look up all scopes.If it's not defined, raise error.
		// If it's expr, resolve it.
		valueResolve(var_);
	}

	ExprPtr result;
	for (const auto& statement: let.body) {
		result = exprResolve(statement);
	}

	mSymbolTracker.exit();

	return result;
}

ExprPtr SemanticAnalyzer::setqResolve(const SetqExpr& setq) {
	checkConstantVar(setq.pair);

	const auto var = cast::toVar(setq.pair);
	const std::string_view varName = cast::toString(var->name)->data;

	// Check out the var.If it's not defined, raise error.
	const Symbol sym = mSymbolTracker.lookup(varName);

	if (!sym.value) {
		throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, varName), 0);
	}
	// Resolve the var scope.
	var->sType = sym.sType;
	// Check out the value of var.If it's another var, look up all scopes.If it's not defined, raise error.
	// If it's int or double, update sym->value and bind again.
	// If it's expr, resolve it.
	return valueResolve(var);
}

void SemanticAnalyzer::defvarResolve(const DefvarExpr& defvar) {
	const auto var = cast::toVar(defvar.pair);
	const std::string_view varName = cast::toString(var->name)->data;

	if (mSymbolTracker.level() > 1) {
		throw SemanticError(mFileName, ERROR(GLOBAL_VAR_DECL_ERROR, varName), 0);
	}

	valueResolve(var);
}

void SemanticAnalyzer::defconstResolve(const DefconstExpr& defconst) {
	const auto var = cast::toVar(defconst.pair);
	const std::string_view varName = cast::toString(var->name)->data;

	if (mSymbolTracker.level() > 1) {
		throw SemanticError(mFileName, ERROR(CONSTANT_VAR_DECL_ERROR, varName), 0);
	}

	valueResolve(var, true);
}

ExprPtr SemanticAnalyzer::defunResolve(const ExprPtr& defun) {
	const auto func = cast::toDefun(defun);
	const auto var = cast::toVar(func->name);
	const std::string funcName = cast::toString(var->name)->data;

	mSymbolTracker.bind(funcName, {.name = funcName, .value = defun, .sType = SymbolType::global});

	mSymbolTracker.enter(funcName);
	for (const auto& arg: func->args) {
		const auto argVar = cast::toVar(arg);
		const std::string argName = cast::toString(argVar->name)->data;
		mSymbolTracker.bind(argName, {.name = argName, .value = arg, .sType = argVar->sType});
	}

	ExprPtr result;
	for (const auto& statement: func->forms) {
		result = exprResolve(statement);
	}
	mSymbolTracker.exit(true);

	return result;
}

void SemanticAnalyzer::printResolve(const ExprPtr& print) {
	const auto print_ = cast::toPrint(print);

	if (const auto arg = cast::toVar(print_->arg)) {
		const std::string_view argName = cast::toString(arg->name)->data;

		if (const Symbol sym = mSymbolTracker.lookup(argName); !sym.value) {
			throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, argName), 0);
		}
	} else if (cast::toInt(print_->arg)) {
		ExprPtr name = std::make_shared<StringExpr>("print_int_var");
		print_->arg = std::make_shared<VarExpr>(name, print_->arg);
		cast::toVar(print_->arg)->vType = VarType::int_;
	} else if (cast::toDouble(print_->arg)) {
		ExprPtr name = std::make_shared<StringExpr>("print_double_var");
		print_->arg = std::make_shared<VarExpr>(name, print_->arg);
		cast::toVar(print_->arg)->vType = VarType::double_;
	} else if (cast::toString(print_->arg)) {
		ExprPtr name = std::make_shared<StringExpr>("print_string_var");
		print_->arg = std::make_shared<VarExpr>(name, print_->arg);
		cast::toVar(print_->arg)->vType = VarType::string;
	} else {
		ExprPtr expr = exprResolve(print_->arg);
		print_->returnType = std::move(expr);
	}
}

ExprPtr SemanticAnalyzer::funcCallResolve(FuncCallExpr& funcCall, bool isParam) {
	const auto var = cast::toVar(funcCall.name);
	const std::string_view funcName = cast::toString(var->name)->data;

	if (!isParam && mSymbolTracker.level() == 1) {
		mTfCtx.isStarted = true;
		mTfCtx.entryPoint = funcName;
	}

	Symbol sym = mSymbolTracker.lookup(funcName);

	if (!sym.value || !cast::toDefun(sym.value)) {
		throw SemanticError(mFileName, ERROR(FUNC_UNDEFINED_ERROR, funcName), 0);
	}

	const auto func = cast::toDefun(sym.value);

	if (funcCall.args.size() != func->args.size()) {
		throw SemanticError(mFileName, ERROR(FUNC_INVALID_NUMBER_OF_ARGS_ERROR, funcName, funcCall.args.size()), 0);
	}

	// Match the param names to values
	if (!funcCall.args.empty()) {
		const auto fcarg = funcCall.args[0];
		const auto fcargVar = cast::toVar(fcarg);

		if ((fcargVar && cast::toUninitialized(fcargVar->value)) || isPrimitive(fcarg) || cast::toBinop(fcarg) ||
		    cast::toFuncCall(fcarg)) {
			for (size_t i = 0; i < func->args.size(); ++i) {
				const auto fArg = cast::toVar(func->args[i]);

				ExprPtr name = fArg->name;
				ExprPtr value = funcCall.args[i];

				funcCall.args[i] = std::make_shared<VarExpr>(name, value, fArg->sType);
			}
		}
	}
	// Type Resolution Phase
	for (const auto& arg: funcCall.args) {
		auto argVar = cast::toVar(arg);

		if (isPrimitive(argVar->value)) {
			setType(*argVar, argVar->value);
		} else if (auto binop = cast::toBinop(argVar->value)) {
			auto value = binopResolve(*binop);
			setType(*argVar, value);
		} else if (auto fc = cast::toFuncCall(argVar->value)) {
			auto value = funcCallResolve(*fc, true);
			setType(*argVar, value);
		} else if (auto innerVar = cast::toVar(argVar->value)) {
			bool found{false};

			do {
				const std::string_view innerVarName = cast::toString(innerVar->name)->data;

				sym = mSymbolTracker.lookup(innerVarName);

				if (sym.value) {
					const auto sym_value = cast::toVar(sym.value);
					innerVar->value = sym_value->value;
					innerVar->sType = sym_value->sType;
					// Loop sym value until finding a primitive. Update var.
					if (isPrimitive(sym_value->value)) {
						setType(*innerVar, sym_value->value);
						setType(*argVar, sym_value->value);
						break;
					}

					if (auto innerValue = cast::toVar(sym_value->value)) {
						do {
							if (isPrimitive(innerValue->value)) {
								setType(*innerVar, innerValue->value);
								setType(*argVar, innerValue->value);
								found = true;
								break;
							}
							innerValue = cast::toVar(innerValue->value);
						} while (innerValue);
					}
				}
				if (found)
					break;
				innerVar = cast::toVar(innerVar->value);
			} while (innerVar);
		}
	}
	// Make the arg type local because we'll keep them onto stack inside the function
	int32_t scratchIdx{0};
	int32_t sseIdx{0};
	auto makeLocal = [&](VarExpr& arg) {
		// The params beyond 6 for scratch and beyond 7 for SSE are already onto stack
		if (arg.vType == VarType::int_ && scratchIdx < 6) {
			arg.sType = SymbolType::local;
			scratchIdx++;
		} else if (arg.vType == VarType::double_ && sseIdx < 8) {
			arg.sType = SymbolType::local;
			sseIdx++;
		}
	};

	if (mTfCtx.isStarted) {
		for (size_t i = 0; i < funcCall.args.size(); ++i) {
			auto arg = cast::toVar(funcCall.args[i]);
			makeLocal(*arg);
			func->args[i] = funcCall.args[i];
		}
		// Find the proper type of variables and the return type of the function
		if (const auto currentScope = mSymbolTracker.scopeName(); currentScope != funcName) {
			funcCall.returnType = defunResolve(func);

			if (funcName == mTfCtx.entryPoint)
				mTfCtx.isStarted = false;
		}
	}

	return funcCall.returnType;
}

void SemanticAnalyzer::returnResolve(const ReturnExpr& return_) {
	if (cast::toT(return_.arg) || cast::toNIL(return_.arg))
		return;

	const auto arg = cast::toVar(return_.arg);
	const std::string_view argName = cast::toString(arg->name)->data;

	// Check out the var.If it's not defined, raise error.
	if (const Symbol sym = mSymbolTracker.lookup(argName); !sym.value) {
		throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, argName), 0);
	}
}

ExprPtr SemanticAnalyzer::ifResolve(IfExpr& if_) {
	if (const auto test = cast::toVar(if_.test)) {
		const std::string_view name = cast::toString(test->name)->data;

		const Symbol sym = mSymbolTracker.lookup(name);
		if (!sym.value) {
			throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
		}

		if_.test = sym.value;
	} else {
		exprResolve(if_.test);
	}

	ExprPtr result = exprResolve(if_.then);

	if (!cast::toUninitialized(if_.else_)) {
		result = exprResolve(if_.else_);
	}

	return result;
}

ExprPtr SemanticAnalyzer::whenResolve(WhenExpr& when) {
	if (const auto test = cast::toVar(when.test)) {
		const std::string_view name = cast::toString(test->name)->data;

		const Symbol sym = mSymbolTracker.lookup(name);
		if (!sym.value) {
			throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
		}

		when.test = sym.value;
	} else {
		exprResolve(when.test);
	}

	ExprPtr result;
	for (const auto& form: when.then) {
		result = exprResolve(form);
	}

	return result;
}

ExprPtr SemanticAnalyzer::condResolve(CondExpr& cond) {
	ExprPtr result;

	for (auto& [test, statements]: cond.variants) {
		if (const auto test_ = cast::toVar(test)) {
			const std::string_view name = cast::toString(test_->name)->data;

			const Symbol sym = mSymbolTracker.lookup(name);
			if (!sym.value) {
				throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
			}

			test = sym.value;
		} else {
			exprResolve(test);
		}

		for (const auto& statement: statements) {
			result = exprResolve(statement);
		}
	}

	return result;
}

void SemanticAnalyzer::checkConstantVar(const ExprPtr& var) {
	const auto var_ = cast::toVar(var);
	const std::string_view varName = cast::toString(var_->name)->data;

	if (const Symbol sym = mSymbolTracker.lookup(varName); sym.isConstant) {
		throw SemanticError(mFileName, ERROR(CONSTANT_VAR_ERROR, varName), 0);
	}
}

void SemanticAnalyzer::checkBool(const ExprPtr& var, const TokenType ttype) const {
	if (ttype == TokenType::and_ || ttype == TokenType::or_ || ttype == TokenType::not_)
		return;

	if (cast::toT(var)) {
		throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "t"), 0);
	}

	if (cast::toNIL(var)) {
		throw SemanticError(mFileName, ERROR(NOT_NUMBER_ERROR, "nil"), 0);
	}
}

void SemanticAnalyzer::checkBitwiseOp(const ExprPtr& n, const TokenType ttype) const {
	if (ttype == TokenType::logand ||
	    ttype == TokenType::logior ||
	    ttype == TokenType::logxor ||
	    ttype == TokenType::lognor) {
		throw SemanticError(mFileName, ERROR(NOT_INT_ERROR, std::get<double>(getValue(n))), 0);
	}
}

std::variant<int, double> SemanticAnalyzer::getValue(const ExprPtr& num) const {
	auto getPrimitive = [&](const ExprPtr& n) -> std::variant<int, double> {
		if (const auto double_ = cast::toDouble(n))
			return double_->n;

		if (const auto int_ = cast::toInt(n)) {
			return int_->n;
		}

		return {};
	};

	if (cast::toInt(num) || cast::toDouble(num))
		return getPrimitive(num);

	if (const auto var = cast::toVar(num)) {
		return getPrimitive(var->value);
	}

	return {};
}

ExprPtr SemanticAnalyzer::returnValue(const VarExpr& var) {
	if (var.vType == VarType::int_) {
		return std::make_shared<IntExpr>(0);
	}

	if (var.vType == VarType::double_) {
		return std::make_shared<DoubleExpr>(0.0);
	}

	if (var.vType == VarType::string) {
		return std::make_shared<StringExpr>();
	}

	if (var.vType == VarType::nil) {
		return std::make_shared<NILExpr>();
	}

	if (var.vType == VarType::t) {
		return std::make_shared<TExpr>();
	}

	return nullptr;
}

ExprPtr SemanticAnalyzer::varResolve(ExprPtr& n, const TokenType ttype) {
	if (isPrimitive(n) || cast::toUninitialized(n)) {
		if (cast::toDouble(n)) {
			checkBitwiseOp(n, ttype);
		}

		return n;
	}

	// If var is t/nil and token type is different from and/or/not raise error.
	checkBool(n, ttype);

	const auto var = cast::toVar(n);
	const std::string_view name = cast::toString(var->name)->data;

	const Symbol sym = mSymbolTracker.lookup(name);

	if (!sym.value) {
		throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
	}

	var->sType = sym.sType;

	auto innerVar = cast::toVar(sym.value);

	// If we already know the type, return it.
	if (innerVar->vType != VarType::unknown) {
		var->vType = innerVar->vType;
		var->value = innerVar->value;
		return returnValue(*innerVar);
	}
	// Loop sym value until finding a primitive. Update var.
	do {
		checkBool(innerVar->value, ttype);

		if (isPrimitive(innerVar->value)) {
			if (innerVar->vType == VarType::double_) {
				checkBitwiseOp(innerVar->value, ttype);
			}

			ExprPtr value_;
			if (innerVar->vType == VarType::int_) {
				value_ = std::make_shared<IntExpr>(0);
			} else if (innerVar->vType == VarType::double_) {
				value_ = std::make_shared<DoubleExpr>(0.0);
			}
			var->value = value_;
			var->vType = innerVar->vType;
			return value_;
		}

		if (const auto binop = cast::toBinop(innerVar->value)) {
			const auto value = binopResolve(*binop);
			setType(*var, value);
			var->value = innerVar->value;
			return returnValue(*var);
		}

		if (const auto fc = cast::toFuncCall(innerVar->value)) {
			const auto value = funcCallResolve(*fc);
			setType(*var, value);
			var->value = innerVar->value;
			return returnValue(*var);
		}
		// If the value is param
		if (cast::toUninitialized(innerVar->value)) {
			var->value = std::make_shared<DoubleExpr>(0.0);
			return var->value;
		}

		innerVar = cast::toVar(innerVar->value);
	} while (innerVar);

	if (!innerVar) {
		throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, name), 0);
	}

	return nullptr;
}

ExprPtr SemanticAnalyzer::nodeResolve(ExprPtr& n, const TokenType ttype) {
	if (const auto binop = cast::toBinop(n)) {
		return binopResolve(*binop);
	}

	if (const auto funcCall = cast::toFuncCall(n)) {
		return funcCallResolve(*funcCall);
	}

	return varResolve(n, ttype);
}

ExprPtr SemanticAnalyzer::valueResolve(const ExprPtr& var, const bool isConstant) {
	const auto var_ = cast::toVar(var);
	const std::string varName = cast::toString(var_->name)->data;

	if (isPrimitive(var_->value) || cast::toUninitialized(var_->value)) {
		setType(*var_, var_->value);

		mSymbolTracker.bind(varName, {
			                    .name = varName,
			                    .value = var,
			                    .sType = var_->sType,
			                    .isConstant = isConstant
		                    });
		return var_->value;
	}

	if (const auto value = cast::toVar(var_->value)) {
		const std::string_view valueName = cast::toString(value->name)->data;
		const Symbol sym = mSymbolTracker.lookup(valueName);

		if (!sym.value) {
			throw SemanticError(mFileName, ERROR(UNBOUND_VAR_ERROR, varName), 0);
		}
		// Update value
		var_->value = sym.value;
		var_->vType = cast::toVar(sym.value)->vType;

		mSymbolTracker.bind(varName, {
			                    .name = varName,
			                    .value = var,
			                    .sType = var_->sType,
			                    .isConstant = isConstant
		                    });
		return var_->value;
	}

	ExprPtr name = var_->name;
	ExprPtr value_ = exprResolve(var_->value);
	var_->vType = cast::toInt(value_) ? VarType::int_ : VarType::double_;

	mSymbolTracker.bind(varName, {
		                    .name = varName,
		                    .value = var,
		                    .sType = var_->sType,
		                    .isConstant = isConstant
	                    });
	return value_;
}

bool SemanticAnalyzer::isPrimitive(const ExprPtr& var) {
	return cast::toInt(var) ||
	       cast::toDouble(var) ||
	       cast::toNIL(var) ||
	       cast::toT(var) ||
	       cast::toString(var);
}

void SemanticAnalyzer::setType(VarExpr& var, const ExprPtr& value) {
	if (cast::toInt(value)) {
		var.vType = VarType::int_;
	} else if (cast::toDouble(value)) {
		var.vType = VarType::double_;
	} else if (cast::toString(value)) {
		var.vType = VarType::string;
	} else if (cast::toT(value)) {
		var.vType = VarType::t;
	} else if (cast::toNIL(value)) {
		var.vType = VarType::nil;
	}
}
