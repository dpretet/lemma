#include "lemma/frontend_parse.h"

#include "unity.h"

#include <string.h>

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
    TEST_ASSERT_TRUE(diagnostic.has_span);
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
