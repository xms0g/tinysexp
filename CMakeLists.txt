cmake_minimum_required(VERSION 3.23)
project(tinysexp)

set(CMAKE_CXX_STANDARD 20)

if (APPLE)
    add_compile_options(-gdwarf-4)
endif ()

set(SOURCES
        src/lexer.cpp src/lexer.h
        src/parser.cpp src/parser.h
        src/codegen.cpp src/codegen.h)

add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})
