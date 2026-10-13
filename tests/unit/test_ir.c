#include "lemma/ir.h"

#include "unity.h"

void test_ir_accepts_flat_circuit_with_comb_logic_and_ffd(void)
{
    static const LemmaIRSignal signals[] = {
        {.name = "clk", .width = 1U},
        {.name = "data", .width = 1U},
        {.name = "state", .width = 1U},
        {.name = "out", .width = 1U},
        {.name = "reset_n", .width = 1U},
    };
    static const LemmaIRPort ports[] = {
        {.signal = 0U, .direction = LEMMA_IR_PORT_INPUT},
        {.signal = 1U, .direction = LEMMA_IR_PORT_INPUT},
        {.signal = 3U, .direction = LEMMA_IR_PORT_OUTPUT},
        {.signal = 4U, .direction = LEMMA_IR_PORT_INPUT},
    };
    static const LemmaIRExpr expressions[] = {
        {.width = 1U, .kind = LEMMA_IR_EXPR_SIGNAL, .value.signal = 1U},
        {.width = 1U, .kind = LEMMA_IR_EXPR_SIGNAL, .value.signal = 2U},
    };
    static const uint64_t zero_word[] = {0U};
    static const uint64_t known_bit[] = {1U};
    static const LemmaIRCombDriver drivers[] = {
        {.target = 3U, .value = 1U},
    };
    static const LemmaIRFFD ffds[] = {
        {.q = 2U,
         .d = 0U,
         .clock = 0U,
         .active_edge = LEMMA_IR_EDGE_POSEDGE,
         .async_reset = {
             .present = true,
             .signal = 4U,
             .asserted_edge = LEMMA_IR_EDGE_NEGEDGE,
             .value = {.words = zero_word,
                       .known_mask = known_bit,
                       .word_count = 1U},
         },
         .has_initial_value = true,
         .initial_value = {.words = zero_word,
                           .known_mask = known_bit,
                           .word_count = 1U}},
    };
    const LemmaIRCircuit circuit = {
        .module_name = "smoke",
        .ports = ports,
        .port_count = sizeof(ports) / sizeof(ports[0]),
        .signals = signals,
        .signal_count = sizeof(signals) / sizeof(signals[0]),
        .expressions = expressions,
        .expression_count = sizeof(expressions) / sizeof(expressions[0]),
        .comb_drivers = drivers,
        .comb_driver_count = sizeof(drivers) / sizeof(drivers[0]),
        .ffds = ffds,
        .ffd_count = sizeof(ffds) / sizeof(ffds[0]),
    };
    LemmaIRDiagnostic diagnostic;

    TEST_ASSERT_EQUAL_INT(LEMMA_IR_STATUS_OK,
                         lemma_ir_validate(&circuit, &diagnostic));
    TEST_ASSERT_EQUAL_INT(LEMMA_IR_STATUS_OK, diagnostic.status);
}

void test_ir_rejects_ffd_with_mismatched_d_width(void)
{
    static const LemmaIRSignal signals[] = {
        {.name = "clk", .width = 1U},
        {.name = "q", .width = 2U},
    };
    static const LemmaIRExpr expressions[] = {
        {.width = 1U, .kind = LEMMA_IR_EXPR_SIGNAL, .value.signal = 0U},
    };
    static const LemmaIRFFD ffds[] = {
        {.q = 1U,
         .d = 0U,
         .clock = 0U,
         .active_edge = LEMMA_IR_EDGE_POSEDGE},
    };
    const LemmaIRCircuit circuit = {
        .module_name = "bad_width",
        .signals = signals,
        .signal_count = sizeof(signals) / sizeof(signals[0]),
        .expressions = expressions,
        .expression_count = sizeof(expressions) / sizeof(expressions[0]),
        .ffds = ffds,
        .ffd_count = sizeof(ffds) / sizeof(ffds[0]),
    };
    LemmaIRDiagnostic diagnostic;

    TEST_ASSERT_EQUAL_INT(LEMMA_IR_STATUS_WIDTH_MISMATCH,
                         lemma_ir_validate(&circuit, &diagnostic));
    TEST_ASSERT_EQUAL_INT(LEMMA_IR_OBJECT_FFD, diagnostic.object_kind);
    TEST_ASSERT_EQUAL_UINT(0U, diagnostic.object_index);
}

void test_ir_rejects_unresolved_expression_reference(void)
{
    static const LemmaIRSignal signals[] = {{.name = "out", .width = 1U}};
    static const LemmaIRExpr expressions[] = {
        {.width = 1U,
         .kind = LEMMA_IR_EXPR_SIGNAL,
         .value.signal = 4U},
    };
    const LemmaIRCircuit circuit = {
        .module_name = "bad_reference",
        .signals = signals,
        .signal_count = sizeof(signals) / sizeof(signals[0]),
        .expressions = expressions,
        .expression_count = sizeof(expressions) / sizeof(expressions[0]),
    };
    LemmaIRDiagnostic diagnostic;

    TEST_ASSERT_EQUAL_INT(LEMMA_IR_STATUS_INVALID_ID,
                         lemma_ir_validate(&circuit, &diagnostic));
    TEST_ASSERT_EQUAL_INT(LEMMA_IR_OBJECT_EXPRESSION, diagnostic.object_kind);
}

void test_ir_rejects_driver_on_input_port(void)
{
    static const LemmaIRSignal signals[] = {{.name = "input", .width = 1U}};
    static const LemmaIRPort ports[] = {
        {.signal = 0U, .direction = LEMMA_IR_PORT_INPUT},
    };
    static const LemmaIRExpr expressions[] = {
        {.width = 1U, .kind = LEMMA_IR_EXPR_SIGNAL, .value.signal = 0U},
    };
    static const LemmaIRCombDriver drivers[] = {
        {.target = 0U, .value = 0U},
    };
    const LemmaIRCircuit circuit = {
        .module_name = "driven_input",
        .ports = ports,
        .port_count = 1U,
        .signals = signals,
        .signal_count = 1U,
        .expressions = expressions,
        .expression_count = 1U,
        .comb_drivers = drivers,
        .comb_driver_count = 1U,
    };

    TEST_ASSERT_EQUAL_INT(LEMMA_IR_STATUS_INPUT_HAS_DRIVER,
                         lemma_ir_validate(&circuit, NULL));
}

void test_ir_rejects_multiple_drivers(void)
{
    static const LemmaIRSignal signals[] = {
        {.name = "data", .width = 1U},
        {.name = "q", .width = 1U},
        {.name = "clk", .width = 1U},
    };
    static const LemmaIRExpr expressions[] = {
        {.width = 1U, .kind = LEMMA_IR_EXPR_SIGNAL, .value.signal = 0U},
    };
    static const LemmaIRCombDriver drivers[] = {
        {.target = 1U, .value = 0U},
    };
    static const LemmaIRFFD ffds[] = {
        {.q = 1U,
         .d = 0U,
         .clock = 2U,
         .active_edge = LEMMA_IR_EDGE_POSEDGE},
    };
    const LemmaIRCircuit circuit = {
        .module_name = "multiple_drivers",
        .signals = signals,
        .signal_count = sizeof(signals) / sizeof(signals[0]),
        .expressions = expressions,
        .expression_count = sizeof(expressions) / sizeof(expressions[0]),
        .comb_drivers = drivers,
        .comb_driver_count = 1U,
        .ffds = ffds,
        .ffd_count = 1U,
    };

    TEST_ASSERT_EQUAL_INT(LEMMA_IR_STATUS_MULTIPLE_DRIVERS,
                         lemma_ir_validate(&circuit, NULL));
}

void test_ir_rejects_combinational_cycle(void)
{
    static const LemmaIRSignal signals[] = {
        {.name = "left", .width = 1U},
        {.name = "right", .width = 1U},
    };
    static const LemmaIRExpr expressions[] = {
        {.width = 1U, .kind = LEMMA_IR_EXPR_SIGNAL, .value.signal = 1U},
        {.width = 1U, .kind = LEMMA_IR_EXPR_SIGNAL, .value.signal = 0U},
    };
    static const LemmaIRCombDriver drivers[] = {
        {.target = 0U, .value = 0U},
        {.target = 1U, .value = 1U},
    };
    const LemmaIRCircuit circuit = {
        .module_name = "combinational_cycle",
        .signals = signals,
        .signal_count = sizeof(signals) / sizeof(signals[0]),
        .expressions = expressions,
        .expression_count = sizeof(expressions) / sizeof(expressions[0]),
        .comb_drivers = drivers,
        .comb_driver_count = sizeof(drivers) / sizeof(drivers[0]),
    };

    TEST_ASSERT_EQUAL_INT(LEMMA_IR_STATUS_COMBINATIONAL_CYCLE,
                         lemma_ir_validate(&circuit, NULL));
}
