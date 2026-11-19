#include "lemma/frontend_parse.h"

#include "unity.h"

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RTL_FIXTURE_COUNT 54U

//--------------------------------------------------------
// Read a named RTL fixture into caller-owned NUL-terminated storage.
//
// @name: Fixture file name relative to tests/fixtures/rtl.
// @length: Receives the source byte length on success.
// @returns: Caller-owned source, or NULL on path, I/O, size, or allocation failure.
//--------------------------------------------------------
static char *read_rtl_fixture(const char *name, size_t *length)
{
    char path[512];
    int written = snprintf(path, sizeof(path), "%s/tests/fixtures/rtl/%s",
                           LEMMA_SOURCE_DIR, name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return NULL;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0) {
        if (file != NULL) {
            (void)fclose(file);
        }
        return NULL;
    }
    long file_size = ftell(file);
    if (file_size < 0 || (uintmax_t)file_size > UINT32_MAX ||
        (uintmax_t)file_size >= SIZE_MAX ||
        fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }
    size_t size = (size_t)file_size;
    char *source = malloc(size + 1U);
    if (source == NULL || fread(source, 1U, size, file) != size ||
        ferror(file) != 0) {
        free(source);
        (void)fclose(file);
        return NULL;
    }
    if (fclose(file) != 0) {
        free(source);
        return NULL;
    }
    source[size] = '\0';
    *length = size;
    return source;
}

//--------------------------------------------------------
// Check whether a directory entry names a SystemVerilog fixture.
//
// @name: Directory-entry name.
// @returns: Nonzero when name ends in the exact .sv suffix.
//--------------------------------------------------------
static int is_systemverilog_fixture(const char *name)
{
    size_t length = strlen(name);
    return length >= 3U && strcmp(name + length - 3U, ".sv") == 0;
}

//--------------------------------------------------------
// Accept a complete module and return a parsed handle while its source lives.
//
// @returns: None.
//--------------------------------------------------------
void test_parser_accepts_valid_module_source(void)
{
    static const char source[] = "module top(input wire a); endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(
        source, sizeof(source) - 1U, &diagnostic);

    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);
    TEST_ASSERT_NULL(diagnostic.code);
    lemma_parsed_source_destroy(parsed);
}

//--------------------------------------------------------
// Reject malformed syntax at parsing and report a source-located diagnostic.
//
// @returns: None.
//--------------------------------------------------------
void test_parser_rejects_malformed_module_source(void)
{
    static const char source[] = "module broken(input wire a) endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(
        source, sizeof(source) - 1U, &diagnostic);

    TEST_ASSERT_NULL(parsed);
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_STAGE_PARSING, diagnostic.stage);
    TEST_ASSERT_NOT_NULL(diagnostic.code);
    TEST_ASSERT_TRUE(diagnostic.has_source_range);
}

//--------------------------------------------------------
// Reject a null input buffer before passing it to Tree-sitter.
//
// @returns: None.
//--------------------------------------------------------
void test_parser_rejects_null_source_buffer(void)
{
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(NULL, 0U, &diagnostic);

    TEST_ASSERT_NULL(parsed);
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_STAGE_PARSING, diagnostic.stage);
    TEST_ASSERT_EQUAL_STRING("parse.invalid_input", diagnostic.code);
}

//--------------------------------------------------------
// Stop syntax-error inspection at a bounded depth with a source-located diagnostic.
//
// @returns: None.
//--------------------------------------------------------
void test_parser_rejects_overdeep_error_tree_without_recursing_unboundedly(void)
{
    static const char prefix[] = "module deep; always_comb ";
    static const char suffix[] = "endmodule";
    char source[4096];
    size_t offset = 0U;
    memcpy(source + offset, prefix, sizeof(prefix) - 1U);
    offset += sizeof(prefix) - 1U;
    for (size_t index = 0U; index < 160U; ++index) {
        memcpy(source + offset, "begin ", 6U);
        offset += 6U;
    }
    memcpy(source + offset, "@;", 2U);
    offset += 2U;
    for (size_t index = 0U; index < 160U; ++index) {
        memcpy(source + offset, "end ", 4U);
        offset += 4U;
    }
    memcpy(source + offset, suffix, sizeof(suffix));

    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(
        source, offset + sizeof(suffix) - 1U, &diagnostic);

    TEST_ASSERT_NULL(parsed);
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_STAGE_PARSING, diagnostic.stage);
    TEST_ASSERT_EQUAL_STRING("parse.tree_too_deep", diagnostic.code);
    TEST_ASSERT_TRUE(diagnostic.has_source_range);
}

//--------------------------------------------------------
// Parse every .sv fixture and fail if the checked-in corpus count changes.
//
// @returns: None.
//--------------------------------------------------------
void test_parser_parses_all_rtl_fixtures(void)
{
    char directory_path[512];
    int written = snprintf(directory_path, sizeof(directory_path),
                           "%s/tests/fixtures/rtl", LEMMA_SOURCE_DIR);
    TEST_ASSERT_TRUE(written >= 0 && (size_t)written < sizeof(directory_path));
    DIR *directory = opendir(directory_path);
    TEST_ASSERT_NOT_NULL(directory);

    size_t fixture_count = 0U;
    int success = 1;
    char failure[512] = {0};
    for (;;) {
        errno = 0;
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                (void)snprintf(failure, sizeof(failure),
                               "unable to enumerate RTL fixtures");
                success = 0;
            }
            break;
        }
        if (!is_systemverilog_fixture(entry->d_name)) {
            continue;
        }
        ++fixture_count;
        size_t length = 0U;
        char *source = read_rtl_fixture(entry->d_name, &length);
        if (source == NULL) {
            (void)snprintf(failure, sizeof(failure),
                           "unable to read RTL fixture %s", entry->d_name);
            success = 0;
            break;
        }
        lemma_frontend_diagnostic diagnostic = {0};
        lemma_parsed_source *parsed = lemma_frontend_parse(source, length, &diagnostic);
        if (parsed == NULL) {
            (void)snprintf(failure, sizeof(failure),
                           "fixture %s failed syntax parsing: %s",
                           entry->d_name,
                           diagnostic.message[0] == '\0' ? "no diagnostic"
                                                          : diagnostic.message);
            success = 0;
        }
        lemma_parsed_source_destroy(parsed);
        free(source);
        if (success == 0) {
            break;
        }
    }
    (void)closedir(directory);

    TEST_ASSERT_TRUE_MESSAGE(success != 0, failure);
    TEST_ASSERT_EQUAL_UINT(RTL_FIXTURE_COUNT, fixture_count);
}
