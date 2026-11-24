#include "unity.h"

//---------------------------------------------------------------------------
// Test that the Tree-sitter parser returns an owned handle for valid syntax.
//
// @returns: None.
//---------------------------------------------------------------------------
void test_parser_accepts_valid_module_source(void);
//-------------------------------------------------------------------------
// Test that malformed module syntax is rejected with a parsing diagnostic.
//
// @returns: None.
//-------------------------------------------------------------------------
void test_parser_rejects_malformed_module_source(void);
//----------------------------------------------------------------
// Test that a null source buffer is rejected before parser setup.
//
// @returns: None.
//----------------------------------------------------------------
void test_parser_rejects_null_source_buffer(void);
//--------------------------------------------------------------------------------
// Test that an over-deep error tree returns a bounded, source-located diagnostic.
//
// @returns: None.
//--------------------------------------------------------------------------------
void test_parser_rejects_overdeep_error_tree_without_recursing_unboundedly(void);
//------------------------------------------------------------------------
// Test that every checked-in RTL fixture remains syntactically parseable.
//
// @returns: None.
//------------------------------------------------------------------------
void test_parser_parses_all_rtl_fixtures(void);
//--------------------------------------------------------------------------
// Test that the scalar direct-alias fixture lowers to the expected circuit.
//
// @returns: None.
//--------------------------------------------------------------------------
void test_frontend_builds_scalar_alias_fixture(void);
//-----------------------------------------------------------------------------
// Test that boolean expression syntax preserves source order and parent links.
//
// @returns: None.
//-----------------------------------------------------------------------------
void test_frontend_exposes_boolean_expression_syntax_tree(void);
//------------------------------------------------------------------------
// Test that the boolean continuous-assignment fixture lowers to typed IR.
//
// @returns: None.
//------------------------------------------------------------------------
void test_frontend_lowers_boolean_expression_fixture(void);
//--------------------------------------------------------------------------------
// Test that vector bitwise, reduction, arithmetic, and comparison fixtures lower.
//
// @returns: None.
//--------------------------------------------------------------------------------
void test_frontend_lowers_vector_operator_fixtures(void);
//----------------------------------------------------------------------
// Test that a caller-owned frontend result owns the successful circuit.
//
// @returns: None.
//----------------------------------------------------------------------
void test_frontend_result_owns_successful_circuit(void);
//----------------------------------------------------------------------------
// Test that parse rejection returns an error diagnostic with source location.
//
// @returns: None.
//----------------------------------------------------------------------------
void test_frontend_result_reports_parse_error(void);
//--------------------------------------------------------------------------
// Test that RTL rejection returns an error diagnostic with source location.
//
// @returns: None.
//--------------------------------------------------------------------------
void test_frontend_result_reports_rtl_error(void);
//-------------------------------------------------------------------
// Test that the vector alias fixture preserves width and signedness.
//
// @returns: None.
//-------------------------------------------------------------------
void test_frontend_builds_signed_vector_alias_fixture(void);
//------------------------------------------------------------------------
// Test that bare ANSI ports inherit fully specified preceding attributes.
//
// @returns: None.
//------------------------------------------------------------------------
void test_frontend_inherits_complete_ansi_port_attributes(void);
//-----------------------------------------------------------------------------
// Test that packed declaration widths ignore ascending or descending direction.
//
// @returns: None.
//-----------------------------------------------------------------------------
void test_frontend_accepts_ascending_and_descending_packed_ranges(void);
//----------------------------------------------------------------------
// Test that unsupported RTL constructs are rejected during elaboration.
//
// @returns: None.
//----------------------------------------------------------------------
void test_frontend_rejects_unsupported_constructs(void);
//------------------------------------------------------------------------
// Test unsupported expression boundaries and nested-read cycle detection.
//
// @returns: None.
//------------------------------------------------------------------------
void test_frontend_rejects_unsupported_expressions_and_nested_cycles(void);
//-----------------------------------------------------------------------------
// Test that operators and excluded forms reject at RTL validation with ranges.
//
// @returns: None.
//-----------------------------------------------------------------------------
void test_frontend_rejects_unsupported_operator_boundaries(void);
//-----------------------------------------------------------------------------
// Test that drive strength on a continuous assignment is rejected with a range.
//
// @returns: None.
//-----------------------------------------------------------------------------
void test_frontend_rejects_continuous_assignment_drive_strength(void);
//-------------------------------------------------------------------------------
// Test that syntax extraction visits grouped declarations and nested statements.
//
// @returns: None.
//-------------------------------------------------------------------------------
void test_frontend_visits_module_declarations_and_process_statements(void);
//------------------------------------------------------------------------
// Test that extraction reports typed declaration syntax and first bounds.
//
// @returns: None.
//------------------------------------------------------------------------
void test_frontend_extracts_typed_declaration_facts(void);
//-----------------------------------------------------------------------------
// Test that syntax visitor stop and failure actions end traversal immediately.
//
// @returns: None.
//-----------------------------------------------------------------------------
void test_frontend_syntax_visitor_stops_and_fails_immediately(void);
//--------------------------------------------------------------------
// Test that deeply nested expressions and process syntax fail safely.
//
// @returns: None.
//--------------------------------------------------------------------
void test_frontend_rejects_overdeep_syntax_walks(void);
//---------------------------------------------------------------------------------------
// Test that builder arrays grow safely while preserving stored IDs and records.
//
// @returns: None.
//---------------------------------------------------------------------------------------
void test_circuit_builder_grows_owned_arrays(void);

//-----------------------------------------------------------------------
// Check arena growth, reset-based reuse, and preservation of prior data.
//
// @returns: None.
//-----------------------------------------------------------------------
void test_arena_grows_without_invalidating_prior_allocations(void);
//-----------------------------------------------------------
// Exercise record declarations from the public trace header.
//
// @returns: None.
//-----------------------------------------------------------
void test_trace_declarations(void);
//------------------------------------------------------------------------
// Exercise record and function declarations from the verification header.
//
// @returns: None.
//------------------------------------------------------------------------
void test_verification_header_declarations(void);

//---------------------------------------------------------------------
// Initialize test state; these tests currently need no per-test setup.
//
// @returns: None.
//---------------------------------------------------------------------
void setUp(void)
{
}

//------------------------------------------------------------
// Tear down test state; each test releases its own resources.
//
// @returns: None.
//------------------------------------------------------------
void tearDown(void)
{
}

//---------------------------------------------------------------------
// Register and run the unit tests; registration order is not semantic.
//
// @returns: Unity's test-runner status.
//---------------------------------------------------------------------
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parser_accepts_valid_module_source);
    RUN_TEST(test_parser_rejects_malformed_module_source);
    RUN_TEST(test_parser_rejects_null_source_buffer);
    RUN_TEST(test_parser_rejects_overdeep_error_tree_without_recursing_unboundedly);
    RUN_TEST(test_parser_parses_all_rtl_fixtures);
    RUN_TEST(test_frontend_builds_scalar_alias_fixture);
    RUN_TEST(test_frontend_exposes_boolean_expression_syntax_tree);
    RUN_TEST(test_frontend_lowers_boolean_expression_fixture);
    RUN_TEST(test_frontend_lowers_vector_operator_fixtures);
    RUN_TEST(test_frontend_result_owns_successful_circuit);
    RUN_TEST(test_frontend_result_reports_parse_error);
    RUN_TEST(test_frontend_result_reports_rtl_error);
    RUN_TEST(test_frontend_builds_signed_vector_alias_fixture);
    RUN_TEST(test_frontend_inherits_complete_ansi_port_attributes);
    RUN_TEST(test_frontend_accepts_ascending_and_descending_packed_ranges);
    RUN_TEST(test_frontend_rejects_unsupported_constructs);
    RUN_TEST(test_frontend_rejects_unsupported_expressions_and_nested_cycles);
    RUN_TEST(test_frontend_rejects_unsupported_operator_boundaries);
    RUN_TEST(test_frontend_rejects_continuous_assignment_drive_strength);
    RUN_TEST(test_frontend_visits_module_declarations_and_process_statements);
    RUN_TEST(test_frontend_extracts_typed_declaration_facts);
    RUN_TEST(test_frontend_syntax_visitor_stops_and_fails_immediately);
    RUN_TEST(test_frontend_rejects_overdeep_syntax_walks);
    RUN_TEST(test_circuit_builder_grows_owned_arrays);
    RUN_TEST(test_arena_grows_without_invalidating_prior_allocations);
    RUN_TEST(test_trace_declarations);
    RUN_TEST(test_verification_header_declarations);
    return UNITY_END();
}
