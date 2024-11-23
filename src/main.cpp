#include <iostream>
#include <fstream>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "exceptions.hpp"

#define VERSION_MAJOR 0
#define VERSION_MINOR 5
#define VERSION_PATCH 1

#define STRINGIFY0(s) # s
#define STRINGIFY(s) STRINGIFY0(s)
#define VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

#define ERROR_COLOR "\x1b[31m"
#define RESET_COLOR "\x1b[0m"

void compile(const char* fn, std::string& program) {
    try {
        Lexer lexer{fn, program};
        Parser parser{fn, lexer};

        lexer.process();
        ExprPtr ast = parser.parse();

        std::cout << CodeGen::emit(ast) << '\n';

    } catch (IllegalCharError& e) {
        std::cerr << ERROR_COLOR << e.what();
    } catch (InvalidSyntaxError& e) {
        std::cerr << ERROR_COLOR << e.what();
    }
}

int main(int argc, char** argv) {
    std::string program;

    if (argc > 1) {
        std::ifstream file;
        file.open(argv[1], std::ios::in);

        file.seekg(0, std::ios::end);
        size_t length = file.tellg();
        file.seekg(0, std::ios::beg);

        file.read(program.data(), length);

        compile(argv[1], program);

    } else {
        while (true) {
            std::cout << RESET_COLOR << "lbf> ";
            std::getline(std::cin >> std::ws, program);

            if (program == "q")
                break;

            compile("<stdin>", program);
        }
    }

    return 0;
}
