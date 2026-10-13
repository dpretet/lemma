#include "lemma/parser.h"

#include "unity.h"

void test_parses_valid_systemverilog(void)
{
    static const char source[] = "module smoke(input logic clk); endmodule";
    TSTree *tree = lemma_parse_source(source, sizeof(source) - 1U);

    TEST_ASSERT_NOT_NULL(tree);
    TEST_ASSERT_FALSE(ts_node_has_error(ts_tree_root_node(tree)));

    ts_tree_delete(tree);
}

void test_reports_malformed_systemverilog(void)
{
    static const char source[] = "module smoke(input logic clk) endmodule";
    TSTree *tree = lemma_parse_source(source, sizeof(source) - 1U);

    TEST_ASSERT_NOT_NULL(tree);
    TEST_ASSERT_TRUE(ts_node_has_error(ts_tree_root_node(tree)));

    ts_tree_delete(tree);
}
