#ifndef LEMMA_FRONTEND_TYPES_H
#define LEMMA_FRONTEND_TYPES_H

#include "lemma/source_range.h"

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
    int has_source_range;
    lemma_source_range source_range;
} lemma_frontend_diagnostic;

#endif
