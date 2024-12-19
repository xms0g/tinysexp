#ifndef TINYSEXP_EXCEPTIONS_HPP
#define TINYSEXP_EXCEPTIONS_HPP

#include <exception>
#include <string>

/* Syntax Errors */
constexpr const char* MISSING_PAREN_ERROR = "Missing parenthesis";
constexpr const char* EXPECTED_NUMBER_ERROR = "Expected int or double";
constexpr const char* EXPECTED_ELEMS_NUMBER_ERROR = "Too few elements in '{}'";
/* Semantic Errors */
constexpr const char* UNBOUND_VAR_ERROR = "The variable '{}' is unbound";
constexpr const char* CONSTANT_VAR_ERROR = "'{}' is a constant";
constexpr const char* CONSTANT_VAR_DECL_ERROR = "Constant variable '{}' is not allowed here";
constexpr const char* GLOBAL_VAR_DECL_ERROR = "Global variable '{}' is not allowed here";
constexpr const char* MULTIPLE_DECL_ERROR = "The variable {} occurs more than once";
constexpr const char* T_VAR_ERROR = "The value 't' is not of type number";
constexpr const char* NIL_VAR_ERROR = "The value 'nil' is not of type number";
constexpr const char* INVALID_NUMBER_OF_ARGS_ERROR = "Invalid number of arguments: {}";
constexpr const char* FUNC_DEF_ERROR = "Function '{}' definition is not allowed here";

#define ERROR(STR, N) std::format(STR, N).c_str()

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

class MultipleDeclarationError : public IError {
public:
    explicit MultipleDeclarationError(const char* fn, const char* detail, int ln) :
            IError("Multiple Declaration: ", fn, detail, ln) {}
};

class UnboundVariableError : public IError {
public:
    explicit UnboundVariableError(const char* fn, const char* detail, int ln) :
            IError("Unbound Variable: ", fn, detail, ln) {}
};

class TypeError : public IError {
public:
    explicit TypeError(const char* fn, const char* detail, int ln) :
            IError("Type Error: ", fn, detail, ln) {}
};

class ScopeError : public IError {
public:
    explicit ScopeError(const char* fn, const char* detail, int ln) :
            IError("Scope Error: ", fn, detail, ln) {}
};

#endif
