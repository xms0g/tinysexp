#ifndef TINYSEXP_ERROR_HPP
#define TINYSEXP_ERROR_HPP

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

#define ERROR(STR, N) std::format((STR), (N)).c_str()

#endif //TINYSEXP_ERROR_HPP
