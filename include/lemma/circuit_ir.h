#ifndef LEMMA_CIRCUIT_IR_H
#define LEMMA_CIRCUIT_IR_H

#include "frontend_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Port directions supported by the private circuit representation.
typedef enum lemma_ir_port_direction {
    LEMMA_IR_PORT_INPUT,
    LEMMA_IR_PORT_OUTPUT
} lemma_ir_port_direction;

// A circuit signal; an owned circuit owns its name string.
typedef struct lemma_ir_signal {
    char *name;
    uint32_t width;
    bool is_signed;
    bool is_port;
    lemma_ir_source_span source;
} lemma_ir_signal;

// Maps an ordered circuit port to its signal by array index.
typedef struct lemma_ir_port {
    uint32_t signal_id;
    lemma_ir_port_direction direction;
} lemma_ir_port;

// Expression forms currently represented by the private circuit IR.
typedef enum lemma_ir_expression_kind {
    LEMMA_IR_EXPRESSION_READ_SIGNAL
} lemma_ir_expression_kind;

// An expression whose signal_id indexes the containing circuit's signals.
typedef struct lemma_ir_expression {
    lemma_ir_expression_kind kind;
    uint32_t signal_id;
    uint32_t width;
    bool is_signed;
    lemma_ir_source_span source;
} lemma_ir_expression;

// A combinational assignment; IDs index arrays in the containing circuit.
typedef struct lemma_ir_driver {
    uint32_t target_signal_id;
    uint32_t expression_id;
    lemma_ir_source_span source;
} lemma_ir_driver;

// A flip-flop assignment; IDs index arrays in the containing circuit.
typedef struct lemma_ir_ffd {
    uint32_t q_signal_id;
    uint32_t d_expression_id;
    lemma_ir_source_span source;
} lemma_ir_ffd;

// One owning IR object; its arrays and strings are released by destroy.
typedef struct lemma_circuit {
    char *module_name;
    lemma_ir_port *ports;
    size_t port_count;
    lemma_ir_signal *signals;
    size_t signal_count;
    lemma_ir_expression *expressions;
    size_t expression_count;
    lemma_ir_driver *drivers;
    size_t driver_count;
    lemma_ir_ffd *ffds;
    size_t ffd_count;
} lemma_circuit;

// Opaque internal owner of circuit storage while elaboration populates it.
typedef struct lemma_circuit_builder lemma_circuit_builder;

//--------------------------------------------------------
// Destroy a circuit and all its owned storage.
//
// @circuit: Circuit to destroy, or NULL.
// @returns: None.
//--------------------------------------------------------
void lemma_circuit_destroy(lemma_circuit *circuit);

#endif
