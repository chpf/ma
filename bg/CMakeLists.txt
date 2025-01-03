cmake_minimum_required(VERSION 3.16.3)
project(BLISGenerator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json")

# antlr4 runtime is needed
set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "ANTLR4 tests")
set(ANTLR4_INSTALL OFF CACHE BOOL "ANTLR4 installation targets")
set(ANTLR_BUILD_SHARED OFF CACHE BOOL "Build the shared library of the ANTLR runtime")
include(FetchContent)
FetchContent_Declare(
  antlr4_runtime
  GIT_REPOSITORY https://github.com/antlr/antlr4.git
  GIT_COMMIT cc82115a4e7f53d71d9d905caa2c2dfa4da58899 # release commit 4.13.2
  SOURCE_SUBDIR runtime/Cpp
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(antlr4_runtime)

set(GRAMMAR_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cdsl/CoreDSL2.g4")
set(GENERATED_PARSER_DIR "${CMAKE_CURRENT_BINARY_DIR}/cdsl")
file(MAKE_DIRECTORY ${GENERATED_PARSER_DIR})


find_program(ANTLR4_EXECUTABLE antlr4)
if (NOT ANTLR4_EXECUTABLE)
    message(FATAL_ERROR "ANTLR4 executable not found in PATH.")
endif()

add_custom_command(
    OUTPUT  ${GENERATED_PARSER_DIR}/CoreDSL2Lexer.cpp
            ${GENERATED_PARSER_DIR}/CoreDSL2Parser.cpp
            ${GENERATED_PARSER_DIR}/CoreDSL2Lexer.h
            ${GENERATED_PARSER_DIR}/CoreDSL2Parser.h
    COMMAND ${ANTLR4_EXECUTABLE} -visitor -listener -Dlanguage=Cpp ${GRAMMAR_FILE} -o ${GENERATED_PARSER_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${GRAMMAR_FILE}
    VERBATIM
)

add_executable(bg
    main.cpp
    ${GENERATED_PARSER_DIR}/CoreDSL2Lexer.cpp
    ${GENERATED_PARSER_DIR}/CoreDSL2Parser.cpp
)

target_include_directories(bg PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cdsl
    ${GENERATED_PARSER_DIR}
    ${antlr4_runtime_SOURCE_DIR}/runtime/src
)

target_link_libraries(bg PRIVATE
    antlr4_static
)
