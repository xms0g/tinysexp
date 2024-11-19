#pragma once

#include <exception>
#include <string>

class IllegalCharError : public std::exception {
public:
    explicit IllegalCharError(const char* fn, const char* detail, int ln) {
        msg += detail;
        msg += "\nFile " + std::string(fn) + ", line " + std::to_string(ln);
        msg += '\n';
    }

    [[nodiscard]] const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg = "Illegal Character: ";
};

class InvalidSyntaxError : public std::exception {
public:
    explicit InvalidSyntaxError(const char* fn, const char* detail, int ln) {
        msg += detail;
        msg += "\nFile " + std::string(fn) + ", line " + std::to_string(ln);
        msg += '\n';
    }

    [[nodiscard]] const char* what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg = "Invalid Syntax: ";
};
