#ifndef TINYSEXP_EXCEPTIONS_HPP
#define TINYSEXP_EXCEPTIONS_HPP

#include <exception>
#include <format>
#include <string>

/* Syntax Errors */
constexpr auto MISSING_PAREN_ERROR = "Missing parenthesis";
constexpr auto EXPECTED_NUMBER_ERROR = "Expected int or double";
constexpr auto EXPECTED_ELEMS_NUMBER_ERROR = "Too few elements in '{}'";
constexpr auto OP_INVALID_NUMBER_OF_ARGS_ERROR = "The function '{}' is called with two arguments, but wants exactly one.\n"
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

#define ERROR(STR, ...) std::format(STR, __VA_ARGS__).c_str()

class IError : public std::exception {
public:
    explicit IError(const char* msg, const char* fn, const char* detail, int ln) : msg(msg) {
        this->msg += detail;
        this->msg += "\nFile " + std::string(fn) + ", line " + std::to_string(ln);
        this->msg += '\n';
    }

    [[nodiscard]] const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

class IllegalCharError final : public IError {
public:
    IllegalCharError(const char* fn, const char* detail, int ln) :
            IError("Illegal Character: ", fn, detail, ln) {}
};

class InvalidSyntaxError final : public IError {
public:
    explicit InvalidSyntaxError(const char* fn, const char* detail, int ln) :
            IError("Invalid Syntax: ", fn, detail, ln) {}
};

class SemanticError final : public IError {
public:
    explicit SemanticError(const char* fn, const char* detail, int ln) :
            IError("Error: ", fn, detail, ln) {}
};

#endif
