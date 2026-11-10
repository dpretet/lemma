#include "lemma/frontend_ts_internal.h"

#include <limits.h>
#include <string.h>

//--------------------------------------------------------
// Apply a child filter to one direct Tree-sitter child.
//
// @child: Direct child considered for selection.
// @filter: Requested child category.
// @returns: Nonzero when the child satisfies the filter.
//--------------------------------------------------------
static int selected_child(TSNode child, lemma_ts_child_filter filter)
{
    if (filter == LEMMA_TS_CHILD_FILTER_ALL) {
        return 1;
    }
    if (!ts_node_is_named(child)) {
        return 0;
    }
    return filter != LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS ||
           !ts_node_is_extra(child);
}

//--------------------------------------------------------
// Visit selected direct children in original source order.
//
// @parent: Non-null node whose immediate children are visited.
// @filter: Valid child filter.
// @visitor: Non-NULL callback invoked synchronously.
// @context: Caller value borrowed during callbacks.
// @returns: COMPLETE, STOPPED, FAILED, or INVALID_INPUT as documented.
//--------------------------------------------------------
lemma_ts_child_visit_result lemma_frontend_ts_visit_children(
    TSNode parent, lemma_ts_child_filter filter,
    lemma_ts_child_visitor visitor, void *context)
{
    if (ts_node_is_null(parent) || visitor == NULL ||
        filter < LEMMA_TS_CHILD_FILTER_ALL ||
        filter > LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS) {
        return LEMMA_TS_CHILD_VISIT_INVALID_INPUT;
    }
    uint32_t count = ts_node_child_count(parent);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode child = ts_node_child(parent, index);
        if (!selected_child(child, filter)) {
            continue;
        }
        const char *field = ts_node_field_name_for_child(parent, index);
        lemma_ts_child_visit_action action = visitor(child, index, field, context);
        if (action == LEMMA_TS_CHILD_VISIT_STOP) {
            return LEMMA_TS_CHILD_VISIT_STOPPED;
        }
        if (action != LEMMA_TS_CHILD_VISIT_CONTINUE) {
            return LEMMA_TS_CHILD_VISIT_FAILED;
        }
    }
    return LEMMA_TS_CHILD_VISIT_COMPLETE;
}

//--------------------------------------------------------
// Find a unique immediate child with the requested exact type.
//
// @parent: Non-null parent node.
// @type: Nonempty exact node type.
// @filter: Valid child filter.
// @child: Required output initialized to a null node.
// @returns: UNIQUE, MISSING, AMBIGUOUS, or INVALID_INPUT.
//--------------------------------------------------------
lemma_ts_child_type_result lemma_frontend_ts_unique_child_by_type(
    TSNode parent, const char *type, lemma_ts_child_filter filter, TSNode *child)
{
    if (child != NULL) {
        *child = (TSNode){0};
    }
    if (child == NULL || type == NULL || type[0] == '\0' || ts_node_is_null(parent) ||
        filter < LEMMA_TS_CHILD_FILTER_ALL ||
        filter > LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS) {
        return LEMMA_TS_CHILD_TYPE_INVALID_INPUT;
    }
    size_t matches = 0U;
    TSNode found = {0};
    uint32_t count = ts_node_child_count(parent);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode candidate = ts_node_child(parent, index);
        if (selected_child(candidate, filter) &&
            strcmp(ts_node_type(candidate), type) == 0) {
            if (matches == 0U) {
                found = candidate;
            }
            ++matches;
        }
    }
    if (matches == 0U) {
        return LEMMA_TS_CHILD_TYPE_MISSING;
    }
    if (matches != 1U) {
        return LEMMA_TS_CHILD_TYPE_AMBIGUOUS;
    }
    *child = found;
    return LEMMA_TS_CHILD_TYPE_UNIQUE;
}

//--------------------------------------------------------
// Find the first immediate child bearing an exact grammar field name.
//
// @parent: Non-null parent node.
// @field_name: Nonempty grammar field name.
// @child: Required output initialized to a null node.
// @returns: FOUND, MISSING, or INVALID_INPUT.
//--------------------------------------------------------
lemma_ts_child_field_result lemma_frontend_ts_child_by_field_name(
    TSNode parent, const char *field_name, TSNode *child)
{
    if (child != NULL) {
        *child = (TSNode){0};
    }
    if (child == NULL || field_name == NULL || field_name[0] == '\0' ||
        ts_node_is_null(parent)) {
        return LEMMA_TS_CHILD_FIELD_INVALID_INPUT;
    }
    uint32_t count = ts_node_child_count(parent);
    for (uint32_t index = 0U; index < count; ++index) {
        const char *field = ts_node_field_name_for_child(parent, index);
        if (field != NULL && strcmp(field, field_name) == 0) {
            *child = ts_node_child(parent, index);
            return LEMMA_TS_CHILD_FIELD_FOUND;
        }
    }
    return LEMMA_TS_CHILD_FIELD_MISSING;
}

//--------------------------------------------------------
// Convert Tree-sitter coordinates to Lemma's half-open range.
//
// @node: Non-null node whose range is requested.
// @range: Required output initialized to zero.
// @returns: VALID when representable; INVALID_INPUT otherwise.
//--------------------------------------------------------
lemma_ts_range_result lemma_frontend_ts_node_range(
    TSNode node, lemma_source_range *range)
{
    if (range != NULL) {
        memset(range, 0, sizeof(*range));
    }
    if (range == NULL || ts_node_is_null(node)) {
        return LEMMA_TS_RANGE_INVALID_INPUT;
    }
    TSPoint start = ts_node_start_point(node);
    TSPoint end = ts_node_end_point(node);
    uint32_t start_byte = ts_node_start_byte(node);
    uint32_t end_byte = ts_node_end_byte(node);
    if ((uintmax_t)start_byte > (uintmax_t)SIZE_MAX ||
        (uintmax_t)end_byte > (uintmax_t)SIZE_MAX ||
        (uintmax_t)start.row > (uintmax_t)SIZE_MAX ||
        (uintmax_t)start.column > (uintmax_t)SIZE_MAX ||
        (uintmax_t)end.row > (uintmax_t)SIZE_MAX ||
        (uintmax_t)end.column > (uintmax_t)SIZE_MAX) {
        memset(range, 0, sizeof(*range));
        return LEMMA_TS_RANGE_INVALID_INPUT;
    }
    range->start_byte = (size_t)start_byte;
    range->end_byte = (size_t)end_byte;
    range->start_row = (size_t)start.row;
    range->start_column = (size_t)start.column;
    range->end_row = (size_t)end.row;
    range->end_column = (size_t)end.column;
    return LEMMA_TS_RANGE_VALID;
}

//--------------------------------------------------------
// Form a checked borrowed slice from a node belonging to parsed's tree.
//
// @parsed: Live handle owning the node's tree and borrowing its source.
// @node: Node whose range is requested.
// @text: Required output initialized to an empty slice.
// @returns: VALID with a borrowed slice, or INVALID_INPUT for bad ranges/inputs.
//--------------------------------------------------------
lemma_ts_text_result lemma_frontend_ts_node_text(
    const lemma_parsed_source *parsed, TSNode node, lemma_syntax_text *text)
{
    if (text != NULL) {
        memset(text, 0, sizeof(*text));
    }
    if (parsed == NULL || parsed->source == NULL || parsed->tree == NULL || text == NULL ||
        ts_node_is_null(node) || node.tree != parsed->tree) {
        return LEMMA_TS_TEXT_INVALID_INPUT;
    }
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    if ((size_t)start > parsed->source_length || (size_t)end > parsed->source_length ||
        end < start || lemma_frontend_ts_node_range(
                           node, &text->source_range) != LEMMA_TS_RANGE_VALID) {
        memset(text, 0, sizeof(*text));
        return LEMMA_TS_TEXT_INVALID_INPUT;
    }
    text->data = parsed->source + start;
    text->length = (size_t)(end - start);
    return LEMMA_TS_TEXT_VALID;
}
