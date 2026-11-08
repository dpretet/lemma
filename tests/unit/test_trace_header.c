#include "unity.h"

#include "lemma/trace.h"

//--------------------------------------------------------
// Check aggregate-initialized trace fields for width, event kind and edge,
// transition count, and nested signal name. Referenced data stays live through
// the assertions; this header-only test checks fields, not ABI offsets.
//
// @returns: None.
//--------------------------------------------------------
void test_trace_declarations(void)
{
    const unsigned char bytes[] = {0x05U};
    const lemma_bit_vector bit_vector = {3U, false, bytes};
    const lemma_trace_value pre_event_values[] = {
        {"state", bit_vector}
    };
    const lemma_trace_value post_event_values[] = {
        {"state", bit_vector}
    };
    const lemma_trace_event clock_event = {
        LEMMA_TRACE_EVENT_CLOCK_EDGE,
        "clock",
        LEMMA_TRACE_EDGE_POSEDGE
    };
    const lemma_trace_event reset_event = {
        LEMMA_TRACE_EVENT_ASYNC_RESET,
        "reset_n",
        LEMMA_TRACE_EDGE_NEGEDGE
    };
    const lemma_trace_transition transition = {
        clock_event,
        1U,
        pre_event_values,
        post_event_values
    };
    const lemma_trace trace = {1U, &transition};

    TEST_ASSERT_EQUAL_UINT(3U, bit_vector.width_bits);
    TEST_ASSERT_EQUAL(LEMMA_TRACE_EVENT_ASYNC_RESET, reset_event.kind);
    TEST_ASSERT_EQUAL(LEMMA_TRACE_EDGE_NEGEDGE, reset_event.edge);
    TEST_ASSERT_EQUAL_UINT(1U, trace.transition_count);
    TEST_ASSERT_EQUAL_STRING("state", trace.transitions[0].pre_event_values[0].signal_name);
}
