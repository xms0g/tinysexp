#pragma once
#include <string>
#include <vector>

enum class TokenType {
	// Type
	int_, double_, string, var, nil, t,
	// Arithmetic Operators
	plus, minus, div, mul,
	// Comparison Operators
	equal, nequal, greaterThen, lessThen, greaterThenEq, lessThenEq,
	// Logical Operators
	and_, or_, not_,
	// Bitwise Operators
	logand, logior, logxor, lognor,
	// Loop
	dotimes, loop,
	// Condition
	if_, when, cond,
	// Assignment
	let, setq, defvar, defconst,
	// Function
	defun,
	// Special function
	return_,
	// Others
	lparen, rparen,
	eof
};

struct Token {
	TokenType type{};
	std::string lexeme;

	Token() = default;

	explicit Token(const TokenType type, std::string value = "")
		: type(type),
		  lexeme(std::move(value)) {
	}
};

struct Position {
	int32_t index;
	int32_t lineNumber;
	int32_t columnNumber;

	Position(const int32_t idx, const int32_t ln, const int32_t coln)
		: index(idx),
		  lineNumber(ln),
		  columnNumber(coln) {
	}

	void advance(const char* token) {
		++index;
		++columnNumber;

		if (token && token[0] == '\n') {
			++lineNumber;
			columnNumber = 0;
		}
	}
};

class Lexer {
public:
	Lexer(std::string_view fn, std::string text);

	void process();

	[[nodiscard]]
	size_t getTokenSize() const;

	Token getToken(int32_t index);

private:
	void advance();

	void advance(int32_t step);

	std::string mText;
	Position mPos;
	std::vector<Token> mTokens;
	char* mCurrentChar{};
	std::string_view mFileName;
};
