#include "unity.h"

void test_parses_valid_systemverilog(void);
void test_reports_malformed_systemverilog(void);
void test_arena_grows_without_invalidating_prior_allocations(void);
void test_trace_declarations(void);
void test_verification_header_declarations(void);

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parses_valid_systemverilog);
    RUN_TEST(test_reports_malformed_systemverilog);
    RUN_TEST(test_arena_grows_without_invalidating_prior_allocations);
    RUN_TEST(test_trace_declarations);
    RUN_TEST(test_verification_header_declarations);
    return UNITY_END();
}
