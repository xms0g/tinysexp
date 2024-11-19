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

void printAST(NodePtr& ast) {
    NodePtr left = std::move(dynamic_cast<BinOpNode*>(ast.get())->leftNode);
    NodePtr right = std::move(dynamic_cast<BinOpNode*>(ast.get())->rightNode);

    if (dynamic_cast<NumberNode*>(left.get())) {
        std::cout << "LeftNode: " << dynamic_cast<NumberNode*>(left.get())->n << std::endl;
    } else
        printAST(left);

    if (dynamic_cast<NumberNode*>(right.get())) {
        std::cout << "RightNode: " << dynamic_cast<NumberNode*>(right.get())->n << std::endl;
    } else
        printAST(right);

}

void compile(const char* fn, std::string& program) {
    std::vector<Token> tokens;

    Lexer lexer{fn, program};
    Parser parser{fn};

    try {
        tokens = lexer.makeTokens();
        parser.setTokens(tokens);

        NodePtr ast = parser.parse();

        printAST(ast);

    } catch (IllegalCharError& e) {
        std::cerr << e.what();
    } catch (InvalidSyntaxError& e) {
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
