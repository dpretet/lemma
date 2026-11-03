#include "unity.h"

// Confirm valid syntax produces a tree without parser errors.
void test_parses_valid_systemverilog(void);
// Confirm malformed syntax is recorded as a tree-sitter error.
void test_reports_malformed_systemverilog(void);
// Build IR for a scalar direct-alias fixture in the initial alias profile.
void test_frontend_builds_scalar_alias_fixture_ir(void);
// Preserve widths and signedness for the profile's vector alias fixtures.
void test_frontend_builds_signed_and_unsigned_vector_alias_fixture_ir(void);
// Reject malformed source during parsing, before RTL validation or IR.
void test_frontend_rejects_malformed_source_at_parse_stage(void);
// Reject constructs outside the initial scalar/vector alias profile.
void test_frontend_rejects_out_of_profile_constructs(void);
// Reject a flat design containing more than one module.
void test_frontend_rejects_multiple_modules(void);
// Reject a design when the requested top module is not present.
void test_frontend_requires_selected_flat_module(void);
// Check arena growth, reset-based reuse, and preservation of prior data.
void test_arena_grows_without_invalidating_prior_allocations(void);
// Exercise record declarations from the public trace header.
void test_trace_declarations(void);
// Exercise record and function declarations from the verification header.
void test_verification_header_declarations(void);

// Unity setup hook; these tests currently need no per-test initialization.
void setUp(void)
{
}

// Unity teardown hook; each test releases any resources it creates itself.
void tearDown(void)
{
}

// Register and run the unit tests; their registration order is not semantic.
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parses_valid_systemverilog);
    RUN_TEST(test_reports_malformed_systemverilog);
    RUN_TEST(test_frontend_builds_scalar_alias_fixture_ir);
    RUN_TEST(test_frontend_builds_signed_and_unsigned_vector_alias_fixture_ir);
    RUN_TEST(test_frontend_rejects_malformed_source_at_parse_stage);
    RUN_TEST(test_frontend_rejects_out_of_profile_constructs);
    RUN_TEST(test_frontend_rejects_multiple_modules);
    RUN_TEST(test_frontend_requires_selected_flat_module);
    RUN_TEST(test_arena_grows_without_invalidating_prior_allocations);
    RUN_TEST(test_trace_declarations);
    RUN_TEST(test_verification_header_declarations);
    return UNITY_END();
}
