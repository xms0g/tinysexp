#include <iostream>
#include <fstream>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include "exceptions.hpp"

#define VERSION_MAJOR 0
#define VERSION_MINOR 8
#define VERSION_PATCH 2

#define STRINGIFY0(s) # s
#define STRINGIFY(s) STRINGIFY0(s)
#define VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

#define ERROR_COLOR "\x1b[31m"
#define RESET_COLOR "\x1b[0m"

void compile(const char* fn, std::string& program) {
    try {
        Lexer lexer{fn, program};
        Parser parser{fn, lexer};
        SemanticAnalyzer analyzer{fn};
        CodeGen cgen;

        lexer.process();
        ExprPtr ast = parser.parse();
        analyzer.analyze(ast);

        std::cout << cgen.emit(ast) << '\n';

    } catch (IllegalCharError& e) {
        std::cerr << ERROR_COLOR << e.what();
    } catch (InvalidSyntaxError& e) {
        std::cerr << ERROR_COLOR << e.what();
    } catch (SemanticError& e) {
        std::cerr << ERROR_COLOR << e.what();
    }
}

int main(int argc, char** argv) {
    std::string program;
    const char* fn = argv[1];

    if (argc > 1) {
        std::ifstream file;

        file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        try {
            file.open(fn, std::ios::in);

            file.seekg(0, std::ios::end);
            std::size_t length = file.tellg();
            file.seekg(0, std::ios::beg);

            program.resize(length);

            file.read(program.data(), length);

            file.close();
        } catch (std::ifstream::failure& e) {
            std::cerr << "Exception opening/reading file: " << e.what() << "\t";
            exit(EXIT_FAILURE);
        }

        compile(fn, program);
    }

    return 0;
}
