#include "parser.hpp"
#include "exceptions.hpp"

Parser::Parser(const std::string_view fn, Lexer& lexer)
	: mLexer(lexer),
	  mTokenIndex(-1),
	  mFileName(fn) {
}

ExprPtr Parser::parse() {
	advance();

	ExprPtr root = parseExpr();
	ExprPtr prevExpr = root;

	while (mCurrentToken.type != TokenType::eof) {
		ExprPtr currentExpr = parseExpr();
		prevExpr->child = currentExpr;
		prevExpr = std::move(currentExpr);
	}
	return root;
}

Token Parser::advance() {
	++mTokenIndex;

	if (mTokenIndex < mLexer.getTokenSize()) {
		mCurrentToken = mLexer.getToken(mTokenIndex);
	}

	return mCurrentToken;
}

ExprPtr Parser::parseExpr() {
	ExprPtr expr;

	consume(TokenType::lparen, MISSING_PAREN_ERROR);
	switch (mCurrentToken.type) {
		case TokenType::plus:
		case TokenType::minus:
		case TokenType::div:
		case TokenType::mul:
		case TokenType::equal:
		case TokenType::nequal:
		case TokenType::greaterThen:
		case TokenType::lessThen:
		case TokenType::greaterThenEq:
		case TokenType::lessThenEq:
		case TokenType::and_:
		case TokenType::or_:
		case TokenType::not_:
		case TokenType::logand:
		case TokenType::logior:
		case TokenType::logxor:
		case TokenType::lognor:
			expr = parseSExpr();
			break;
		case TokenType::dotimes:
			expr = parseDotimes();
			break;
		case TokenType::loop:
			expr = parseLoop();
			break;
		case TokenType::let:
			expr = parseLet();
			break;
		case TokenType::setq:
			expr = parseSetq();
			break;
		case TokenType::defvar:
			expr = parseDefvar();
			break;
		case TokenType::defconst:
			expr = parseDefconst();
			break;
		case TokenType::defun:
			expr = parseDefun();
			break;
		case TokenType::if_:
			expr = parseIf();
			break;
		case TokenType::when:
			expr = parseWhen();
			break;
		case TokenType::cond:
			expr = parseCond();
			break;
		case TokenType::var:
			expr = parseFuncCall();
			break;
		case TokenType::return_:
			expr = parseReturn();
			break;
		default:
			throw InvalidSyntaxError(mFileName, mCurrentToken.lexeme.c_str(), 0);
	}
	consume(TokenType::rparen, MISSING_PAREN_ERROR);

	return expr;
}

ExprPtr Parser::parseSExpr() {
	ExprPtr left, right;

	Token token = mCurrentToken;
	advance();

	if (mCurrentToken.type == TokenType::lparen) {
		left = parseExpr();
	} else {
		left = parseAtom();
	}

	if (mCurrentToken.type == TokenType::lparen) {
		right = parseExpr();
	} else {
		right = parseAtom();
	}

	if (token.type == TokenType::not_ && !cast::toUninitialized(right)) {
		throw InvalidSyntaxError(mFileName, ERROR(OP_INVALID_NUMBER_OF_ARGS_ERROR, "NOT", 2), 0);
	}

	return std::make_shared<BinOpExpr>(left, right, token);
}

ExprPtr Parser::parseDotimes() {
	ExprPtr value;
	std::vector<ExprPtr> statements;

	advance();

	consume(TokenType::lparen, ERROR(EXPECTED_ELEMS_NUMBER_ERROR, "DOTIMES"));
	ExprPtr var = parseAtom();

	if (mCurrentToken.type == TokenType::lparen) {
		value = parseExpr();
	} else {
		value = parseAtom();
	}

	cast::toVar(var)->value = std::move(value);
	cast::toVar(var)->sType = SymbolType::local;
	consume(TokenType::rparen, MISSING_PAREN_ERROR);

	while (mCurrentToken.type == TokenType::lparen) {
		statements.push_back(parseExpr());
	}

	return std::make_shared<DotimesExpr>(var, statements);
}

ExprPtr Parser::parseLoop() {
	std::vector<ExprPtr> sexprs;

	advance();

	while (mCurrentToken.type == TokenType::lparen) {
		sexprs.push_back(parseExpr());
	}

	return std::make_shared<LoopExpr>(sexprs);
}

ExprPtr Parser::parseLet() {
	ExprPtr var, value;
	std::vector<ExprPtr> bindings;
	std::vector<ExprPtr> body;

	advance();

	consume(TokenType::lparen, ERROR(EXPECTED_ELEMS_NUMBER_ERROR, "LET"));
	for (;;) {
		// Check out (let (x))
		while (mCurrentToken.type == TokenType::var) {
			var = parseAtom();
			cast::toVar(var)->sType = SymbolType::local;
			bindings.push_back(var);
		}

		// Check out (let ((x 11)) )
		while (mCurrentToken.type == TokenType::lparen) {
			consume(TokenType::lparen, MISSING_PAREN_ERROR);
			var = parseAtom();

			if (mCurrentToken.type == TokenType::lparen) {
				value = parseExpr();
			} else {
				value = parseAtom();
			}

			cast::toVar(var)->value = std::move(value);
			cast::toVar(var)->sType = SymbolType::local;
			bindings.push_back(var);
			consume(TokenType::rparen, MISSING_PAREN_ERROR);
		}

		if (mCurrentToken.type == TokenType::rparen)
			break;
	}
	consume(TokenType::rparen, MISSING_PAREN_ERROR);

	while (mCurrentToken.type == TokenType::lparen) {
		body.push_back(parseExpr());
	}

	return std::make_shared<LetExpr>(bindings, body);
}

ExprPtr Parser::parseSetq() {
	ExprPtr var = createVar(SymbolType::unknown);
	return std::make_shared<SetqExpr>(var);
}

ExprPtr Parser::parseDefvar() {
	ExprPtr var = createVar(SymbolType::global);
	return std::make_shared<DefvarExpr>(var);
}

ExprPtr Parser::parseDefconst() {
	ExprPtr var = createVar(SymbolType::global, true);
	return std::make_shared<DefconstExpr>(var);
}

ExprPtr Parser::parseDefun() {
	std::vector<ExprPtr> args;
	std::vector<ExprPtr> forms;

	advance();

	ExprPtr name = parseAtom();

	// Parse params
	consume(TokenType::lparen, MISSING_PAREN_ERROR);
	while (mCurrentToken.type == TokenType::var) {
		ExprPtr arg = parseAtom();
		cast::toVar(arg)->sType = SymbolType::param;
		args.push_back(arg);
	}
	consume(TokenType::rparen, MISSING_PAREN_ERROR);
	// Parse body
	for (;;) {
		if (mCurrentToken.type == TokenType::lparen) {
			forms.push_back(parseExpr());
		} else {
			forms.push_back(parseAtom());
		}

		if (mCurrentToken.type == TokenType::rparen)
			break;
	}

	return std::make_shared<DefunExpr>(name, args, forms);
}

ExprPtr Parser::parseFuncCall() {
	std::vector<ExprPtr> args;

	ExprPtr name = parseAtom();

	for (;;) {
		if (mCurrentToken.type == TokenType::lparen) {
			args.push_back(parseExpr());
		} else {
			ExprPtr arg = parseAtom();

			if (cast::toUninitialized(arg))
				break;

			args.push_back(arg);
		}

		if (mCurrentToken.type == TokenType::rparen)
			break;
	}

	return std::make_shared<FuncCallExpr>(name, args);
}

ExprPtr Parser::parseReturn() {
	advance();

	ExprPtr arg = parseAtom();

	return std::make_shared<ReturnExpr>(arg);
}

ExprPtr Parser::parseIf() {
	ExprPtr test, then, else_;

	advance();

	if (mCurrentToken.type == TokenType::lparen) {
		test = parseExpr();
	} else {
		test = parseAtom();
	}

	if (mCurrentToken.type == TokenType::lparen) {
		then = parseExpr();
	} else {
		then = parseAtom();
	}

	if (mCurrentToken.type == TokenType::lparen) {
		else_ = parseExpr();
	} else {
		else_ = parseAtom();
	}

	return std::make_shared<IfExpr>(test, then, else_);
}

ExprPtr Parser::parseWhen() {
	ExprPtr test;
	std::vector<ExprPtr> then;

	advance();

	if (mCurrentToken.type == TokenType::lparen) {
		test = parseExpr();
	} else {
		test = parseAtom();
	}

	for (;;) {
		if (mCurrentToken.type == TokenType::lparen) {
			then.push_back(parseExpr());
		} else {
			then.push_back(parseAtom());
		}

		if (mCurrentToken.type == TokenType::rparen)
			break;
	}

	return std::make_shared<WhenExpr>(test, then);
}

ExprPtr Parser::parseCond() {
	ExprPtr test;
	std::vector<std::pair<ExprPtr, std::vector<ExprPtr> > > variants;

	advance();

	while (mCurrentToken.type == TokenType::lparen) {
		consume(TokenType::lparen, MISSING_PAREN_ERROR);

		if (mCurrentToken.type == TokenType::lparen) {
			test = parseExpr();
		} else {
			test = parseAtom();
		}

		std::vector<ExprPtr> statements;
		if (mCurrentToken.type != TokenType::lparen) {
			statements.push_back(parseAtom());
		}

		while (mCurrentToken.type == TokenType::lparen) {
			statements.push_back(parseExpr());
		}

		variants.emplace_back(test, statements);
		consume(TokenType::rparen, MISSING_PAREN_ERROR);
	}

	return std::make_shared<CondExpr>(variants);
}

ExprPtr Parser::parseAtom() {
	if (mCurrentToken.type == TokenType::string) {
		Token token = mCurrentToken;
		advance();
		return std::make_shared<StringExpr>(token.lexeme);
	}

	if (mCurrentToken.type == TokenType::var) {
		Token token = mCurrentToken;
		advance();
		ExprPtr name = std::make_shared<StringExpr>(token.lexeme);
		ExprPtr value = std::make_shared<Uninitialized>();
		return std::make_shared<VarExpr>(name, value);
	}

	if (mCurrentToken.type == TokenType::nil) {
		advance();
		return std::make_shared<NILExpr>();
	}

	if (mCurrentToken.type == TokenType::t) {
		advance();
		return std::make_shared<TExpr>();
	}

	if (mCurrentToken.type == TokenType::rparen) {
		return std::make_shared<Uninitialized>();
	}

	return parseNumber();
}

ExprPtr Parser::parseNumber() {
	const auto token = mCurrentToken;
	advance();

	if (token.type == TokenType::int_) {
		return std::make_shared<IntExpr>(std::stoi(token.lexeme));
	}
	if (token.type == TokenType::double_) {
		return std::make_shared<DoubleExpr>(std::stof(token.lexeme));
	}

	throw InvalidSyntaxError(mFileName, EXPECTED_NUMBER_ERROR, 0);
}

ExprPtr Parser::createVar(const SymbolType type, const bool isConstant) {
	ExprPtr value;
	advance();

	ExprPtr var = parseAtom();

	if (mCurrentToken.type == TokenType::lparen) {
		if (isConstant)
			throw InvalidSyntaxError(mFileName, ERROR(SEXPR_ERROR, "DEFCONSTANT"), 0);
		value = parseExpr();
	} else {
		value = parseAtom();

		if (isConstant && cast::toUninitialized(value)) {
			throw InvalidSyntaxError(mFileName, ERROR(EXPECTED_ELEMS_NUMBER_ERROR, "DEFCONSTANT"), 0);
		}
	}

	cast::toVar(var)->sType = type;
	cast::toVar(var)->value = std::move(value);

	return var;
}

void Parser::consume(const TokenType expected, const std::string_view errorStr) {
	expect(expected, errorStr);
	advance();
}

void Parser::expect(const TokenType expected, const std::string_view errorStr) const {
	if (mCurrentToken.type != expected)
		throw InvalidSyntaxError(mFileName, errorStr, 0);
}
