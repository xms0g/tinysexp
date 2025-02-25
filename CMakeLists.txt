cmake_minimum_required(VERSION 3.23)
project(tinysexp)

set(CMAKE_CXX_STANDARD 20)

if (APPLE)
    add_compile_options(-gdwarf-4)
endif ()

set(SOURCES
        src/frontend/lexer.cpp src/frontend/lexer.h
        src/frontend/parser.cpp src/frontend/parser.h
        src/frontend/semantic.cpp src/frontend/semantic.h
        src/backend/stack.cpp  src/backend/stack.h
        src/backend/register.cpp  src/backend/register.h
        src/backend/codegen.cpp src/backend/codegen.h
)

add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})
