#include <iostream>
#include <fstream>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "exceptions.hpp"

#define VERSION_MAJOR 0
#define VERSION_MINOR 3
#define VERSION_PATCH 1

#define STRINGIFY0(s) # s
#define STRINGIFY(s) STRINGIFY0(s)
#define VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

void printAST(ExprPtr& ast) {
    ExprPtr left = std::move(dynamic_cast<BinOpExpr*>(ast.get())->lhs);
    ExprPtr right = std::move(dynamic_cast<BinOpExpr*>(ast.get())->rhs);

    if (dynamic_cast<NumberExpr*>(left.get())) {
        std::cout << "LeftNode: " << dynamic_cast<NumberExpr*>(left.get())->n << std::endl;
    } else
        printAST(left);

    if (dynamic_cast<NumberExpr*>(right.get())) {
        std::cout << "RightNode: " << dynamic_cast<NumberExpr*>(right.get())->n << std::endl;
    } else
        printAST(right);
}

void compile(const char* fn, std::string& program) {
    std::vector<Token> tokens;
    std::string bfCode;

    Lexer lexer{fn, program};
    Parser parser{fn};
    CodeGen codegen;

    try {
        tokens = lexer.makeTokens();
        parser.setTokens(tokens);

        ExprPtr ast = parser.parse();

        bfCode = codegen.emit(ast);

        std::cout << bfCode << '\n';

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
