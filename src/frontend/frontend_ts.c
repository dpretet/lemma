#include "frontend_internal.h"
#include "lemma/parser.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Convert a syntax node's byte offsets and point coordinates to an IR span.
lemma_ir_source_span frontend_span_for_node(TSNode node)
{
    TSPoint start = ts_node_start_point(node);
    TSPoint end = ts_node_end_point(node);
    lemma_ir_source_span span = {
        (size_t)ts_node_start_byte(node), (size_t)ts_node_end_byte(node),
        (size_t)start.row, (size_t)start.column,
        (size_t)end.row, (size_t)end.column
    };
    return span;
}

// Reset an optional diagnostic to its zero-initialized state.
void frontend_clear_diagnostic(lemma_frontend_diagnostic *diagnostic)
{
    if (diagnostic != NULL) {
        memset(diagnostic, 0, sizeof(*diagnostic));
    }
}

// Populate an optional diagnostic and attach a span when the node is non-null.
void frontend_set_diagnostic(lemma_frontend_diagnostic *diagnostic,
                           lemma_frontend_stage stage, const char *code,
                           const char *message, TSNode node)
{
    if (diagnostic == NULL) {
        return;
    }
    diagnostic->stage = stage;
    diagnostic->code = code;
    (void)snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    if (!ts_node_is_null(node)) {
        diagnostic->has_span = 1;
        diagnostic->span = frontend_span_for_node(node);
    }
}

// Compare a node's exact source slice with a NUL-terminated string.
int frontend_node_text_equals(const lemma_parsed_design *parsed, TSNode node,
                            const char *text)
{
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    size_t text_length = strlen(text);
    return (size_t)(end - start) == text_length &&
           memcmp(parsed->source + start, text, text_length) == 0;
}

/* Find the first depth-first descendant of the requested type and count matches.
 * The starting node is excluded; count is incremented and must be non-NULL.
 */
TSNode frontend_find_descendant(TSNode node, const char *type, size_t *count)
{
    TSNode match = {0};
    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t index = 0U; index < child_count; ++index) {
        TSNode child = ts_node_child(node, index);
        if (strcmp(ts_node_type(child), type) == 0) {
            if (*count == 0U) {
                match = child;
            }
            ++*count;
        }
        TSNode nested = frontend_find_descendant(child, type, count);
        if (ts_node_is_null(match) && !ts_node_is_null(nested)) {
            match = nested;
        }
    }
    return match;
}

// Return the first error or missing syntax node in depth-first traversal order.
static TSNode first_syntax_error(TSNode node)
{
    if (ts_node_is_error(node) || ts_node_is_missing(node)) {
        return node;
    }

    uint32_t child_count = ts_node_child_count(node);
    for (uint32_t index = 0U; index < child_count; ++index) {
        TSNode child = first_syntax_error(ts_node_child(node, index));
        if (!ts_node_is_null(child)) {
            return child;
        }
    }
    return (TSNode){0};
}

/* Parse non-NULL borrowed source bytes whose length fits uint32_t and allows a
 * trailing NUL into an owned design. diagnostic may be NULL and is cleared on
 * entry; invalid input, parser/syntax failure, or allocation failure returns
 * NULL and reports the failure when a diagnostic is supplied.
 */
lemma_parsed_design *lemma_frontend_parse(const char *source, size_t length,
                                          lemma_frontend_diagnostic *diagnostic)
{
    frontend_clear_diagnostic(diagnostic);
    // Check source bounds before allocating its owned, NUL-terminated copy.
    if (source == NULL || (uintmax_t)length > (uintmax_t)UINT32_MAX ||
        length == SIZE_MAX) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_PARSING,
                       "parse.invalid_input", "source is null or too large", (TSNode){0});
        return NULL;
    }

    // Parse the owned source, then reject any recovered syntax errors.
    lemma_parsed_design *parsed = calloc(1U, sizeof(*parsed));
    if (parsed == NULL) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_PARSING,
                       "parse.out_of_memory", "unable to allocate parse result", (TSNode){0});
        return NULL;
    }
    parsed->source = malloc(length + 1U);
    if (parsed->source == NULL) {
        free(parsed);
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_PARSING,
                       "parse.out_of_memory", "unable to copy source buffer", (TSNode){0});
        return NULL;
    }
    memcpy(parsed->source, source, length);
    parsed->source[length] = '\0';
    parsed->tree = lemma_parse_source(parsed->source, length);
    if (parsed->tree == NULL) {
        lemma_parsed_design_destroy(parsed);
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_PARSING,
                       "parse.failure", "parser could not process source", (TSNode){0});
        return NULL;
    }

    TSNode root = ts_tree_root_node(parsed->tree);
    if (ts_node_has_error(root)) {
        TSNode error = first_syntax_error(root);
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_PARSING,
                       "parse.syntax_error", "SystemVerilog syntax error", error);
        lemma_parsed_design_destroy(parsed);
        return NULL;
    }
    return parsed;
}

// Destroy the parsed design and its owned source/tree; NULL is accepted.
void lemma_parsed_design_destroy(lemma_parsed_design *parsed)
{
    if (parsed != NULL) {
        if (parsed->tree != NULL) {
            ts_tree_delete(parsed->tree);
        }
        free(parsed->source);
        free(parsed);
    }
}
