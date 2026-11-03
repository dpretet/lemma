#ifndef LEMMA_FRONTEND_INTERNAL_H
#define LEMMA_FRONTEND_INTERNAL_H

#include "frontend.h"

#include <tree_sitter/api.h>

typedef struct lemma_resolved_signal {
    uint32_t name_start;
    uint32_t name_end;
    uint32_t width;
    int is_signed;
    lemma_ir_port_direction direction;
    lemma_ir_source_span source;
} lemma_resolved_signal;

typedef struct lemma_resolved_driver {
    uint32_t target_signal_id;
    uint32_t source_signal_id;
    lemma_ir_source_span source;
    lemma_ir_source_span expression_source;
} lemma_resolved_driver;

// Parsed designs own their copied source and Tree-sitter tree.
struct lemma_parsed_design {
    char *source;
    TSTree *tree;
};

// Elaborations own resolved facts and retain source offsets into the parsed design.
struct lemma_elaborated_design {
    uint32_t module_name_start;
    uint32_t module_name_end;
    size_t signal_count;
    lemma_resolved_signal *signals;
    size_t driver_count;
    lemma_resolved_driver *drivers;
};

// Convert a live syntax node's offsets and points to its IR source span.
lemma_ir_source_span frontend_span_for_node(TSNode node);

// Clear an optional diagnostic; NULL is accepted.
void frontend_clear_diagnostic(lemma_frontend_diagnostic *diagnostic);

/* Set an optional diagnostic; code/message must be non-NULL, code must outlive
 * diagnostic use, and node may be null or a live syntax node.
 */
void frontend_set_diagnostic(lemma_frontend_diagnostic *diagnostic,
                             lemma_frontend_stage stage, const char *code,
                             const char *message, TSNode node);

// Compare a live parsed node's source slice against non-NULL text.
int frontend_node_text_equals(const lemma_parsed_design *parsed, TSNode node,
                              const char *text);

/* Find/count matching descendants excluding node; count must be non-NULL and
 * initialized by the caller.
 */
TSNode frontend_find_descendant(TSNode node, const char *type, size_t *count);

#endif
