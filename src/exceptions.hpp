#ifndef TINYSEXP_EXCEPTIONS_HPP
#define TINYSEXP_EXCEPTIONS_HPP

#include <exception>
#include <format>
#include <string>

/* Syntax Errors */
constexpr const char* MISSING_PAREN_ERROR = "Missing parenthesis";
constexpr const char* EXPECTED_NUMBER_ERROR = "Expected int or double";
constexpr const char* EXPECTED_ELEMS_NUMBER_ERROR = "Too few elements in '{}'";
constexpr const char* OP_INVALID_NUMBER_OF_ARGS_ERROR = "The function '{}' is called with two arguments, but wants exactly one.\n"
                                                        "Invalid number of arguments: {}";
/* Semantic Errors */
// Variables
constexpr const char* UNBOUND_VAR_ERROR = "The variable '{}' is unbound";
constexpr const char* CONSTANT_VAR_ERROR = "'{}' is a constant";
constexpr const char* CONSTANT_VAR_DECL_ERROR = "Constant variable '{}' is not allowed here";
constexpr const char* GLOBAL_VAR_DECL_ERROR = "Global variable '{}' is not allowed here";
constexpr const char* MULTIPLE_DECL_ERROR = "The variable '{}' occurs more than once";
constexpr const char* NOT_NUMBER_ERROR = "The value '{}' is not of type number";
// Functions
constexpr const char* FUNC_UNDEFINED_ERROR = "The function '{}' is undefined";
constexpr const char* FUNC_INVALID_NUMBER_OF_ARGS_ERROR = "'{}' Invalid number of arguments: {}";
constexpr const char* FUNC_DEF_ERROR = "Function '{}' definition is not allowed here";

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

class IllegalCharError : public IError {
public:
    IllegalCharError(const char* fn, const char* detail, int ln) :
            IError("Illegal Character: ", fn, detail, ln) {}
};

class InvalidSyntaxError : public IError {
public:
    explicit InvalidSyntaxError(const char* fn, const char* detail, int ln) :
            IError("Invalid Syntax: ", fn, detail, ln) {}
};

class SemanticError : public IError {
public:
    explicit SemanticError(const char* fn, const char* detail, int ln) :
            IError("Error: ", fn, detail, ln) {}
};

#endif
