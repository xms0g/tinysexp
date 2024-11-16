#include <iostream>
#include <fstream>
#include "lexer.h"
#include "parser.h"
#include "exceptions.hpp"

#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#define VERSION_PATCH 0

#define STRINGIFY0(s) # s
#define STRINGIFY(s) STRINGIFY0(s)
#define VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

void compile(const char* fn, std::string& program) {
    std::vector<Token> tokens;

    Lexer lexer{fn, program};
    Parser parser;

    try {
        tokens = lexer.makeTokens();
        parser.setTokens(tokens);

        auto ast = parser.parse();

        std::cout << std::format("LeftNode: {}\n"
                                 "RightNode: {}\n",
                                 dynamic_cast<NumberNode*>(dynamic_cast<BinOpNode*>(ast.get())->leftNode.get())->n,
                                 dynamic_cast<NumberNode*>(dynamic_cast<BinOpNode*>(ast.get())->rightNode.get())->n);

    } catch (IllegalCharError& e) {
        std::cerr << e.what();
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
            std::cout << "lbf> ";
            std::getline(std::cin >> std::ws, program);

            if (program == "q")
                break;

            compile("<stdin>", program);
        }
    }

    return 0;
}
