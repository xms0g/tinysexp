#include "lexer.hpp"
#include <cstring>
#include "exceptions.hpp"

Lexer::Lexer(const std::string_view fn, std::string text)
	: mText(std::move(text)),
	  mPos(-1, 0, -1),
	  mFileName(fn) {
	advance();
}

void Lexer::process() {
	while (mCurrentChar) {
		if (mCurrentChar[0] == '\t' || mCurrentChar[0] == '\n' || std::isspace(mCurrentChar[0])) {
			advance();
		} else if (!std::strncmp("dotimes", mCurrentChar, 7)) {
			mTokens.emplace_back(TokenType::dotimes);
			advance(7);
		} else if (!std::strncmp("return", mCurrentChar, 6)) {
			mTokens.emplace_back(TokenType::return_);
			advance(6);
		} else if (!std::strncmp("loop", mCurrentChar, 4)) {
			mTokens.emplace_back(TokenType::loop);
			advance(4);
		} else if (!std::strncmp("let", mCurrentChar, 3)) {
			mTokens.emplace_back(TokenType::let);
			advance(3);
		} else if (!std::strncmp("setq", mCurrentChar, 4)) {
			mTokens.emplace_back(TokenType::setq);
			advance(4);
		} else if (!std::strncmp("if", mCurrentChar, 2)) {
			mTokens.emplace_back(TokenType::if_);
			advance(2);
		} else if (!std::strncmp("when", mCurrentChar, 4)) {
			mTokens.emplace_back(TokenType::when);
			advance(4);
		} else if (!std::strncmp("cond", mCurrentChar, 4)) {
			mTokens.emplace_back(TokenType::cond);
			advance(4);
		} else if (!std::strncmp("defvar", mCurrentChar, 4)) {
			mTokens.emplace_back(TokenType::defvar);
			advance(6);
		} else if (!std::strncmp("defconstant", mCurrentChar, 11)) {
			mTokens.emplace_back(TokenType::defconst);
			advance(11);
		} else if (!std::strncmp("defun", mCurrentChar, 5)) {
			mTokens.emplace_back(TokenType::defun);
			advance(5);
		} else if (!std::strncmp("nil", mCurrentChar, 3)) {
			mTokens.emplace_back(TokenType::nil);
			advance(3);
		} else if (!std::strncmp("logand", mCurrentChar, 6)) {
			mTokens.emplace_back(TokenType::logand);
			advance(6);
		} else if (!std::strncmp("logior", mCurrentChar, 6)) {
			mTokens.emplace_back(TokenType::logior);
			advance(6);
		} else if (!std::strncmp("logxor", mCurrentChar, 6)) {
			mTokens.emplace_back(TokenType::logxor);
			advance(6);
		} else if (!std::strncmp("lognor", mCurrentChar, 6)) {
			mTokens.emplace_back(TokenType::lognor);
			advance(6);
		} else if (!std::strncmp("and", mCurrentChar, 3)) {
			mTokens.emplace_back(TokenType::and_);
			advance(3);
		} else if (!std::strncmp("or", mCurrentChar, 2)) {
			mTokens.emplace_back(TokenType::or_);
			advance(2);
		} else if (!std::strncmp("not", mCurrentChar, 3)) {
			mTokens.emplace_back(TokenType::not_);
			advance(3);
		} else if ((mCurrentChar[0] == 't' && mCurrentChar[1] == ' ') ||
		           (mCurrentChar[0] == 't' && mCurrentChar[1] == ')')) {
			mTokens.emplace_back(TokenType::t);
			advance();
		} else if (std::isalpha(mCurrentChar[0])) {
			std::string token;

			while (mCurrentChar && (std::isalnum(mCurrentChar[0]) || mCurrentChar[0] == '_' || mCurrentChar[0] == '-')) {
				token += mCurrentChar[0];
				advance();
			}

			mTokens.emplace_back(TokenType::var, token);
		} else if (std::isdigit(mCurrentChar[0])) {
			std::string token;
			bool isDouble{false};

			while (mCurrentChar && (std::isalnum(mCurrentChar[0]) || mCurrentChar[0] == '.')) {
				if (std::isalpha(mCurrentChar[0]))
					throw IllegalCharError(mFileName, (token + mCurrentChar[0]).c_str(), mPos.lineNumber);

				if (!isDouble && mCurrentChar[0] == '.') isDouble = true;

				token += mCurrentChar[0];
				advance();
			}

			mTokens.emplace_back(isDouble ? TokenType::double_ : TokenType::int_, token);
		} else if (mCurrentChar[0] == '"') {
			std::string data;

			advance();
			while (mCurrentChar && mCurrentChar[0] != '"') {
				data += mCurrentChar[0];
				advance();
			}
			advance();
			mTokens.emplace_back(TokenType::string, data);
		} else if (!std::strncmp("/=", mCurrentChar, 2)) {
			mTokens.emplace_back(TokenType::nequal);
			advance(2);
		} else if (!std::strncmp(">=", mCurrentChar, 2)) {
			mTokens.emplace_back(TokenType::greaterThenEq);
			advance(2);
		} else if (!std::strncmp("<=", mCurrentChar, 2)) {
			mTokens.emplace_back(TokenType::lessThenEq);
			advance(2);
		} else if (mCurrentChar[0] == '+') {
			mTokens.emplace_back(TokenType::plus);
			advance();
		} else if (mCurrentChar[0] == '-') {
			mTokens.emplace_back(TokenType::minus);
			advance();
		} else if (mCurrentChar[0] == '*') {
			mTokens.emplace_back(TokenType::mul);
			advance();
		} else if (mCurrentChar[0] == '/') {
			mTokens.emplace_back(TokenType::div);
			advance();
		} else if (mCurrentChar[0] == '=') {
			mTokens.emplace_back(TokenType::equal);
			advance();
		} else if (mCurrentChar[0] == '>') {
			mTokens.emplace_back(TokenType::greaterThen);
			advance();
		} else if (mCurrentChar[0] == '<') {
			mTokens.emplace_back(TokenType::lessThen);
			advance();
		} else if (mCurrentChar[0] == '(') {
			mTokens.emplace_back(TokenType::lparen);
			advance();
		} else if (mCurrentChar[0] == ')') {
			mTokens.emplace_back(TokenType::rparen);
			advance();
		} else {
			throw IllegalCharError(mFileName, std::string(1, mCurrentChar[0]), mPos.lineNumber);
		}
	}

	mTokens.emplace_back(TokenType::eof);
}

size_t Lexer::getTokenSize() const {
	return mTokens.size();
}

Token Lexer::getToken(const int32_t index) {
	return mTokens[index];
}

void Lexer::advance() {
	mPos.advance(mCurrentChar);

	if (mPos.index < mText.size()) {
		mCurrentChar = &mText[mPos.index];
	} else {
		mCurrentChar = nullptr;
	}
}

void Lexer::advance(const int step) {
	for (int i = 0; i < step; ++i) advance();
}
