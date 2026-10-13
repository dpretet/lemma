cmake_minimum_required(VERSION 3.16)

project(lemma VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)

set(TREE_SITTER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tree-sitter/lib")
set(SYSTEMVERILOG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tree-sitter-systemverilog")
set(SYSTEMVERILOG_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/systemverilog")
set(SYSTEMVERILOG_PARSER "${SYSTEMVERILOG_GENERATED_DIR}/src/parser.c")
file(MAKE_DIRECTORY "${SYSTEMVERILOG_GENERATED_DIR}")

file(GLOB TREE_SITTER_SOURCES CONFIGURE_DEPENDS "${TREE_SITTER_DIR}/src/*.c")
list(REMOVE_ITEM TREE_SITTER_SOURCES "${TREE_SITTER_DIR}/src/lib.c")

add_library(tree_sitter_runtime STATIC ${TREE_SITTER_SOURCES})
target_include_directories(tree_sitter_runtime
    PUBLIC "${TREE_SITTER_DIR}/include"
    PRIVATE "${TREE_SITTER_DIR}/src" "${TREE_SITTER_DIR}/src/wasm"
)
set_target_properties(tree_sitter_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_custom_command(
    OUTPUT "${SYSTEMVERILOG_PARSER}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SYSTEMVERILOG_GENERATED_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E tar xzf
            "${SYSTEMVERILOG_DIR}/parser-source.tar.gz"
    COMMAND "${CMAKE_COMMAND}" -E touch "${SYSTEMVERILOG_PARSER}"
    DEPENDS "${SYSTEMVERILOG_DIR}/parser-source.tar.gz"
    WORKING_DIRECTORY "${SYSTEMVERILOG_GENERATED_DIR}"
    COMMENT "Extracting vendored SystemVerilog parser source"
    VERBATIM
)
add_library(systemverilog_grammar STATIC
    "${SYSTEMVERILOG_PARSER}"
)
target_include_directories(systemverilog_grammar
    PUBLIC "${SYSTEMVERILOG_DIR}/bindings/c"
    PRIVATE "${SYSTEMVERILOG_GENERATED_DIR}/src" "${SYSTEMVERILOG_DIR}/src"
)
target_link_libraries(systemverilog_grammar PUBLIC tree_sitter_runtime)

add_library(tsoding_arena STATIC src/memory/arena.c)
target_include_directories(tsoding_arena
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tsoding-arena"
)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(tsoding_arena PRIVATE -UNDEBUG)
elseif(MSVC)
    target_compile_options(tsoding_arena PRIVATE /UNDEBUG)
else()
    message(FATAL_ERROR "The arena target needs a compiler option to keep assert enabled")
endif()

add_library(lemma_core STATIC src/frontend/parser.c)
target_include_directories(lemma_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(lemma_core PUBLIC
    systemverilog_grammar
    tree_sitter_runtime
    tsoding_arena
)

add_executable(lemma src/main.c)
target_link_libraries(lemma PRIVATE lemma_core)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(lemma_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(lemma PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS lemma RUNTIME DESTINATION bin)

if(BUILD_TESTING)
    add_executable(lemma_unit_tests
        tests/unit/test_main.c
        tests/unit/test_parser.c
        tests/unit/test_arena.c
        third_party/unity/src/unity.c
    )
    target_include_directories(lemma_unit_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src"
    )
    target_link_libraries(lemma_unit_tests PRIVATE lemma_core)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(lemma_unit_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME lemma_unit_tests COMMAND lemma_unit_tests)
    add_test(NAME lemma_parse_valid_fixture
        COMMAND lemma "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/valid.sv")
    add_test(NAME lemma_rejects_malformed_fixture
        COMMAND lemma "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/malformed.sv")
    set_tests_properties(lemma_rejects_malformed_fixture PROPERTIES WILL_FAIL TRUE)
endif()
