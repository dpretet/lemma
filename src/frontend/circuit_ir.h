#ifndef LEMMA_CIRCUIT_IR_H
#define LEMMA_CIRCUIT_IR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct lemma_ir_source_span {
    size_t start_byte;
    size_t end_byte;
    size_t start_row;
    size_t start_column;
    size_t end_row;
    size_t end_column;
} lemma_ir_source_span;

typedef enum lemma_ir_port_direction {
    LEMMA_IR_PORT_INPUT,
    LEMMA_IR_PORT_OUTPUT
} lemma_ir_port_direction;

typedef struct lemma_ir_signal {
    char *name;
    uint32_t width;
    bool is_signed;
    bool is_port;
    lemma_ir_source_span source;
} lemma_ir_signal;

typedef struct lemma_ir_port {
    uint32_t signal_id;
    lemma_ir_port_direction direction;
} lemma_ir_port;

typedef enum lemma_ir_expression_kind {
    LEMMA_IR_EXPRESSION_READ_SIGNAL
} lemma_ir_expression_kind;

typedef struct lemma_ir_expression {
    lemma_ir_expression_kind kind;
    uint32_t signal_id;
    uint32_t width;
    bool is_signed;
    lemma_ir_source_span source;
} lemma_ir_expression;

typedef struct lemma_ir_driver {
    uint32_t target_signal_id;
    uint32_t expression_id;
    lemma_ir_source_span source;
} lemma_ir_driver;

typedef struct lemma_ir_ffd {
    uint32_t q_signal_id;
    uint32_t d_expression_id;
    lemma_ir_source_span source;
} lemma_ir_ffd;

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

// Destroys the circuit and every allocation owned by it. NULL is accepted.
void lemma_circuit_destroy(lemma_circuit *circuit);

#endif
