#include "unity.h"

void test_parses_valid_systemverilog(void);
void test_reports_malformed_systemverilog(void);
void test_arena_grows_without_invalidating_prior_allocations(void);
void test_ir_accepts_flat_circuit_with_comb_logic_and_ffd(void);
void test_ir_rejects_ffd_with_mismatched_d_width(void);
void test_ir_rejects_unresolved_expression_reference(void);
void test_ir_rejects_driver_on_input_port(void);
void test_ir_rejects_multiple_drivers(void);
void test_ir_rejects_combinational_cycle(void);

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
    RUN_TEST(test_ir_accepts_flat_circuit_with_comb_logic_and_ffd);
    RUN_TEST(test_ir_rejects_ffd_with_mismatched_d_width);
    RUN_TEST(test_ir_rejects_unresolved_expression_reference);
    RUN_TEST(test_ir_rejects_driver_on_input_port);
    RUN_TEST(test_ir_rejects_multiple_drivers);
    RUN_TEST(test_ir_rejects_combinational_cycle);
    return UNITY_END();
}
