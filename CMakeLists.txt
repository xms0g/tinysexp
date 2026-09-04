cmake_minimum_required(VERSION 3.23)
project(tinysexp)

set(CMAKE_CXX_STANDARD 23)

if (APPLE)
    add_compile_options(-gdwarf-4)
endif ()

set(SOURCES
        src/lexer.cpp src/lexer.hpp
        src/parser.cpp src/parser.hpp
        src/semantic.cpp src/semantic.hpp
        src/stack.cpp  src/stack.hpp
        src/register.cpp  src/register.hpp
        src/codegen.cpp src/codegen.hpp
)

add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})

add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lrt.c"
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/src/lrt.c"
        "${CMAKE_CURRENT_BINARY_DIR}/lrt.c"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/lrt.c"
        COMMENT "Updating lrt.c in binary directory..."
)

target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/lrt.c")
