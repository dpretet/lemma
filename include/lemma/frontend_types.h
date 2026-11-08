#ifndef LEMMA_FRONTEND_TYPES_H
#define LEMMA_FRONTEND_TYPES_H

#include <stddef.h>

// Source location with half-open byte offsets and zero-based row/column values.
typedef struct lemma_ir_source_span {
    size_t start_byte;
    size_t end_byte;
    size_t start_row;
    size_t start_column;
    size_t end_row;
    size_t end_column;
} lemma_ir_source_span;

// Frontend processing stage reported by a diagnostic.
typedef enum lemma_frontend_stage {
    LEMMA_FRONTEND_STAGE_PARSING,
    LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
    LEMMA_FRONTEND_STAGE_CIRCUIT_IR
} lemma_frontend_stage;

// Diagnostic output; code is implementation-owned and remains valid while
// inspected, while message is stored inline.
typedef struct lemma_frontend_diagnostic {
    lemma_frontend_stage stage;
    const char *code;
    char message[192];
    int has_span;
    lemma_ir_source_span span;
} lemma_frontend_diagnostic;

#endif
