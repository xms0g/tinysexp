#include <iostream>
#include <fstream>
#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "codegen.hpp"
#include "exceptions.hpp"

#define VERSION_MAJOR 0
#define VERSION_MINOR 8
#define VERSION_PATCH 47

#define STRINGIFY0(s) # s
#define STRINGIFY(s) STRINGIFY0(s)
#define VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

#define ERROR_COLOR "\x1b[31m"
#define RESET_COLOR "\x1b[0m"

static void compile(const std::string_view fn, const std::string_view in, const std::string_view out) {
    std::ofstream asmFile;
    asmFile.open(out);

    try {
        Lexer lexer{fn, in};
        Parser parser{fn, lexer};
        SemanticAnalyzer analyzer{fn};
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
    static auto usage =
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

    std::string fn, in, out;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {
            out = argv[++i];
        } else {
            fn = argv[i];
        }
    }

    if (out.empty()) {
        size_t pos = fn.rfind('.');
        std::string base = pos != std::string::npos ? fn.substr(0, pos) : fn;
        out = base + ".s";
    }

    std::ifstream file;
    file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    try {
        file.open(fn, std::ios::in);

        file.seekg(0, std::ios::end);
        const std::size_t length = file.tellg();
        file.seekg(0, std::ios::beg);

        in.resize(length);

        file.read(in.data(), static_cast<long>(length));

        file.close();
    } catch (std::ifstream::failure& e) {
        std::cerr << "Exception opening/reading file: " << e.what() << "\t";
        exit(EXIT_FAILURE);
    }

    compile(fn, in, out);

    return 0;
}
