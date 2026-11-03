#ifndef LEMMA_FRONTEND_H
#define LEMMA_FRONTEND_H

#include <stddef.h>

#include "circuit_ir.h"

typedef enum lemma_frontend_stage {
    LEMMA_FRONTEND_STAGE_PARSING,
    LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
    LEMMA_FRONTEND_STAGE_CIRCUIT_IR
} lemma_frontend_stage;

typedef struct lemma_frontend_diagnostic {
    lemma_frontend_stage stage;
    const char *code;
    char message[192];
    int has_span;
    lemma_ir_source_span span;
} lemma_frontend_diagnostic;

typedef struct lemma_parsed_design lemma_parsed_design;
typedef struct lemma_elaborated_design lemma_elaborated_design;

/* Parses source bytes; source must be non-NULL, length must fit uint32_t and
 * allow a trailing NUL. The source is borrowed. diagnostic may be NULL and
 * is cleared on entry. Returns an owned design, or NULL on invalid input,
 * syntax/parser failure, or allocation failure.
 */
lemma_parsed_design *lemma_frontend_parse(const char *source, size_t length,
                                          lemma_frontend_diagnostic *diagnostic);

// Destroys the parsed design and its owned source/tree; NULL is accepted.
void lemma_parsed_design_destroy(lemma_parsed_design *parsed);

/* parsed must be a live design returned by lemma_frontend_parse; top_name must
 * be a non-NULL NUL-terminated string. Both inputs are borrowed. diagnostic
 * may be NULL and is cleared on entry. Returns an owned elaboration, or NULL
 * on invalid input, unsupported RTL, or allocation failure.
 */
lemma_elaborated_design *lemma_frontend_elaborate(
    const lemma_parsed_design *parsed, const char *top_name,
    lemma_frontend_diagnostic *diagnostic);

// Destroys the elaboration and its owned storage; NULL is accepted.
void lemma_frontend_elaborate_destroy(lemma_elaborated_design *elaborated);

/* parsed and elaborated must be live results from parsing and elaboration;
 * elaborated must come from this same parsed design (this is not checked).
 * Both inputs are borrowed. diagnostic may be NULL and is cleared on entry.
 * Returns an owned circuit, or NULL on invalid/oversized input or allocation
 * failure.
 */
lemma_circuit *lemma_frontend_build_ir(
    const lemma_parsed_design *parsed,
    const lemma_elaborated_design *elaborated,
    lemma_frontend_diagnostic *diagnostic);

#endif
