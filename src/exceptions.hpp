#pragma once
#include <exception>
#include <format>
#include <string>
#include <utility>

/* Syntax Errors */
constexpr auto MISSING_PAREN_ERROR = "Missing parenthesis";
constexpr auto EXPECTED_NUMBER_ERROR = "Expected int or double";
constexpr auto SEXPR_ERROR = "S-expression is not allowed here";
constexpr auto EXPECTED_ELEMS_NUMBER_ERROR = "Too few elements in '{}'";
constexpr auto OP_INVALID_NUMBER_OF_ARGS_ERROR =
		"The function '{}' is called with two arguments, but wants exactly one.\n"
		"Invalid number of arguments: {}";
/* Semantic Errors */
// Variables
constexpr auto UNBOUND_VAR_ERROR = "The variable '{}' is unbound";
constexpr auto CONSTANT_VAR_ERROR = "'{}' is a constant";
constexpr auto CONSTANT_VAR_DECL_ERROR = "Constant variable '{}' is not allowed here";
constexpr auto GLOBAL_VAR_DECL_ERROR = "Global variable '{}' is not allowed here";
constexpr auto MULTIPLE_DECL_ERROR = "The variable '{}' occurs more than once";
constexpr auto NOT_NUMBER_ERROR = "The value '{}' is not of type number";
constexpr auto NOT_INT_ERROR = "The value '{}' is not of type INTEGER";

// Functions
constexpr auto FUNC_UNDEFINED_ERROR = "The function '{}' is undefined";
constexpr auto FUNC_INVALID_NUMBER_OF_ARGS_ERROR = "'{}' Invalid number of arguments: {}";
constexpr auto FUNC_DEF_ERROR = "Function '{}' definition is not allowed here";

#define ERROR(STR, ...) std::format(STR, __VA_ARGS__)

class IError : public std::exception {
public:
	explicit IError(std::string err, const std::string_view fn, const std::string_view detail, const int32_t ln)
		: mErrStr(std::move(err)) {
		mErrStr += detail;
		mErrStr += "\nFile " + std::string(fn) + ", line " + std::to_string(ln);
		mErrStr += '\n';
	}

	[[nodiscard]] const char* what() const noexcept override {
		return mErrStr.c_str();
	}

private:
	std::string mErrStr;
};

class IllegalCharError final : public IError {
public:
	IllegalCharError(std::string_view fn, std::string_view detail, const int32_t ln)
		: IError("Illegal Character: ", fn, detail, ln) {
	}
};

class InvalidSyntaxError final : public IError {
public:
	explicit InvalidSyntaxError(const std::string_view fn, std::string_view detail, const int32_t ln)
		: IError("Invalid Syntax: ", fn, detail, ln) {
	}
};

class SemanticError final : public IError {
public:
	explicit SemanticError(const std::string_view fn, std::string_view detail, const int32_t ln)
		: IError("Error: ", fn, detail, ln) {
	}
};
