#ifndef LEMMA_FRONTEND_TS_INTERNAL_H
#define LEMMA_FRONTEND_TS_INTERNAL_H

// Internal frontend state; Tree-sitter types are confined to this header.
#include "frontend_parse.h"
#include "frontend_syntax.h"

#include <stddef.h>
#include <stdint.h>
#include <tree_sitter/api.h>

// Internal Tree-sitter state for a parsed handle; source is borrowed through
// handle destruction, while the tree is owned by the handle.
struct lemma_parsed_source {
    const char *source;
    size_t source_length;
    TSTree *tree;
};

// Selects which immediate children a helper considers.
typedef enum lemma_ts_child_filter {
    LEMMA_TS_CHILD_FILTER_ALL,
    LEMMA_TS_CHILD_FILTER_NAMED,
    LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS
} lemma_ts_child_filter;

// The visitor chooses whether direct-child traversal continues, stops, or fails.
typedef enum lemma_ts_child_visit_action {
    LEMMA_TS_CHILD_VISIT_CONTINUE,
    LEMMA_TS_CHILD_VISIT_STOP,
    LEMMA_TS_CHILD_VISIT_FAIL
} lemma_ts_child_visit_action;

// Result distinguishes full traversal, requested stop, failure, and bad input.
typedef enum lemma_ts_child_visit_result {
    LEMMA_TS_CHILD_VISIT_COMPLETE,
    LEMMA_TS_CHILD_VISIT_STOPPED,
    LEMMA_TS_CHILD_VISIT_FAILED,
    LEMMA_TS_CHILD_VISIT_INVALID_INPUT
} lemma_ts_child_visit_result;

// Result of looking up a direct child by type.
typedef enum lemma_ts_child_type_result {
    LEMMA_TS_CHILD_TYPE_MISSING,
    LEMMA_TS_CHILD_TYPE_UNIQUE,
    LEMMA_TS_CHILD_TYPE_AMBIGUOUS,
    LEMMA_TS_CHILD_TYPE_INVALID_INPUT
} lemma_ts_child_type_result;

// Result of looking up a direct child by its grammar field name.
typedef enum lemma_ts_child_field_result {
    LEMMA_TS_CHILD_FIELD_MISSING,
    LEMMA_TS_CHILD_FIELD_FOUND,
    LEMMA_TS_CHILD_FIELD_INVALID_INPUT
} lemma_ts_child_field_result;

// Result of obtaining a Tree-sitter node's source span.
typedef enum lemma_ts_span_result {
    LEMMA_TS_SPAN_VALID,
    LEMMA_TS_SPAN_INVALID_INPUT
} lemma_ts_span_result;

// Result of obtaining a checked borrowed source slice for a Tree-sitter node.
typedef enum lemma_ts_text_result {
    LEMMA_TS_TEXT_VALID,
    LEMMA_TS_TEXT_INVALID_INPUT
} lemma_ts_text_result;

//--------------------------------------------------------
// Handle one selected direct child synchronously; child and field name are
// borrowed and must not be retained after the callback.
//
// @child: Borrowed direct-child node view; valid only during the callback.
// @child_index: Original zero-based index among all direct children.
// @field_name: Borrowed grammar field name valid during the callback, or NULL.
// @context: Caller-provided context borrowed for the duration of the visit.
// @returns: CONTINUE to visit the next selected child, STOP to end traversal
//     normally, or FAIL to report a failed visit.
//--------------------------------------------------------
typedef lemma_ts_child_visit_action (*lemma_ts_child_visitor)(
    TSNode child, uint32_t child_index, const char *field_name, void *context);

//--------------------------------------------------------
// Visit selected direct children in source order without allocation; ALL selects
// every child, NAMED selects named children, and NAMED_EXCLUDING_EXTRAS selects
// named non-extra children. The parent tree and context are borrowed for the call.
//
// @parent: Non-null node whose immediate children are visited; its tree is borrowed.
// @filter: Valid child filter selecting all, named, or named non-extra children.
// @visitor: Non-NULL callback invoked synchronously for each selected child.
// @context: Caller-provided value borrowed for the duration of the visit.
// @returns: COMPLETE after full traversal, STOPPED after a requested stop,
//     FAILED after callback failure or an invalid callback action, or
//     INVALID_INPUT for a null parent, null visitor, or invalid filter.
//--------------------------------------------------------
lemma_ts_child_visit_result lemma_frontend_ts_visit_children(
    TSNode parent, lemma_ts_child_filter filter,
    lemma_ts_child_visitor visitor, void *context);

//--------------------------------------------------------
// Find a unique direct child by exact node type without allocation; initialize
// the output to a null node before validating inputs.
//
// @parent: Non-null node whose immediate children are searched.
// @type: Non-empty NUL-terminated exact Tree-sitter node type string.
// @filter: Valid child filter selecting all, named, or named non-extra children.
// @child: Required output set to a null node on entry and unless one match exists.
// @returns: UNIQUE and stores the matching node, MISSING when none match,
//     AMBIGUOUS when more than one matches, or INVALID_INPUT for invalid arguments.
//--------------------------------------------------------
lemma_ts_child_type_result lemma_frontend_ts_unique_child_by_type(
    TSNode parent, const char *type, lemma_ts_child_filter filter,
    TSNode *child);

//--------------------------------------------------------
// Find the first direct child by exact grammar field name without allocation;
// initialize the output to a null node before validating inputs.
//
// @parent: Non-null node whose immediate children are searched.
// @field_name: Non-empty NUL-terminated grammar field name.
// @child: Required output set to a null node on entry and unless a child is found.
// @returns: FOUND and stores the first matching child, MISSING when no direct
//     child has that field, or INVALID_INPUT for invalid arguments.
//--------------------------------------------------------
lemma_ts_child_field_result lemma_frontend_ts_child_by_field_name(
    TSNode parent, const char *field_name, TSNode *child);

//--------------------------------------------------------
// Obtain a node's half-open byte and zero-based row/column span without allocation.
//
// @node: Non-null Tree-sitter node whose span is requested.
// @span: Required output initialized to a zero span on entry.
// @returns: VALID with the node's half-open source span, or INVALID_INPUT for
//     a null node, NULL output, or unrepresentable position.
//--------------------------------------------------------
lemma_ts_span_result lemma_frontend_ts_node_span(
    TSNode node, lemma_ir_source_span *span);

//--------------------------------------------------------
// Form a checked borrowed source slice for a node in parsed's tree without
// allocation; the slice remains valid only while parsed's source is alive.
//
// @parsed: Live parsed-source handle whose source and tree are borrowed.
// @node: Non-null node belonging to parsed's tree.
// @text: Required output initialized to an empty zero slice on entry.
// @returns: VALID with a borrowed source slice and span, or INVALID_INPUT for
//     invalid arguments, a foreign/null node, or an out-of-range node span.
//--------------------------------------------------------
lemma_ts_text_result lemma_frontend_ts_node_text(
    const lemma_parsed_source *parsed, TSNode node, lemma_syntax_text *text);

#endif
