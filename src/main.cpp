#include <iostream>
#include <fstream>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include "exceptions.hpp"

#define VERSION_MAJOR 0
#define VERSION_MINOR 8
#define VERSION_PATCH 29

#define STRINGIFY0(s) # s
#define STRINGIFY(s) STRINGIFY0(s)
#define VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

#define ERROR_COLOR "\x1b[31m"
#define RESET_COLOR "\x1b[0m"

void compile(std::string& fn, const std::string& in, std::string& out) {
    std::ofstream asmFile;
    asmFile.open(out);

    try {
        Lexer lexer{fn.c_str(), in};
        Parser parser{fn.c_str(), lexer};
        SemanticAnalyzer analyzer{fn.c_str()};
        CodeGen cgen;

        lexer.process();
        ExprPtr ast = parser.parse();
        analyzer.analyze(ast);

        asmFile << cgen.emit(ast);
    } catch (IllegalCharError& e) {
        std::cerr << ERROR_COLOR << e.what();
    } catch (InvalidSyntaxError& e) {
        std::cerr << ERROR_COLOR << e.what();
    } catch (SemanticError& e) {
        std::cerr << ERROR_COLOR << e.what();
    }

    asmFile.close();
}

int main(int argc, char** argv) {
    static const char* usage =
            "OVERVIEW: Lisp compiler for x86-64 architecture\n\n"
            "USAGE: tinysexp [options] file\n\n"
            "OPTIONS:\n"
            "  -o, --output          The output file name\n"
            "  -h, --help            Display available options\n"
            "  -v, --version         Display the version of this program\n";

    if (argc < 2) {
        std::cerr << usage << std::endl;
        return EXIT_FAILURE;
    }

    if (!std::strcmp(argv[1], "-h") || !std::strcmp(argv[1], "--help")) {
        std::cout << usage << std::endl;
        return EXIT_SUCCESS;
    }

    if (!std::strcmp(argv[1], "-v") || !std::strcmp(argv[1], "--version")) {
        std::cout << VERSION << std::endl;
        return EXIT_SUCCESS;
    }

    std::string fn, lispFile, outputFile;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {
            outputFile = argv[++i];
        } else {
            fn = argv[i];
        }
    }

    if (outputFile.empty()) {
        size_t pos = fn.rfind('.');
        std::string base = pos != std::string::npos ? fn.substr(0, pos) : fn;
        outputFile = base + ".s";
    }

    std::ifstream file;
    file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    try {
        file.open(fn, std::ios::in);

        file.seekg(0, std::ios::end);
        const std::size_t length = file.tellg();
        file.seekg(0, std::ios::beg);

        lispFile.resize(length);

        file.read(lispFile.data(), length);

        file.close();
    } catch (std::ifstream::failure& e) {
        std::cerr << "Exception opening/reading file: " << e.what() << "\t";
        exit(EXIT_FAILURE);
    }

    compile(fn, lispFile, outputFile);

    return 0;
}
