#ifndef LEMMA_TRACE_H
#define LEMMA_TRACE_H

#include <stdbool.h>
#include <stddef.h>

/*
 * An explicitly sized bit vector. Bytes are packed little-endian: byte zero
 * contains the least-significant bits, and bit zero is the least-significant
 * bit of its byte. The byte array length is ceil(width_bits / 8); unused high
 * bits in the last byte are zero.
 *
 * Ownership depends on use: callers provide parameter-value bytes, while
 * trace-value bytes are owned by the containing result.
 */
typedef struct lemma_bit_vector {
    size_t width_bits;
    bool is_signed;
    const unsigned char *bytes;
} lemma_bit_vector;

typedef enum lemma_trace_event_kind {
    LEMMA_TRACE_EVENT_CLOCK_EDGE,
    LEMMA_TRACE_EVENT_ASYNC_RESET
} lemma_trace_event_kind;

typedef enum lemma_trace_edge {
    LEMMA_TRACE_EDGE_POSEDGE,
    LEMMA_TRACE_EDGE_NEGEDGE
} lemma_trace_edge;

/*
 * Trace storage, including signal names and bit-vector bytes, is owned by the
 * containing result and remains valid until that result is destroyed.
 */
typedef struct lemma_trace_event {
    lemma_trace_event_kind kind;
    const char *signal_name;
    lemma_trace_edge edge;
} lemma_trace_event;

typedef struct lemma_trace_value {
    const char *signal_name;
    lemma_bit_vector value;
} lemma_trace_value;

typedef struct lemma_trace_transition {
    lemma_trace_event event;
    size_t value_count;
    const lemma_trace_value *pre_event_values;
    const lemma_trace_value *post_event_values;
} lemma_trace_transition;

/*
 * Each transition has value_count entries in both valuation arrays. At clock
 * edges, pre_event_values are the SVA sample. Asynchronous-reset transitions
 * are included in the trace but are not SVA samples.
 */
typedef struct lemma_trace {
    size_t transition_count;
    const lemma_trace_transition *transitions;
} lemma_trace;

#endif
