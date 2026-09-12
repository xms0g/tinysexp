#pragma once
#include <utility>
#include <memory>
#include "lexer.hpp"

enum class SymbolType {
	unknown,
	local,
	param,
	global
};

enum class VarType {
	unknown,
	int_,
	double_,
	string,
	nil,
	t
};

enum class InitType {
	unknown,
	constant,
	runtime
};

struct IExpr {
	std::shared_ptr<IExpr> child;

	virtual ~IExpr() = default;
};

using ExprPtr = std::shared_ptr<IExpr>;

struct IntExpr final : IExpr {
	int n;

	explicit IntExpr(const int n_)
		: n(n_) {
	}
};

struct DoubleExpr final : IExpr {
	double n;

	explicit DoubleExpr(const double n_)
		: n(n_) {
	}
};

struct StringExpr final : IExpr {
	std::string data;

	StringExpr() = default;

	explicit StringExpr(std::string str)
		: data(std::move(str)) {
	}
};

struct NILExpr final : IExpr {
	const bool value{false};

	NILExpr() = default;
};

struct TExpr final : IExpr {
	const bool value{true};

	TExpr() = default;
};

struct BinOpExpr final : IExpr {
	ExprPtr lhs;
	ExprPtr rhs;
	Token opToken;

	BinOpExpr(ExprPtr lhs_, ExprPtr rhs_, Token opTok)
		: lhs(std::move(lhs_)),
		  rhs(std::move(rhs_)),
		  opToken(std::move(opTok)) {
	}
};

struct DotimesExpr final : IExpr {
	ExprPtr countForm;
	ExprPtr resultForm;
	std::vector<ExprPtr> statements;

	DotimesExpr(ExprPtr cf, ExprPtr rf, std::vector<ExprPtr> statements_)
		: countForm(std::move(cf)),
		  resultForm(std::move(rf)),
		  statements(std::move(statements_)) {
	}
};

struct LoopExpr final : IExpr {
	std::vector<ExprPtr> sexprs;

	explicit LoopExpr(std::vector<ExprPtr> sexprs_)
		: sexprs(std::move(sexprs_)) {
	}
};

struct LetExpr final : IExpr {
	std::vector<ExprPtr> bindings;
	std::vector<ExprPtr> body;

	LetExpr(std::vector<ExprPtr> bindings_, std::vector<ExprPtr> body_)
		: bindings(std::move(bindings_)),
		  body(std::move(body_)) {
	}
};

struct PairExpr {
	ExprPtr pair;

	PairExpr() = default;

	explicit PairExpr(ExprPtr pair_)
		: pair(std::move(pair_)) {
	}
};

struct SetqExpr final : PairExpr, IExpr {
	explicit SetqExpr(ExprPtr pair)
		: PairExpr(std::move(pair)) {
	}
};

struct DefvarExpr final : PairExpr, IExpr {
	explicit DefvarExpr(ExprPtr pair)
		: PairExpr(std::move(pair)) {
	}
};

struct DefconstExpr final : PairExpr, IExpr {
	explicit DefconstExpr(ExprPtr pair)
		: PairExpr(std::move(pair)) {
	}
};

struct DefunExpr final : IExpr {
	ExprPtr name;
	std::vector<ExprPtr> args;
	std::vector<ExprPtr> forms;

	DefunExpr(ExprPtr name_, std::vector<ExprPtr> params_, std::vector<ExprPtr> body_)
		: name(std::move(name_)),
		  args(std::move(params_)),
		  forms(std::move(body_)) {
	}
};

struct PrintExpr final : IExpr {
	ExprPtr arg;
	ExprPtr returnType;

	explicit PrintExpr(ExprPtr arg_)
		: arg(std::move(arg_)) {
	}
};

struct ReadExpr final : IExpr {
	ExprPtr returnType;

	explicit ReadExpr(ExprPtr rt)
		: returnType(std::move(rt)) {
	}
};

struct FuncCallExpr final : IExpr {
	ExprPtr name;
	ExprPtr returnType;
	std::vector<ExprPtr> args;

	FuncCallExpr(ExprPtr name_, std::vector<ExprPtr> params_)
		: name(std::move(name_)),
		  args(std::move(params_)) {
	}
};

struct ReturnExpr final : IExpr {
	ExprPtr arg;

	explicit ReturnExpr(ExprPtr arg_)
		: arg(std::move(arg_)) {
	}
};

struct IfExpr final : IExpr {
	ExprPtr test, then, else_;

	IfExpr(ExprPtr test_, ExprPtr then_, ExprPtr e = nullptr)
		: test(std::move(test_)),
		  then(std::move(then_)),
		  else_(std::move(e)) {
	}
};

struct WhenExpr final : IExpr {
	ExprPtr test;
	std::vector<ExprPtr> then;

	WhenExpr(ExprPtr test_, std::vector<ExprPtr> then_)
		: test(std::move(test_)),
		  then(std::move(then_)) {
	}
};

struct CondExpr final : IExpr {
	std::vector<std::pair<ExprPtr, std::vector<ExprPtr> > > variants;

	explicit CondExpr(std::vector<std::pair<ExprPtr, std::vector<ExprPtr> > > variants_)
		: variants(std::move(variants_)) {
	}
};

struct VarExpr final : IExpr {
	ExprPtr name;
	ExprPtr value;
	SymbolType sType;
	VarType vType{};
	InitType iType{};

	VarExpr(ExprPtr name_, ExprPtr value_, const SymbolType type = SymbolType::unknown)
		: name(std::move(name_)),
		  value(std::move(value_)),
		  sType(type) {
	}
};

struct Uninitialized final : IExpr {
};

class Parser {
public:
	Parser(std::string_view fn, Lexer& lexer);

	ExprPtr parse();

private:
	Token advance();

	ExprPtr parseExpr();

	ExprPtr parseSExpr();

	ExprPtr parseDotimes();

	ExprPtr parseLoop();

	ExprPtr parseLet();

	ExprPtr parseSetq();

	ExprPtr parseDefvar();

	ExprPtr parseDefconst();

	ExprPtr parseDefun();

	ExprPtr parsePrint();

	ExprPtr parseRead();

	ExprPtr parseFuncCall();

	ExprPtr parseReturn();

	ExprPtr parseIf();

	ExprPtr parseWhen();

	ExprPtr parseCond();

	ExprPtr parseAtom();

	ExprPtr parseNumber();

	ExprPtr createVar(SymbolType type, bool isConstant = false);

	void consume(TokenType expected, std::string_view errorStr);

	void expect(TokenType expected, std::string_view errorStr) const;

	Lexer& mLexer;
	Token mCurrentToken{};
	int32_t mTokenIndex;
	std::string_view mFileName;
};

namespace cast {
inline auto toBinop(const ExprPtr& expr) {
	return dynamic_cast<BinOpExpr*>(expr.get());
}

inline auto toDotimes(const ExprPtr& expr) {
	return dynamic_cast<DotimesExpr*>(expr.get());
}

inline auto toLoop(const ExprPtr& expr) {
	return dynamic_cast<LoopExpr*>(expr.get());
}

inline auto toLet(const ExprPtr& expr) {
	return dynamic_cast<LetExpr*>(expr.get());
}

inline auto toSetq(const ExprPtr& expr) {
	return dynamic_cast<SetqExpr*>(expr.get());
}

inline auto toDefvar(const ExprPtr& expr) {
	return dynamic_cast<DefvarExpr*>(expr.get());
}

inline auto toDefconstant(const ExprPtr& expr) {
	return dynamic_cast<DefconstExpr*>(expr.get());
}

inline auto toDefun(const ExprPtr& expr) {
	return dynamic_cast<DefunExpr*>(expr.get());
}

inline auto toPrint(const ExprPtr& expr) {
	return dynamic_cast<PrintExpr*>(expr.get());
}

inline auto toRead(const ExprPtr& expr) {
	return dynamic_cast<ReadExpr*>(expr.get());
}

inline auto toFuncCall(const ExprPtr& expr) {
	return dynamic_cast<FuncCallExpr*>(expr.get());
}

inline auto toReturn(const ExprPtr& expr) {
	return dynamic_cast<ReturnExpr*>(expr.get());
}

inline auto toIf(const ExprPtr& expr) {
	return dynamic_cast<IfExpr*>(expr.get());
}

inline auto toWhen(const ExprPtr& expr) {
	return dynamic_cast<WhenExpr*>(expr.get());
}

inline auto toCond(const ExprPtr& expr) {
	return dynamic_cast<CondExpr*>(expr.get());
}

inline auto toVar(const ExprPtr& expr) {
	return dynamic_cast<VarExpr*>(expr.get());
}

inline auto toString(const ExprPtr& expr) {
	return dynamic_cast<StringExpr*>(expr.get());
}

inline auto toInt(const ExprPtr& expr) {
	return dynamic_cast<IntExpr*>(expr.get());
}

inline auto toDouble(const ExprPtr& expr) {
	return dynamic_cast<DoubleExpr*>(expr.get());
}

inline auto toT(const ExprPtr& expr) {
	return dynamic_cast<TExpr*>(expr.get());
}

inline auto toNIL(const ExprPtr& expr) {
	return dynamic_cast<NILExpr*>(expr.get());
}

inline auto toUninitialized(const ExprPtr& expr) {
	return dynamic_cast<Uninitialized*>(expr.get());
}
}
