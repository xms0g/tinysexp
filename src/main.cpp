#include <iostream>
#include <fstream>
#include <cstdlib>
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

static void compile(const std::string_view fn, const std::string_view src, const std::string_view outFn) {
    std::ofstream asmFile;

	auto outFnStr = std::string(outFn) + ".asm";
    asmFile.open(outFnStr);

    try {
        Lexer lexer{fn, src};
        Parser parser{fn, lexer};
        SemanticAnalyzer analyzer{fn};
        CodeGen cgen;

        lexer.process();
        ExprPtr ast = parser.parse();
        analyzer.analyze(ast);
        asmFile << cgen.emit(ast);
    	asmFile.close();

    	std::system(std::format("nasm -f macho64 {} -o {}.o", outFnStr, outFnStr).c_str());
    	std::system(std::format("clang {}.o -o {}_exec", outFnStr, outFn).c_str());
    } catch (IllegalCharError& e) {
        std::cerr << ERROR_COLOR << e.what();
    } catch (InvalidSyntaxError& e) {
        std::cerr << ERROR_COLOR << e.what();
    } catch (SemanticError& e) {
        std::cerr << ERROR_COLOR << e.what();
    }
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

    std::string fn, src, outFn;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {
            outFn = argv[++i];
        } else {
            fn = argv[i];
        }
    }

    if (outFn.empty()) {
        size_t pos = fn.rfind('.');
        std::string base = pos != std::string::npos ? fn.substr(0, pos) : fn;
        outFn = base;
    }

    std::ifstream file;
    file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
    try {
        file.open(fn, std::ios::in);

        file.seekg(0, std::ios::end);
        const std::size_t length = file.tellg();
        file.seekg(0, std::ios::beg);

        src.resize(length);

        file.read(src.data(), static_cast<long>(length));

        file.close();
    } catch (std::ifstream::failure& e) {
        std::cerr << "Exception opening/reading file: " << e.what() << "\t";
        exit(EXIT_FAILURE);
    }

    compile(fn, src, outFn);

    return 0;
}
