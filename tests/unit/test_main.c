#include "unity.h"

//--------------------------------------------------------
// Test that the Tree-sitter parser returns an owned handle for valid syntax.
//
// @returns: None.
//--------------------------------------------------------
void test_parser_accepts_valid_module_source(void);
//--------------------------------------------------------
// Test that malformed module syntax is rejected with a parsing diagnostic.
//
// @returns: None.
//--------------------------------------------------------
void test_parser_rejects_malformed_module_source(void);
//--------------------------------------------------------
// Test that a null source buffer is rejected before parser setup.
//
// @returns: None.
//--------------------------------------------------------
void test_parser_rejects_null_source_buffer(void);
//--------------------------------------------------------
// Test that the scalar direct-alias fixture lowers to the expected circuit.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_builds_scalar_alias_fixture(void);
//--------------------------------------------------------
// Test that the vector alias fixture preserves width and signedness.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_builds_signed_vector_alias_fixture(void);
//--------------------------------------------------------
// Test that bare ANSI ports inherit fully specified preceding attributes.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_inherits_complete_ansi_port_attributes(void);
//--------------------------------------------------------
// Test that unsupported RTL constructs are rejected during elaboration.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_rejects_unsupported_constructs(void);
//--------------------------------------------------------
// Test that syntax extraction visits grouped declarations and nested statements.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_visits_module_declarations_and_process_statements(void);
//--------------------------------------------------------
// Test that extraction reports typed declaration syntax and first bounds.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_extracts_typed_declaration_facts(void);
//--------------------------------------------------------
// Test that syntax visitor stop and failure actions end traversal immediately.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_syntax_visitor_stops_and_fails_immediately(void);

//--------------------------------------------------------
// Check arena growth, reset-based reuse, and preservation of prior data.
//
// @returns: None.
//--------------------------------------------------------
void test_arena_grows_without_invalidating_prior_allocations(void);
//--------------------------------------------------------
// Exercise record declarations from the public trace header.
//
// @returns: None.
//--------------------------------------------------------
void test_trace_declarations(void);
//--------------------------------------------------------
// Exercise record and function declarations from the verification header.
//
// @returns: None.
//--------------------------------------------------------
void test_verification_header_declarations(void);

//--------------------------------------------------------
// Initialize test state; these tests currently need no per-test setup.
//
// @returns: None.
//--------------------------------------------------------
void setUp(void)
{
}

//--------------------------------------------------------
// Tear down test state; each test releases its own resources.
//
// @returns: None.
//--------------------------------------------------------
void tearDown(void)
{
}

//--------------------------------------------------------
// Register and run the unit tests; registration order is not semantic.
//
// @returns: Unity's test-runner status.
//--------------------------------------------------------
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parser_accepts_valid_module_source);
    RUN_TEST(test_parser_rejects_malformed_module_source);
    RUN_TEST(test_parser_rejects_null_source_buffer);
    RUN_TEST(test_frontend_builds_scalar_alias_fixture);
    RUN_TEST(test_frontend_builds_signed_vector_alias_fixture);
    RUN_TEST(test_frontend_inherits_complete_ansi_port_attributes);
    RUN_TEST(test_frontend_rejects_unsupported_constructs);
    RUN_TEST(test_frontend_visits_module_declarations_and_process_statements);
    RUN_TEST(test_frontend_extracts_typed_declaration_facts);
    RUN_TEST(test_frontend_syntax_visitor_stops_and_fails_immediately);
    RUN_TEST(test_arena_grows_without_invalidating_prior_allocations);
    RUN_TEST(test_trace_declarations);
    RUN_TEST(test_verification_header_declarations);
    return UNITY_END();
}
