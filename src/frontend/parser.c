#include "lemma/frontend_parse.h"
#include "lemma/frontend_ts_internal.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <tree_sitter/tree-sitter-systemverilog.h>

//--------------------------------------------------------
// Reset an optional diagnostic before a parsing operation.
//
// @diagnostic: Optional output record cleared when non-NULL.
// @returns: None.
//--------------------------------------------------------
static void clear_diagnostic(lemma_frontend_diagnostic *diagnostic)
{
    if (diagnostic != NULL) {
        memset(diagnostic, 0, sizeof(*diagnostic));
    }
}

//--------------------------------------------------------
// Report a parsing-stage diagnostic with an optional source span.
//
// @diagnostic: Optional record receiving the diagnostic.
// @code: Static diagnostic code string.
// @message: Concise diagnostic message.
// @node: Node supplying a span, or a null node when unavailable.
// @returns: None.
//--------------------------------------------------------
static void set_parse_diagnostic(lemma_frontend_diagnostic *diagnostic,
                                const char *code, const char *message,
                                TSNode node)
{
    if (diagnostic == NULL) {
        return;
    }
    diagnostic->stage = LEMMA_FRONTEND_STAGE_PARSING;
    diagnostic->code = code;
    (void)snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    if (!ts_node_is_null(node)) {
        lemma_ir_source_span span;
        if (lemma_frontend_ts_node_span(node, &span) == LEMMA_TS_SPAN_VALID) {
            diagnostic->has_span = 1;
            diagnostic->span = span;
        }
    }
}

//--------------------------------------------------------
// Locate the first parser error or missing node in source order.
//
// @node: Root or descendant node inspected recursively.
// @returns: Earliest error node, or a null node when none exists.
//--------------------------------------------------------
static TSNode first_syntax_error(TSNode node)
{
    if (ts_node_is_error(node) || ts_node_is_missing(node)) {
        return node;
    }
    uint32_t count = ts_node_child_count(node);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode found = first_syntax_error(ts_node_child(node, index));
        if (!ts_node_is_null(found)) {
            return found;
        }
    }
    return (TSNode){0};
}

//--------------------------------------------------------
// Parse a borrowed source buffer into a caller-owned tree handle.
//
// @source: Non-NULL source bytes borrowed until handle destruction.
// @length: Source byte count within Tree-sitter's uint32_t limit.
// @diagnostic: Optional parsing diagnostic cleared on entry.
// @returns: Caller-owned parsed-source handle, or NULL for invalid input,
//     syntax failure, parser failure, or allocation failure.
//--------------------------------------------------------
lemma_parsed_source *lemma_frontend_parse(const char *source, size_t length,
                                          lemma_frontend_diagnostic *diagnostic)
{
    clear_diagnostic(diagnostic);
    if (source == NULL || length > UINT32_MAX) {
        set_parse_diagnostic(diagnostic, "parse.invalid_input",
                             "source is NULL or exceeds parser input limits", (TSNode){0});
        return NULL;
    }

    TSParser *parser = ts_parser_new();
    if (parser == NULL) {
        set_parse_diagnostic(diagnostic, "parse.out_of_memory",
                             "unable to allocate parser", (TSNode){0});
        return NULL;
    }
    if (!ts_parser_set_language(parser, tree_sitter_systemverilog())) {
        ts_parser_delete(parser);
        set_parse_diagnostic(diagnostic, "parse.language", "unable to load grammar",
                             (TSNode){0});
        return NULL;
    }
    TSTree *tree = ts_parser_parse_string(parser, NULL, source, (uint32_t)length);
    ts_parser_delete(parser);
    if (tree == NULL) {
        set_parse_diagnostic(diagnostic, "parse.failed", "parser could not process source",
                             (TSNode){0});
        return NULL;
    }

    TSNode root = ts_tree_root_node(tree);
    if (ts_node_has_error(root)) {
        TSNode error = first_syntax_error(root);
        set_parse_diagnostic(diagnostic, "parse.syntax", "source contains a syntax error", error);
        ts_tree_delete(tree);
        return NULL;
    }
    lemma_parsed_source *parsed = calloc(1U, sizeof(*parsed));
    if (parsed == NULL) {
        ts_tree_delete(tree);
        set_parse_diagnostic(diagnostic, "parse.out_of_memory",
                             "unable to allocate parsed-source handle", (TSNode){0});
        return NULL;
    }
    parsed->source = source;
    parsed->source_length = length;
    parsed->tree = tree;
    return parsed;
}

//--------------------------------------------------------
// Destroy a parsed-source handle and its Tree-sitter tree.
//
// @parsed: Handle to destroy, or NULL.
// @returns: None.
//--------------------------------------------------------
void lemma_parsed_source_destroy(lemma_parsed_source *parsed)
{
    if (parsed != NULL) {
        ts_tree_delete(parsed->tree);
        free(parsed);
    }
}
