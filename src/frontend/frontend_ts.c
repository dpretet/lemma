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
        if (selected_child(candidate, filter) && strcmp(ts_node_type(candidate), type) == 0) {
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
// Convert Tree-sitter coordinates to Lemma's half-open span.
//
// @node: Non-null node whose range is requested.
// @span: Required output initialized to zero.
// @returns: VALID when representable; INVALID_INPUT otherwise.
//--------------------------------------------------------
lemma_ts_span_result lemma_frontend_ts_node_span(TSNode node,
                                                lemma_ir_source_span *span)
{
    if (span != NULL) {
        memset(span, 0, sizeof(*span));
    }
    if (span == NULL || ts_node_is_null(node)) {
        return LEMMA_TS_SPAN_INVALID_INPUT;
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
        memset(span, 0, sizeof(*span));
        return LEMMA_TS_SPAN_INVALID_INPUT;
    }
    span->start_byte = (size_t)start_byte;
    span->end_byte = (size_t)end_byte;
    span->start_row = (size_t)start.row;
    span->start_column = (size_t)start.column;
    span->end_row = (size_t)end.row;
    span->end_column = (size_t)end.column;
    return LEMMA_TS_SPAN_VALID;
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
        end < start || lemma_frontend_ts_node_span(node, &text->span) != LEMMA_TS_SPAN_VALID) {
        memset(text, 0, sizeof(*text));
        return LEMMA_TS_TEXT_INVALID_INPUT;
    }
    text->data = parsed->source + start;
    text->length = (size_t)(end - start);
    return LEMMA_TS_TEXT_VALID;
}

typedef struct syntax_walk {
    const lemma_parsed_source *parsed;
    lemma_syntax_visitor visitor;
    void *visitor_context;
    lemma_syntax_visit_result result;
} syntax_walk;

typedef struct range_accumulator {
    const lemma_parsed_source *parsed;
    TSNode first;
    TSNode last;
} range_accumulator;

//--------------------------------------------------------
// Return a borrowed source view for a node, or an empty view for a null node.
//
// @parsed: Live parsed-source handle that owns the tree and borrows the source.
// @node: Node whose source text is requested, or a null node.
// @returns: Borrowed source slice for node, or an empty view on failure.
//--------------------------------------------------------
static lemma_syntax_text syntax_text_for_node(const lemma_parsed_source *parsed,
                                               TSNode node)
{
    lemma_syntax_text text = {0};
    if (!ts_node_is_null(node)) {
        (void)lemma_frontend_ts_node_text(parsed, node, &text);
    }
    return text;
}

//--------------------------------------------------------
// Return a node's span, or a zero span when the node is null or invalid.
//
// @node: Node whose source span is requested, or a null node.
// @returns: Node's half-open span, or a zero span on failure.
//--------------------------------------------------------
static lemma_ir_source_span syntax_span_for_node(TSNode node)
{
    lemma_ir_source_span span = {0};
    (void)lemma_frontend_ts_node_span(node, &span);
    return span;
}

//--------------------------------------------------------
// Form a borrowed source view from two node boundaries in the same parsed tree.
//
// @parsed: Live parsed-source handle that owns the tree and borrows the source.
// @first: Node providing the first byte and point of the range.
// @last: Node providing the exclusive end byte and point of the range.
// @returns: Borrowed half-open source slice, or an empty view on invalid bounds.
//--------------------------------------------------------
static lemma_syntax_text syntax_text_between_nodes(
    const lemma_parsed_source *parsed, TSNode first, TSNode last)
{
    lemma_syntax_text text = {0};
    if (parsed == NULL || parsed->source == NULL || ts_node_is_null(first) ||
        ts_node_is_null(last) || first.tree != parsed->tree || last.tree != parsed->tree) {
        return text;
    }

    lemma_ir_source_span first_span = {0};
    lemma_ir_source_span last_span = {0};
    if (lemma_frontend_ts_node_span(first, &first_span) != LEMMA_TS_SPAN_VALID ||
        lemma_frontend_ts_node_span(last, &last_span) != LEMMA_TS_SPAN_VALID) {
        return text;
    }
    size_t start = first_span.start_byte;
    size_t end = last_span.start_byte;
    if (end < start || end > parsed->source_length) {
        return text;
    }
    text.data = parsed->source + start;
    text.length = end - start;
    text.span = (lemma_ir_source_span){
        start, end,
        first_span.start_row, first_span.start_column,
        last_span.start_row, last_span.start_column
    };
    return text;
}

//--------------------------------------------------------
// Return the first direct child with an exact named type, or a null node.
//
// @parent: Parent node whose direct children are searched.
// @type: Exact Tree-sitter node type to find.
// @returns: Matching direct child, or a null node if missing or ambiguous.
//--------------------------------------------------------
static TSNode syntax_unique_child(TSNode parent, const char *type)
{
    TSNode child = {0};
    if (lemma_frontend_ts_unique_child_by_type(
            parent, type, LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS,
            &child) != LEMMA_TS_CHILD_TYPE_UNIQUE) {
        return (TSNode){0};
    }
    return child;
}

//--------------------------------------------------------
// Return the first direct child of one of two syntax types, if exactly one exists.
//
// @parent: Parent node whose direct children are searched.
// @first_type: First exact Tree-sitter node type to find.
// @second_type: Alternate exact Tree-sitter node type to find.
// @returns: Matching direct child, or a null node when absent or ambiguous.
//--------------------------------------------------------
static TSNode syntax_child_of_either(TSNode parent, const char *first_type,
                                     const char *second_type)
{
    TSNode first = syntax_unique_child(parent, first_type);
    TSNode second = syntax_unique_child(parent, second_type);
    if (!ts_node_is_null(first) && !ts_node_is_null(second)) {
        return (TSNode){0};
    }
    return ts_node_is_null(first) ? second : first;
}

//--------------------------------------------------------
// Return a unique direct child including anonymous punctuation tokens.
//
// @parent: Parent node whose direct children are searched.
// @type: Exact Tree-sitter node type to find.
// @returns: Matching child, or a null node when missing or ambiguous.
//--------------------------------------------------------
static TSNode syntax_child_any(TSNode parent, const char *type)
{
    TSNode child = {0};
    if (lemma_frontend_ts_unique_child_by_type(
            parent, type, LEMMA_TS_CHILD_FILTER_ALL,
            &child) != LEMMA_TS_CHILD_TYPE_UNIQUE) {
        return (TSNode){0};
    }
    return child;
}

//--------------------------------------------------------
// Count direct named children after excluding Tree-sitter extras.
//
// @parent: Parent node whose direct children are counted.
// @returns: Number of named, non-extra direct children.
//--------------------------------------------------------
static uint32_t syntax_named_non_extra_child_count(TSNode parent)
{
    uint32_t total = ts_node_named_child_count(parent);
    uint32_t count = 0U;
    for (uint32_t index = 0U; index < total; ++index) {
        if (!ts_node_is_extra(ts_node_named_child(parent, index))) {
            ++count;
        }
    }
    return count;
}

//--------------------------------------------------------
// Send one borrowed syntax view to the caller and remember stop or failure.
//
// @walk: Active traversal state.
// @view: Syntax view whose fields remain borrowed from the parsed source.
// @returns: Nonzero to continue traversal; zero after stop or callback failure.
//--------------------------------------------------------
static int syntax_emit(syntax_walk *walk, const lemma_syntax_view *view)
{
    if (walk->result != LEMMA_SYNTAX_VISIT_COMPLETE) {
        return 0;
    }
    lemma_syntax_visit_action action = walk->visitor(view, walk->visitor_context);
    if (action == LEMMA_SYNTAX_VISIT_CONTINUE) {
        return 1;
    }
    walk->result = action == LEMMA_SYNTAX_VISIT_STOP
                       ? LEMMA_SYNTAX_VISIT_STOPPED
                       : LEMMA_SYNTAX_VISIT_FAILED;
    return 0;
}

//--------------------------------------------------------
// Emit an opaque module item so later stages cannot silently omit its syntax.
//
// @walk: Active syntax traversal state.
// @item: Module item retained as raw syntax.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int emit_opaque_module_item(syntax_walk *walk, TSNode item,
                                   lemma_ir_source_span module_span);

//--------------------------------------------------------
// Extend a source range to cover direct packed-dimension children.
//
// @child: Direct child node inspected.
// @child_index: Original direct-child index, unused by this callback.
// @field_name: Optional Tree-sitter field name, unused by this callback.
// @context: Range accumulator updated with the first and last dimension nodes.
// @returns: CONTINUE to inspect the remaining children.
//--------------------------------------------------------
static lemma_ts_child_visit_action collect_packed_dimension(
    TSNode child, uint32_t child_index, const char *field_name, void *context)
{
    (void)child_index;
    (void)field_name;
    range_accumulator *accumulator = context;
    if (strcmp(ts_node_type(child), "packed_dimension") == 0) {
        if (ts_node_is_null(accumulator->first)) {
            accumulator->first = child;
        }
        accumulator->last = child;
    }
    return LEMMA_TS_CHILD_VISIT_CONTINUE;
}

//--------------------------------------------------------
// Visit only the grammar's type wrappers needed to locate packed dimensions.
//
// @accumulator: Range state updated with any packed-dimension nodes.
// @node: Type node whose immediate type wrappers are inspected.
// @returns: Nonzero when its source slice can be inspected.
//--------------------------------------------------------
static int collect_type_dimensions(range_accumulator *accumulator, TSNode node)
{
    if (ts_node_is_null(node)) {
        return 1;
    }
    const char *type = ts_node_type(node);
    if (strcmp(type, "data_type") == 0 || strcmp(type, "implicit_data_type") == 0) {
        (void)lemma_frontend_ts_visit_children(
            node, LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS,
            collect_packed_dimension, accumulator);
        return 1;
    }

    TSNode nested = {0};
    if (strcmp(type, "data_type_or_implicit") == 0) {
        nested = syntax_child_of_either(node, "data_type", "implicit_data_type");
    } else if (strcmp(type, "variable_port_type") == 0) {
        nested = syntax_unique_child(node, "data_type");
    } else if (strcmp(type, "net_port_type") == 0) {
        nested = syntax_unique_child(node, "data_type_or_implicit");
    } else if (strcmp(type, "parameter_declaration") == 0 ||
               strcmp(type, "local_parameter_declaration") == 0) {
        nested = syntax_unique_child(node, "data_type_or_implicit");
    }
    return ts_node_is_null(nested) ? 1 : collect_type_dimensions(accumulator, nested);
}

//--------------------------------------------------------
// Return a source slice covering packed dimensions in a known type grammar shape.
//
// @parsed: Live parsed-source handle that owns the type node's tree.
// @type_node: Type syntax node inspected.
// @returns: Borrowed source slice covering all dimensions, or an empty view.
//--------------------------------------------------------
static lemma_syntax_text syntax_packed_dimensions(
    const lemma_parsed_source *parsed, TSNode type_node)
{
    range_accumulator accumulator = {parsed, {0}, {0}};
    (void)collect_type_dimensions(&accumulator, type_node);
    if (ts_node_is_null(accumulator.first)) {
        return (lemma_syntax_text){0};
    }
    if (ts_node_eq(accumulator.first, accumulator.last)) {
        return syntax_text_for_node(parsed, accumulator.first);
    }
    lemma_ir_source_span first_span = {0};
    lemma_ir_source_span last_span = {0};
    if (lemma_frontend_ts_node_span(accumulator.first, &first_span) !=
            LEMMA_TS_SPAN_VALID ||
        lemma_frontend_ts_node_span(accumulator.last, &last_span) !=
            LEMMA_TS_SPAN_VALID) {
        return (lemma_syntax_text){0};
    }
    size_t start = first_span.start_byte;
    size_t end = last_span.end_byte;
    if (end < start || end > parsed->source_length) {
        return (lemma_syntax_text){0};
    }
    return (lemma_syntax_text){
        parsed->source + start, end - start,
        {start, end, first_span.start_row, first_span.start_column,
         last_span.end_row, last_span.end_column}
    };
}

//--------------------------------------------------------
// Emit one declaration-group view before the group's declarator views.
//
// @walk: Active traversal state.
// @kind: Syntactic category of this declaration group.
// @source_node: Node whose full span identifies the group.
// @module_span: Span of the containing module.
// @direction_node: Explicit port direction node, or null.
// @type_node: Shared type node, or null when the syntax omits it.
// @dimensions_node: Syntax node containing packed dimensions, or null.
// @has_port_header: Nonzero only when this ANSI port node has a header.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int syntax_emit_declaration_group(
    syntax_walk *walk, lemma_syntax_declaration_group_kind kind,
    TSNode source_node, lemma_ir_source_span module_span,
    TSNode direction_node, TSNode type_node, TSNode dimensions_node,
    int has_port_header)
{
    lemma_syntax_declaration_group group = {0};
    group.kind = kind;
    group.direction = syntax_text_for_node(walk->parsed, direction_node);
    group.type = syntax_text_for_node(walk->parsed, type_node);
    group.packed_dimensions = syntax_packed_dimensions(
        walk->parsed, ts_node_is_null(dimensions_node) ? type_node : dimensions_node);
    group.source = syntax_text_for_node(walk->parsed, source_node);
    group.module_span = module_span;
    group.has_explicit_port_header = has_port_header != 0;

    lemma_syntax_view view = {0};
    view.kind = LEMMA_SYNTAX_DECLARATION_GROUP;
    view.as.declaration_group = group;
    return syntax_emit(walk, &view);
}

//--------------------------------------------------------
// Emit one declarator associated with its declaration group's source span.
//
// @walk: Active traversal state.
// @name_node: Identifier node naming the declarator.
// @declarator_node: Full per-name syntax node.
// @group_span: Span of the shared declaration group.
// @module_span: Span of the containing module.
// @value_type: Direct child type holding an optional initializer or default.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int syntax_emit_declarator(
    syntax_walk *walk, TSNode name_node, TSNode declarator_node,
    lemma_ir_source_span group_span, lemma_ir_source_span module_span,
    const char *value_type)
{
    if (ts_node_is_null(name_node)) {
        return emit_opaque_module_item(walk, declarator_node, module_span);
    }
    TSNode suffix_node = syntax_child_of_either(
        declarator_node, "unpacked_dimension", "variable_dimension");
    TSNode value_node = {0};
    if (value_type != NULL) {
        value_node = syntax_unique_child(declarator_node, value_type);
    }
    lemma_syntax_declarator declarator = {0};
    declarator.name = syntax_text_for_node(walk->parsed, name_node);
    declarator.suffix = syntax_text_for_node(walk->parsed, suffix_node);
    declarator.initializer_or_default_expression =
        syntax_text_for_node(walk->parsed, value_node);
    declarator.declarator_span = syntax_span_for_node(declarator_node);
    declarator.group_span = group_span;
    declarator.module_span = module_span;

    lemma_syntax_view view = {0};
    view.kind = LEMMA_SYNTAX_DECLARATOR;
    view.as.declarator = declarator;
    return syntax_emit(walk, &view);
}

typedef struct declarator_visit {
    syntax_walk *walk;
    lemma_ir_source_span group_span;
    lemma_ir_source_span module_span;
    const char *declarator_type;
    const char *value_type;
} declarator_visit;

//--------------------------------------------------------
// Emit one named declarator from a declaration's declarator list.
//
// @child: Direct list child being inspected.
// @child_index: Original child index, unused by this callback.
// @field_name: Optional grammar field name, unused by this callback.
// @context: Declaration-group state for this visit.
// @returns: CONTINUE for unrelated list items; STOP on visitor stop; FAIL on error.
//--------------------------------------------------------
static lemma_ts_child_visit_action visit_declarator_child(
    TSNode child, uint32_t child_index, const char *field_name, void *context)
{
    (void)child_index;
    (void)field_name;
    declarator_visit *visit = context;
    if (strcmp(ts_node_type(child), visit->declarator_type) != 0) {
        if (emit_opaque_module_item(visit->walk, child, visit->module_span)) {
            return LEMMA_TS_CHILD_VISIT_CONTINUE;
        }
        return visit->walk->result == LEMMA_SYNTAX_VISIT_STOPPED
                   ? LEMMA_TS_CHILD_VISIT_STOP
                   : LEMMA_TS_CHILD_VISIT_FAIL;
    }
    TSNode name = syntax_unique_child(child, "simple_identifier");
    if (!syntax_emit_declarator(visit->walk, name, child, visit->group_span,
                                visit->module_span, visit->value_type)) {
        return visit->walk->result == LEMMA_SYNTAX_VISIT_STOPPED
                   ? LEMMA_TS_CHILD_VISIT_STOP
                   : LEMMA_TS_CHILD_VISIT_FAIL;
    }
    return LEMMA_TS_CHILD_VISIT_CONTINUE;
}

//--------------------------------------------------------
// Emit one parameter or local-parameter group and its per-name assignments.
//
// @walk: Active syntax traversal state.
// @declaration: Parameter declaration node sharing a type across names.
// @kind: Parameter or local-parameter declaration category.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_parameter_declaration(
    syntax_walk *walk, TSNode declaration,
    lemma_syntax_declaration_group_kind kind,
    lemma_ir_source_span module_span)
{
    TSNode list = syntax_unique_child(declaration, "list_of_param_assignments");
    if (ts_node_is_null(list)) {
        lemma_syntax_opaque_module_item opaque = {
            syntax_text_for_node(walk->parsed, declaration), module_span
        };
        lemma_syntax_view view = {0};
        view.kind = LEMMA_SYNTAX_OPAQUE_MODULE_ITEM;
        view.as.opaque_module_item = opaque;
        return syntax_emit(walk, &view);
    }

    TSNode type = syntax_child_of_either(declaration, "data_type_or_implicit", "type");
    if (!syntax_emit_declaration_group(walk, kind, declaration, module_span,
                                       (TSNode){0}, type, type, 0)) {
        return 0;
    }
    lemma_syntax_text group_text = syntax_text_for_node(walk->parsed, declaration);
    declarator_visit visit = {
        walk, group_text.span, module_span, "param_assignment",
        "constant_param_expression"
    };
    lemma_ts_child_visit_result result = lemma_frontend_ts_visit_children(
        list, LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS,
        visit_declarator_child, &visit);
    return result == LEMMA_TS_CHILD_VISIT_COMPLETE;
}

//--------------------------------------------------------
// Emit parameter declarations from a module's parameter-port list.
//
// @walk: Active syntax traversal state.
// @parameter_list: Parameter-port list node, or null when absent.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_parameter_ports(syntax_walk *walk, TSNode parameter_list,
                                 lemma_ir_source_span module_span)
{
    if (ts_node_is_null(parameter_list)) {
        return 1;
    }
    uint32_t count = ts_node_named_child_count(parameter_list);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode port = ts_node_named_child(parameter_list, index);
        if (ts_node_is_extra(port)) {
            continue;
        }
        TSNode declaration = syntax_child_of_either(
            port, "parameter_declaration", "local_parameter_declaration");
        if (ts_node_is_null(declaration)) {
            lemma_syntax_opaque_module_item opaque = {
                syntax_text_for_node(walk->parsed, port), module_span
            };
            lemma_syntax_view view = {0};
            view.kind = LEMMA_SYNTAX_OPAQUE_MODULE_ITEM;
            view.as.opaque_module_item = opaque;
            if (!syntax_emit(walk, &view)) {
                return 0;
            }
            continue;
        }
        lemma_syntax_declaration_group_kind kind =
            strcmp(ts_node_type(declaration), "local_parameter_declaration") == 0
                ? LEMMA_SYNTAX_DECLARATION_GROUP_LOCALPARAM
                : LEMMA_SYNTAX_DECLARATION_GROUP_PARAMETER;
        if (!visit_parameter_declaration(walk, declaration, kind, module_span)) {
            return 0;
        }
    }
    return 1;
}

//--------------------------------------------------------
// Emit one ANSI port group and its declared identifier.
//
// @walk: Active syntax traversal state.
// @port: ANSI port declaration node.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_ansi_port(syntax_walk *walk, TSNode port,
                           lemma_ir_source_span module_span)
{
    TSNode name = syntax_unique_child(port, "simple_identifier");
    TSNode net_header = syntax_unique_child(port, "net_port_header");
    TSNode variable_header = syntax_unique_child(port, "variable_port_header");
    uint32_t child_count = syntax_named_non_extra_child_count(port);
    int has_header = !ts_node_is_null(net_header) || !ts_node_is_null(variable_header);
    if (ts_node_is_null(name) ||
        (!ts_node_is_null(net_header) && !ts_node_is_null(variable_header)) ||
        child_count != (has_header != 0 ? 2U : 1U)) {
        lemma_syntax_opaque_module_item opaque = {
            syntax_text_for_node(walk->parsed, port), module_span
        };
        lemma_syntax_view view = {0};
        view.kind = LEMMA_SYNTAX_OPAQUE_MODULE_ITEM;
        view.as.opaque_module_item = opaque;
        return syntax_emit(walk, &view);
    }

    if (ts_node_is_null(net_header) && ts_node_is_null(variable_header) &&
        ts_node_named_child_count(port) != 1U) {
        return emit_opaque_module_item(walk, port, module_span);
    }

    TSNode header = ts_node_is_null(net_header) ? variable_header : net_header;
    TSNode direction = syntax_unique_child(header, "port_direction");
    TSNode type = ts_node_is_null(net_header)
                      ? syntax_unique_child(variable_header, "variable_port_type")
                      : syntax_unique_child(net_header, "net_port_type");
    lemma_syntax_text group_text = syntax_text_for_node(walk->parsed, port);
    if (!syntax_emit_declaration_group(
            walk, LEMMA_SYNTAX_DECLARATION_GROUP_PORT, port, module_span,
            direction, type, type, !ts_node_is_null(header))) {
        return 0;
    }
    TSNode suffix = syntax_unique_child(port, "unpacked_dimension");
    lemma_syntax_declarator declarator = {0};
    declarator.name = syntax_text_for_node(walk->parsed, name);
    declarator.suffix = syntax_text_for_node(walk->parsed, suffix);
    declarator.declarator_span = syntax_span_for_node(name);
    declarator.group_span = group_text.span;
    declarator.module_span = module_span;
    lemma_syntax_view view = {0};
    view.kind = LEMMA_SYNTAX_DECLARATOR;
    view.as.declarator = declarator;
    return syntax_emit(walk, &view);
}

typedef struct port_list_visit {
    syntax_walk *walk;
    lemma_ir_source_span module_span;
} port_list_visit;

//--------------------------------------------------------
// Visit one ANSI port child from the header's ordered port list.
//
// @child: Direct port-list child being inspected.
// @child_index: Original child index, unused by this callback.
// @field_name: Optional grammar field name, unused by this callback.
// @context: Port-list traversal state.
// @returns: CONTINUE for non-port children; STOP on visitor stop; FAIL on error.
//--------------------------------------------------------
static lemma_ts_child_visit_action visit_port_child(
    TSNode child, uint32_t child_index, const char *field_name, void *context)
{
    (void)child_index;
    (void)field_name;
    port_list_visit *visit = context;
    if (strcmp(ts_node_type(child), "ansi_port_declaration") != 0) {
        if (emit_opaque_module_item(visit->walk, child, visit->module_span)) {
            return LEMMA_TS_CHILD_VISIT_CONTINUE;
        }
        return visit->walk->result == LEMMA_SYNTAX_VISIT_STOPPED
                   ? LEMMA_TS_CHILD_VISIT_STOP
                   : LEMMA_TS_CHILD_VISIT_FAIL;
    }
    if (!visit_ansi_port(visit->walk, child, visit->module_span)) {
        return visit->walk->result == LEMMA_SYNTAX_VISIT_STOPPED
                   ? LEMMA_TS_CHILD_VISIT_STOP
                   : LEMMA_TS_CHILD_VISIT_FAIL;
    }
    return LEMMA_TS_CHILD_VISIT_CONTINUE;
}

//--------------------------------------------------------
// Emit ANSI port groups and declarators in header order.
//
// @walk: Active syntax traversal state.
// @port_list: ANSI port list node, or null when absent.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_ports(syntax_walk *walk, TSNode port_list,
                       lemma_ir_source_span module_span)
{
    if (ts_node_is_null(port_list)) {
        return 1;
    }
    port_list_visit visit = {walk, module_span};
    lemma_ts_child_visit_result result = lemma_frontend_ts_visit_children(
        port_list, LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS,
        visit_port_child, &visit);
    return result == LEMMA_TS_CHILD_VISIT_COMPLETE;
}

//--------------------------------------------------------
// Emit one net or data declaration group and all its declarators.
//
// @walk: Active syntax traversal state.
// @declaration: Module-scope net or data declaration node.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_signal_declaration(syntax_walk *walk, TSNode declaration,
                                    lemma_ir_source_span module_span)
{
    const char *declaration_type = ts_node_type(declaration);
    int is_net = strcmp(declaration_type, "net_declaration") == 0;
    TSNode list = syntax_unique_child(
        declaration, is_net ? "list_of_net_decl_assignments"
                            : "list_of_variable_decl_assignments");
    TSNode type = is_net ? syntax_unique_child(declaration, "net_type")
                         : syntax_unique_child(declaration, "data_type_or_implicit");
    TSNode dimensions = is_net
                            ? syntax_unique_child(declaration, "data_type_or_implicit")
                            : type;
    if (ts_node_is_null(list) || ts_node_is_null(type)) {
        lemma_syntax_opaque_module_item opaque = {
            syntax_text_for_node(walk->parsed, declaration), module_span
        };
        lemma_syntax_view view = {0};
        view.kind = LEMMA_SYNTAX_OPAQUE_MODULE_ITEM;
        view.as.opaque_module_item = opaque;
        return syntax_emit(walk, &view);
    }

    lemma_syntax_declaration_group_kind kind =
        is_net ? LEMMA_SYNTAX_DECLARATION_GROUP_NET
               : LEMMA_SYNTAX_DECLARATION_GROUP_DATA;
    if (!syntax_emit_declaration_group(walk, kind, declaration, module_span,
                                       (TSNode){0}, type, dimensions, 0)) {
        return 0;
    }
    lemma_syntax_text group_text = syntax_text_for_node(walk->parsed, declaration);
    declarator_visit visit = {
        walk, group_text.span, module_span,
        is_net ? "net_decl_assignment" : "variable_decl_assignment",
        "expression"
    };
    (void)dimensions;
    lemma_ts_child_visit_result result = lemma_frontend_ts_visit_children(
        list, LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS,
        visit_declarator_child, &visit);
    return result == LEMMA_TS_CHILD_VISIT_COMPLETE;
}

//--------------------------------------------------------
// Extract a simple signal identifier from a net-assignment destination.
//
// @assignment: Net-assignment node being inspected.
// @returns: Simple identifier node, or null for selects and other lvalues.
//--------------------------------------------------------
static TSNode get_destination_signal(TSNode assignment)
{
    TSNode lvalue = syntax_unique_child(assignment, "net_lvalue");
    if (ts_node_is_null(lvalue) ||
        syntax_named_non_extra_child_count(lvalue) != 1U) {
        return (TSNode){0};
    }
    return syntax_unique_child(lvalue, "simple_identifier");
}

//--------------------------------------------------------
// Return the right-hand expression node of one net assignment.
//
// @assignment: Net-assignment node being inspected.
// @returns: Right-hand expression node, or null when the shape is unsupported.
//--------------------------------------------------------
static TSNode get_right_part_of_line(TSNode assignment)
{
    return syntax_unique_child(assignment, "expression");
}

//--------------------------------------------------------
// Return a top-level ternary condition when the RHS has one.
//
// @expression: Right-hand expression node being inspected.
// @returns: Condition expression node, or null when the RHS is not a ternary.
//--------------------------------------------------------
static TSNode get_condition_if_present(TSNode expression)
{
    TSNode conditional = syntax_unique_child(expression, "conditional_expression");
    if (ts_node_is_null(conditional)) {
        return (TSNode){0};
    }
    uint32_t count = ts_node_named_child_count(conditional);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode child = ts_node_named_child(conditional, index);
        if (!ts_node_is_extra(child) &&
            strcmp(ts_node_type(child), "expression") == 0) {
            return child;
        }
    }
    return (TSNode){0};
}

//--------------------------------------------------------
// Emit each continuous assignment pair from a module item.
//
// @walk: Active syntax traversal state.
// @continuous: Continuous-assignment module item.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_continuous_assignments(syntax_walk *walk, TSNode continuous,
                                        lemma_ir_source_span module_span)
{
    if (!ts_node_is_null(syntax_unique_child(continuous, "delay_control")) ||
        !ts_node_is_null(syntax_unique_child(continuous, "delay2")) ||
        !ts_node_is_null(syntax_unique_child(continuous, "delay3"))) {
        return emit_opaque_module_item(walk, continuous, module_span);
    }
    TSNode list = syntax_unique_child(continuous, "list_of_net_assignments");
    if (ts_node_is_null(list)) {
        lemma_syntax_opaque_module_item opaque = {
            syntax_text_for_node(walk->parsed, continuous), module_span
        };
        lemma_syntax_view view = {0};
        view.kind = LEMMA_SYNTAX_OPAQUE_MODULE_ITEM;
        view.as.opaque_module_item = opaque;
        return syntax_emit(walk, &view);
    }

    uint32_t count = ts_node_named_child_count(list);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode assignment = ts_node_named_child(list, index);
        if (ts_node_is_extra(assignment)) {
            continue;
        }
        if (strcmp(ts_node_type(assignment), "net_assignment") != 0) {
            if (!emit_opaque_module_item(walk, assignment, module_span)) {
                return 0;
            }
            continue;
        }
        TSNode target = syntax_unique_child(assignment, "net_lvalue");
        TSNode destination = get_destination_signal(assignment);
        TSNode expression = get_right_part_of_line(assignment);
        if (ts_node_is_null(target) || ts_node_is_null(expression)) {
            lemma_syntax_opaque_module_item opaque = {
                syntax_text_for_node(walk->parsed, assignment), module_span
            };
            lemma_syntax_view opaque_view = {0};
            opaque_view.kind = LEMMA_SYNTAX_OPAQUE_MODULE_ITEM;
            opaque_view.as.opaque_module_item = opaque;
            if (!syntax_emit(walk, &opaque_view)) {
                return 0;
            }
            continue;
        }
        lemma_syntax_continuous_assignment item = {0};
        item.lvalue = syntax_text_for_node(walk->parsed, target);
        item.destination_signal = syntax_text_for_node(walk->parsed, destination);
        item.expression = syntax_text_for_node(walk->parsed, expression);
        item.condition = syntax_text_for_node(
            walk->parsed, get_condition_if_present(expression));
        item.assignment = syntax_text_for_node(walk->parsed, assignment);
        item.module_span = module_span;
        lemma_syntax_view view = {0};
        view.kind = LEMMA_SYNTAX_CONTINUOUS_ASSIGNMENT;
        view.as.continuous_assignment = item;
        if (!syntax_emit(walk, &view)) {
            return 0;
        }
    }
    return 1;
}

//--------------------------------------------------------
// Emit an opaque module item so later stages cannot silently omit its syntax.
//
// @walk: Active syntax traversal state.
// @item: Module item retained as raw syntax.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int emit_opaque_module_item(syntax_walk *walk, TSNode item,
                                   lemma_ir_source_span module_span)
{
    lemma_syntax_opaque_module_item opaque = {
        syntax_text_for_node(walk->parsed, item), module_span
    };
    lemma_syntax_view view = {0};
    view.kind = LEMMA_SYNTAX_OPAQUE_MODULE_ITEM;
    view.as.opaque_module_item = opaque;
    return syntax_emit(walk, &view);
}

//--------------------------------------------------------
// Visit one process and its syntax-only event and statement views.
//
// @walk: Active syntax traversal state.
// @process_node: Process construct node.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_process(syntax_walk *walk, TSNode process_node,
                         lemma_ir_source_span module_span);

//--------------------------------------------------------
// Visit one direct module-body item using its grammar-specific extractor.
//
// @walk: Active syntax traversal state.
// @item: Direct module-body item.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_body_item(syntax_walk *walk, TSNode item,
                           lemma_ir_source_span module_span)
{
    const char *type = ts_node_type(item);
    if (strcmp(type, "net_declaration") == 0 ||
        strcmp(type, "data_declaration") == 0) {
        return visit_signal_declaration(walk, item, module_span);
    }
    if (strcmp(type, "local_parameter_declaration") == 0 ||
        strcmp(type, "parameter_declaration") == 0) {
        lemma_syntax_declaration_group_kind kind =
            strcmp(type, "local_parameter_declaration") == 0
                ? LEMMA_SYNTAX_DECLARATION_GROUP_LOCALPARAM
                : LEMMA_SYNTAX_DECLARATION_GROUP_PARAMETER;
        return visit_parameter_declaration(walk, item, kind, module_span);
    }
    if (strcmp(type, "continuous_assign") == 0) {
        return visit_continuous_assignments(walk, item, module_span);
    }
    if (strcmp(type, "always_construct") == 0) {
        return visit_process(walk, item, module_span);
    }
    return emit_opaque_module_item(walk, item, module_span);
}

typedef struct body_visit {
    syntax_walk *walk;
    TSNode header;
    lemma_ir_source_span module_span;
} body_visit;

//--------------------------------------------------------
// Visit one direct child of a module declaration's body.
//
// @child: Direct module child inspected.
// @child_index: Original child index, unused by this callback.
// @field_name: Optional grammar field name, unused by this callback.
// @context: Module-body traversal state.
// @returns: CONTINUE for the header; otherwise STOP or FAIL on visitor result.
//--------------------------------------------------------
static lemma_ts_child_visit_action visit_body_child(
    TSNode child, uint32_t child_index, const char *field_name, void *context)
{
    (void)child_index;
    (void)field_name;
    body_visit *visit = context;
    if ((!ts_node_is_null(visit->header) && ts_node_eq(child, visit->header)) ||
        ts_node_is_extra(child)) {
        return LEMMA_TS_CHILD_VISIT_CONTINUE;
    }
    if (!visit_body_item(visit->walk, child, visit->module_span)) {
        return visit->walk->result == LEMMA_SYNTAX_VISIT_STOPPED
                   ? LEMMA_TS_CHILD_VISIT_STOP
                   : LEMMA_TS_CHILD_VISIT_FAIL;
    }
    return LEMMA_TS_CHILD_VISIT_CONTINUE;
}

//--------------------------------------------------------
// Visit one nested statement and its statement children in source order.
//
// @walk: Active syntax traversal state.
// @node: Statement node or Tree-sitter wrapper around one statement.
// @module_span: Span of the containing module.
// @process_span: Span of the enclosing process.
// @parent_span: Span of the parent statement or process.
// @depth: Nesting depth from the process body.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_statement_node(syntax_walk *walk, TSNode node,
                                lemma_ir_source_span module_span,
                                lemma_ir_source_span process_span,
                                lemma_ir_source_span parent_span, size_t depth);

typedef struct statement_children_visit {
    syntax_walk *walk;
    TSNode parent;
    lemma_ir_source_span module_span;
    lemma_ir_source_span process_span;
    lemma_ir_source_span parent_span;
    size_t depth;
} statement_children_visit;

//--------------------------------------------------------
// Visit one direct statement child while ignoring expression and punctuation nodes.
//
// @child: Direct child inspected.
// @child_index: Original child index, unused by this callback.
// @field_name: Optional grammar field name, unused by this callback.
// @context: Parent statement and traversal state.
// @returns: CONTINUE for non-statement children; STOP or FAIL on visitor result.
//--------------------------------------------------------
static lemma_ts_child_visit_action visit_statement_child(
    TSNode child, uint32_t child_index, const char *field_name, void *context)
{
    (void)child_index;
    (void)field_name;
    statement_children_visit *visit = context;
    const char *type = ts_node_type(child);
    int statement_child = strcmp(type, "statement") == 0 ||
        strcmp(type, "statement_item") == 0 ||
        strcmp(type, "statement_or_null") == 0 ||
        strcmp(type, "seq_block") == 0 ||
        strcmp(type, "conditional_statement") == 0 ||
        strcmp(type, "case_statement") == 0 ||
        strcmp(type, "case_item") == 0 ||
        strcmp(type, "blocking_assignment") == 0 ||
        strcmp(type, "nonblocking_assignment") == 0 ||
        strcmp(type, "loop_statement") == 0 ||
        strcmp(type, "procedural_timing_control_statement") == 0 ||
        strcmp(type, "block_item_declaration") == 0;
    const char *parent_type = ts_node_type(visit->parent);
    if (!statement_child &&
        ((strcmp(parent_type, "conditional_statement") == 0 &&
          strcmp(type, "cond_predicate") == 0) ||
         (strcmp(parent_type, "case_statement") == 0 &&
          (strcmp(type, "expression") == 0 ||
           strcmp(type, "case_expression") == 0)) ||
         (strcmp(parent_type, "case_item") == 0 &&
          strcmp(type, "expression") == 0))) {
        return LEMMA_TS_CHILD_VISIT_CONTINUE;
    }
    if (!statement_child) {
        lemma_syntax_statement opaque = {0};
        opaque.kind = LEMMA_SYNTAX_STATEMENT_OPAQUE;
        opaque.source = syntax_text_for_node(visit->walk->parsed, child);
        opaque.module_span = visit->module_span;
        opaque.process_span = visit->process_span;
        opaque.parent_span = visit->parent_span;
        opaque.depth = visit->depth;
        lemma_syntax_view opaque_view = {0};
        opaque_view.kind = LEMMA_SYNTAX_STATEMENT;
        opaque_view.as.statement = opaque;
        if (syntax_emit(visit->walk, &opaque_view)) {
            return LEMMA_TS_CHILD_VISIT_CONTINUE;
        }
        return visit->walk->result == LEMMA_SYNTAX_VISIT_STOPPED
                   ? LEMMA_TS_CHILD_VISIT_STOP
                   : LEMMA_TS_CHILD_VISIT_FAIL;
    }
    if (!visit_statement_node(visit->walk, child, visit->module_span,
                              visit->process_span, visit->parent_span,
                              visit->depth)) {
        return visit->walk->result == LEMMA_SYNTAX_VISIT_STOPPED
                   ? LEMMA_TS_CHILD_VISIT_STOP
                   : LEMMA_TS_CHILD_VISIT_FAIL;
    }
    return LEMMA_TS_CHILD_VISIT_CONTINUE;
}

//--------------------------------------------------------
// Visit statement children through the Tree-sitter direct-child helper.
//
// @walk: Active syntax traversal state.
// @parent: Node whose direct children may contain statements.
// @module_span: Span of the containing module.
// @process_span: Span of the enclosing process.
// @parent_span: Span assigned as the immediate statement parent.
// @depth: Nesting depth assigned to child statements.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_statement_children(syntax_walk *walk, TSNode parent,
                                    lemma_ir_source_span module_span,
                                    lemma_ir_source_span process_span,
                                    lemma_ir_source_span parent_span, size_t depth)
{
    statement_children_visit visit = {
        walk, parent, module_span, process_span, parent_span, depth
    };
    lemma_ts_child_visit_result result = lemma_frontend_ts_visit_children(
        parent, LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS,
        visit_statement_child, &visit);
    return result == LEMMA_TS_CHILD_VISIT_COMPLETE;
}

//--------------------------------------------------------
// Find the first statement body nested under a loop-control wrapper.
//
// @node: Loop node or direct child inspected.
// @returns: First nested statement node, or a null node when absent.
//--------------------------------------------------------
static TSNode first_loop_body(TSNode node)
{
    const char *type = ts_node_type(node);
    if (strcmp(type, "statement_or_null") == 0 ||
        strcmp(type, "statement") == 0 ||
        strcmp(type, "statement_item") == 0 ||
        strcmp(type, "seq_block") == 0 ||
        strcmp(type, "conditional_statement") == 0 ||
        strcmp(type, "case_statement") == 0 ||
        strcmp(type, "blocking_assignment") == 0 ||
        strcmp(type, "nonblocking_assignment") == 0) {
        return node;
    }
    uint32_t count = ts_node_named_child_count(node);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode child = ts_node_named_child(node, index);
        if (ts_node_is_extra(child)) {
            continue;
        }
        TSNode body = first_loop_body(child);
        if (!ts_node_is_null(body)) {
            return body;
        }
    }
    return (TSNode){0};
}

//--------------------------------------------------------
// Find a statement's direct assignment operator, including anonymous tokens.
//
// @node: Assignment node whose immediate children are searched.
// @returns: Assignment operator node, or null when the shape is unsupported.
//--------------------------------------------------------
static TSNode assignment_operator(TSNode node)
{
    uint32_t count = ts_node_child_count(node);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode child = ts_node_child(node, index);
        const char *type = ts_node_type(child);
        if (strcmp(type, "assignment_operator") == 0 ||
            strcmp(type, "=") == 0 || strcmp(type, "<=") == 0 ||
            strcmp(type, "+=") == 0 || strcmp(type, "-=") == 0 ||
            strcmp(type, "*=") == 0 || strcmp(type, "/=") == 0 ||
            strcmp(type, "%=") == 0 || strcmp(type, "&=") == 0 ||
            strcmp(type, "|=") == 0 || strcmp(type, "^=") == 0 ||
            strcmp(type, "<<=") == 0 || strcmp(type, ">>=") == 0) {
            return child;
        }
    }
    return (TSNode){0};
}

//--------------------------------------------------------
// Emit one typed statement event with its borrowed source and nesting metadata.
//
// @walk: Active syntax traversal state.
// @kind: Syntactic statement category.
// @node: Complete statement syntax node.
// @header: Borrowed construct-specific source slice, or an empty slice.
// @lvalue_node: Optional assignment destination node.
// @expression_node: Optional condition or right-hand expression node.
// @module_span: Span of the containing module.
// @process_span: Span of the enclosing process.
// @parent_span: Span of the immediate parent statement or process.
// @depth: Nesting depth from the process body.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int emit_statement(syntax_walk *walk, lemma_syntax_statement_kind kind,
                          TSNode node, lemma_syntax_text header,
                          TSNode lvalue_node,
                          TSNode expression_node, lemma_ir_source_span module_span,
                          lemma_ir_source_span process_span,
                          lemma_ir_source_span parent_span, size_t depth)
{
    lemma_syntax_statement statement = {0};
    statement.kind = kind;
    statement.source = syntax_text_for_node(walk->parsed, node);
    statement.header = header;
    statement.lvalue = syntax_text_for_node(walk->parsed, lvalue_node);
    statement.expression = syntax_text_for_node(walk->parsed, expression_node);
    statement.module_span = module_span;
    statement.process_span = process_span;
    statement.parent_span = parent_span;
    statement.depth = depth;
    lemma_syntax_view view = {0};
    view.kind = LEMMA_SYNTAX_STATEMENT;
    view.as.statement = statement;
    return syntax_emit(walk, &view);
}

//--------------------------------------------------------
// Find the first direct statement_or_null child, if present.
//
// @node: Parent statement node.
// @returns: Direct statement body node, or null when the statement has no body.
//--------------------------------------------------------
static TSNode direct_statement_body(TSNode node)
{
    return syntax_unique_child(node, "statement_or_null");
}

//--------------------------------------------------------
// Return a statement header slice ending immediately before its first body.
//
// @parsed: Live parsed-source handle borrowing the source.
// @statement: Full statement node.
// @body: First child statement node.
// @returns: Borrowed prefix slice, or an empty view for invalid ranges.
//--------------------------------------------------------
static lemma_syntax_text statement_prefix(const lemma_parsed_source *parsed,
                                          TSNode statement, TSNode body)
{
    return syntax_text_between_nodes(parsed, statement, body);
}

//--------------------------------------------------------
// Visit one statement, unwrapping Tree-sitter's statement container nodes.
//
// @walk: Active syntax traversal state.
// @node: Statement node or grammar wrapper.
// @module_span: Span of the containing module.
// @process_span: Span of the enclosing process.
// @parent_span: Span of the immediate parent statement or process.
// @depth: Nesting depth from the process body.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_statement_node(syntax_walk *walk, TSNode node,
                                lemma_ir_source_span module_span,
                                lemma_ir_source_span process_span,
                                lemma_ir_source_span parent_span, size_t depth)
{
    const char *type = ts_node_type(node);
    if (strcmp(type, "statement") == 0 ||
        strcmp(type, "statement_item") == 0 ||
        strcmp(type, "statement_or_null") == 0) {
        return visit_statement_children(walk, node, module_span, process_span,
                                        parent_span, depth);
    }
    if (strcmp(type, "procedural_timing_control_statement") == 0) {
        return emit_statement(walk, LEMMA_SYNTAX_STATEMENT_OPAQUE, node,
                              (lemma_syntax_text){0}, (TSNode){0}, (TSNode){0},
                              module_span, process_span, parent_span, depth);
    }

    lemma_syntax_statement_kind kind = LEMMA_SYNTAX_STATEMENT_OPAQUE;
    lemma_syntax_text header = {0};
    TSNode lvalue = {0};
    TSNode expression = {0};
    if (strcmp(type, "seq_block") == 0) {
        kind = LEMMA_SYNTAX_STATEMENT_BLOCK;
        TSNode opener = syntax_child_of_either(node, "begin", "fork");
        if (ts_node_is_null(opener)) {
            opener = syntax_child_any(node, "begin");
        }
        if (ts_node_is_null(opener)) {
            opener = syntax_child_any(node, "fork");
        }
        header = syntax_text_for_node(walk->parsed, opener);
    } else if (strcmp(type, "conditional_statement") == 0) {
        kind = LEMMA_SYNTAX_STATEMENT_CONDITIONAL;
        TSNode predicate = syntax_unique_child(node, "cond_predicate");
        header = syntax_text_for_node(walk->parsed, predicate);
        expression = predicate;
    } else if (strcmp(type, "case_statement") == 0) {
        kind = LEMMA_SYNTAX_STATEMENT_CASE;
        TSNode selector = syntax_child_of_either(node, "expression", "case_expression");
        header = syntax_text_for_node(walk->parsed, selector);
        expression = selector;
    } else if (strcmp(type, "case_item") == 0) {
        kind = LEMMA_SYNTAX_STATEMENT_CASE_ITEM;
        TSNode body = direct_statement_body(node);
        if (!ts_node_is_null(body)) {
            header = statement_prefix(walk->parsed, node, body);
        }
    } else if (strcmp(type, "blocking_assignment") == 0 ||
               strcmp(type, "nonblocking_assignment") == 0) {
        kind = strcmp(type, "blocking_assignment") == 0
                   ? LEMMA_SYNTAX_STATEMENT_BLOCKING_ASSIGNMENT
                   : LEMMA_SYNTAX_STATEMENT_NONBLOCKING_ASSIGNMENT;
        TSNode operation = syntax_unique_child(node, "operator_assignment");
        TSNode search = ts_node_is_null(operation) ? node : operation;
        lvalue = syntax_child_of_either(search, "variable_lvalue", "net_lvalue");
        expression = syntax_unique_child(search, "expression");
        header = syntax_text_for_node(walk->parsed, assignment_operator(search));
    } else if (strcmp(type, "loop_statement") == 0) {
        kind = LEMMA_SYNTAX_STATEMENT_LOOP;
        TSNode body = first_loop_body(node);
        if (!ts_node_is_null(body)) {
            header = statement_prefix(walk->parsed, node, body);
        }
    } else if (strcmp(type, "block_item_declaration") == 0) {
        kind = LEMMA_SYNTAX_STATEMENT_OPAQUE;
    } else {
        kind = LEMMA_SYNTAX_STATEMENT_OPAQUE;
    }

    if (!emit_statement(walk, kind, node, header, lvalue, expression,
                        module_span, process_span, parent_span, depth)) {
        return 0;
    }

    lemma_ir_source_span current_span = syntax_span_for_node(node);
    if (kind == LEMMA_SYNTAX_STATEMENT_BLOCK ||
        kind == LEMMA_SYNTAX_STATEMENT_CONDITIONAL ||
        kind == LEMMA_SYNTAX_STATEMENT_CASE ||
        kind == LEMMA_SYNTAX_STATEMENT_CASE_ITEM) {
        return visit_statement_children(walk, node, module_span, process_span,
                                        current_span, depth + 1U);
    }
    if (kind == LEMMA_SYNTAX_STATEMENT_LOOP) {
        TSNode body = first_loop_body(node);
        if (!ts_node_is_null(body)) {
            return visit_statement_node(walk, body, module_span, process_span,
                                        current_span, depth + 1U);
        }
    }
    return 1;
}

//--------------------------------------------------------
// Find the timing control and executable body inside an always statement.
//
// @node: Statement wrapper under an always construct.
// @control: Receives an event or delay control when found.
// @body: Receives the executable statement when found.
// @returns: Nonzero if a body statement was found.
//--------------------------------------------------------
static int find_process_parts(TSNode node, TSNode *control, TSNode *body)
{
    const char *type = ts_node_type(node);
    if (strcmp(type, "procedural_timing_control_statement") == 0) {
        *control = syntax_child_of_either(node, "event_control", "delay_control");
        *body = direct_statement_body(node);
        return !ts_node_is_null(*body);
    }
    if (strcmp(type, "statement") == 0 || strcmp(type, "statement_item") == 0 ||
        strcmp(type, "statement_or_null") == 0) {
        uint32_t count = ts_node_named_child_count(node);
        for (uint32_t index = 0U; index < count; ++index) {
            TSNode child = ts_node_named_child(node, index);
            if (!ts_node_is_extra(child) && find_process_parts(child, control, body)) {
                return 1;
            }
        }
        return 0;
    }
    *body = node;
    return 1;
}

//--------------------------------------------------------
// Emit individual edge or level expressions from a nested event expression.
//
// @walk: Active syntax traversal state.
// @node: Event-expression node or wrapper.
// @module_span: Span of the containing module.
// @process_span: Span of the process owning this event control.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_event_expression(syntax_walk *walk, TSNode node,
                                  lemma_ir_source_span module_span,
                                  lemma_ir_source_span process_span)
{
    if (strcmp(ts_node_type(node), "event_expression") != 0) {
        uint32_t count = ts_node_named_child_count(node);
        for (uint32_t index = 0U; index < count; ++index) {
            TSNode child = ts_node_named_child(node, index);
            if (!ts_node_is_extra(child) &&
                strcmp(ts_node_type(child), "event_expression") == 0 &&
                !visit_event_expression(walk, child, module_span, process_span)) {
                return 0;
            }
        }
        return 1;
    }

    size_t nested_count = 0U;
    uint32_t count = ts_node_named_child_count(node);
    for (uint32_t index = 0U; index < count; ++index) {
        TSNode child = ts_node_named_child(node, index);
        if (!ts_node_is_extra(child) &&
            strcmp(ts_node_type(child), "event_expression") == 0) {
            ++nested_count;
            if (!visit_event_expression(walk, child, module_span, process_span)) {
                return 0;
            }
        }
    }
    if (nested_count != 0U) {
        return 1;
    }

    TSNode edge = syntax_unique_child(node, "edge_identifier");
    TSNode expression = syntax_unique_child(node, "expression");
    if (ts_node_is_null(expression)) {
        return 1;
    }
    lemma_syntax_process_event_expression event = {0};
    event.kind = LEMMA_SYNTAX_EVENT_EXPRESSION_LEVEL;
    if (!ts_node_is_null(edge)) {
        lemma_syntax_text edge_text = syntax_text_for_node(walk->parsed, edge);
        if (edge_text.length == 7U && memcmp(edge_text.data, "posedge", 7U) == 0) {
            event.kind = LEMMA_SYNTAX_EVENT_EXPRESSION_POSEDGE;
        } else if (edge_text.length == 7U &&
                   memcmp(edge_text.data, "negedge", 7U) == 0) {
            event.kind = LEMMA_SYNTAX_EVENT_EXPRESSION_NEGEDGE;
        } else {
            event.kind = LEMMA_SYNTAX_EVENT_EXPRESSION_OTHER;
        }
    }
    event.expression = syntax_text_for_node(walk->parsed, expression);
    event.module_span = module_span;
    event.process_span = process_span;
    lemma_syntax_view view = {0};
    view.kind = LEMMA_SYNTAX_PROCESS_EVENT_EXPRESSION;
    view.as.process_event_expression = event;
    return syntax_emit(walk, &view);
}

//--------------------------------------------------------
// Visit one process construct, its event expressions, and its statements.
//
// @walk: Active syntax traversal state.
// @process_node: Process construct node.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_process(syntax_walk *walk, TSNode process_node,
                         lemma_ir_source_span module_span)
{
    TSNode keyword = syntax_unique_child(process_node, "always_keyword");
    TSNode statement = syntax_unique_child(process_node, "statement");
    if (ts_node_is_null(keyword) || ts_node_is_null(statement)) {
        return emit_opaque_module_item(walk, process_node, module_span);
    }
    lemma_syntax_text keyword_text = syntax_text_for_node(walk->parsed, keyword);
    lemma_syntax_process process = {0};
    process.kind = LEMMA_SYNTAX_PROCESS_OTHER;
    if (keyword_text.length == 6U && memcmp(keyword_text.data, "always", 6U) == 0) {
        process.kind = LEMMA_SYNTAX_PROCESS_ALWAYS;
    } else if (keyword_text.length == 11U &&
               memcmp(keyword_text.data, "always_comb", 11U) == 0) {
        process.kind = LEMMA_SYNTAX_PROCESS_ALWAYS_COMB;
    } else if (keyword_text.length == 9U &&
               memcmp(keyword_text.data, "always_ff", 9U) == 0) {
        process.kind = LEMMA_SYNTAX_PROCESS_ALWAYS_FF;
    } else if (keyword_text.length == 12U &&
               memcmp(keyword_text.data, "always_latch", 12U) == 0) {
        process.kind = LEMMA_SYNTAX_PROCESS_ALWAYS_LATCH;
    }

    TSNode control = {0};
    TSNode body = {0};
    if (!find_process_parts(statement, &control, &body)) {
        return emit_opaque_module_item(walk, process_node, module_span);
    }
    process.control_kind = LEMMA_SYNTAX_PROCESS_CONTROL_NONE;
    if (!ts_node_is_null(control)) {
        const char *control_type = ts_node_type(control);
        process.control_kind = strcmp(control_type, "event_control") == 0
                                   ? LEMMA_SYNTAX_PROCESS_CONTROL_EVENT
                                   : strcmp(control_type, "delay_control") == 0
                                         ? LEMMA_SYNTAX_PROCESS_CONTROL_DELAY
                                         : LEMMA_SYNTAX_PROCESS_CONTROL_OTHER;
        process.control = syntax_text_for_node(walk->parsed, control);
    }
    process.source = syntax_text_for_node(walk->parsed, process_node);
    process.module_span = module_span;
    process.process_span = process.source.span;
    lemma_syntax_view view = {0};
    view.kind = LEMMA_SYNTAX_PROCESS;
    view.as.process = process;
    if (!syntax_emit(walk, &view)) {
        return 0;
    }

    if (!ts_node_is_null(control) &&
        strcmp(ts_node_type(control), "event_control") == 0) {
        TSNode clocking = syntax_unique_child(control, "clocking_event");
        if (!ts_node_is_null(clocking) &&
            !visit_event_expression(walk, clocking, module_span,
                                    process.process_span)) {
            return 0;
        }
    }
    return visit_statement_node(walk, body, module_span, process.process_span,
                                process.process_span, 0U);
}

//--------------------------------------------------------
// Visit an ANSI header's parameter and port declarations.
//
// @walk: Active syntax traversal state.
// @header: Module header node.
// @module_span: Span of the containing module.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_module_header(syntax_walk *walk, TSNode header,
                               lemma_ir_source_span module_span)
{
    if (strcmp(ts_node_type(header), "module_ansi_header") != 0) {
        return emit_opaque_module_item(walk, header, module_span);
    }
    TSNode parameter_list = syntax_unique_child(header, "parameter_port_list");
    TSNode port_list = syntax_unique_child(header, "list_of_port_declarations");
    return visit_parameter_ports(walk, parameter_list, module_span) &&
           visit_ports(walk, port_list, module_span);
}

//--------------------------------------------------------
// Visit direct module-body items, dispatching each item by its grammar type.
//
// @walk: Active syntax traversal state.
// @module: Module declaration whose body is visited.
// @header: Header child skipped during body traversal, or null.
// @module_span: Span of the containing module.
// @returns: Nonzero after complete traversal; zero after stop or failure.
//--------------------------------------------------------
static int visit_body(syntax_walk *walk, TSNode module, TSNode header,
                      lemma_ir_source_span module_span)
{
    body_visit body = {walk, header, module_span};
    lemma_ts_child_visit_result result = lemma_frontend_ts_visit_children(
        module, LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS,
        visit_body_child, &body);
    return result == LEMMA_TS_CHILD_VISIT_COMPLETE;
}

//--------------------------------------------------------
// Visit one module boundary, its header, and its body.
//
// @walk: Active syntax traversal state.
// @module: Module declaration node.
// @returns: Nonzero to continue traversal; zero after visitor stop or failure.
//--------------------------------------------------------
static int visit_module(syntax_walk *walk, TSNode module)
{
    lemma_ir_source_span module_span = syntax_span_for_node(module);
    TSNode header = syntax_child_of_either(
        module, "module_ansi_header", "module_nonansi_header");
    TSNode name = syntax_unique_child(header, "simple_identifier");
    lemma_syntax_module_boundary boundary = {0};
    boundary.kind = LEMMA_SYNTAX_MODULE_BEGIN;
    boundary.name = syntax_text_for_node(walk->parsed, name);
    boundary.module_span = module_span;
    lemma_syntax_view begin_view = {0};
    begin_view.kind = LEMMA_SYNTAX_MODULE_BOUNDARY;
    begin_view.as.module_boundary = boundary;
    if (!syntax_emit(walk, &begin_view)) {
        return 0;
    }

    if (ts_node_is_null(header)) {
        if (!emit_opaque_module_item(walk, module, module_span)) {
            return 0;
        }
    } else if (!visit_module_header(walk, header, module_span)) {
        return 0;
    }

    if (!visit_body(walk, module, header, module_span)) {
        return 0;
    }

    boundary.kind = LEMMA_SYNTAX_MODULE_END;
    lemma_syntax_view end_view = {0};
    end_view.kind = LEMMA_SYNTAX_MODULE_BOUNDARY;
    end_view.as.module_boundary = boundary;
    return syntax_emit(walk, &end_view);
}

typedef struct source_visit {
    syntax_walk *walk;
} source_visit;

//--------------------------------------------------------
// Visit one source-file child, retaining unknown top-level syntax as opaque.
//
// @child: Direct source-file child inspected.
// @child_index: Original child index, unused by this callback.
// @field_name: Optional grammar field name, unused by this callback.
// @context: Source traversal state.
// @returns: CONTINUE for a visited item; STOP or FAIL on visitor result.
//--------------------------------------------------------
static lemma_ts_child_visit_action visit_source_child(
    TSNode child, uint32_t child_index, const char *field_name, void *context)
{
    (void)child_index;
    (void)field_name;
    source_visit *visit = context;
    if (strcmp(ts_node_type(child), "module_declaration") == 0) {
        if (visit_module(visit->walk, child)) {
            return LEMMA_TS_CHILD_VISIT_CONTINUE;
        }
    } else {
        if (emit_opaque_module_item(visit->walk, child, (lemma_ir_source_span){0})) {
            return LEMMA_TS_CHILD_VISIT_CONTINUE;
        }
    }
    return visit->walk->result == LEMMA_SYNTAX_VISIT_STOPPED
               ? LEMMA_TS_CHILD_VISIT_STOP
               : LEMMA_TS_CHILD_VISIT_FAIL;
}

//--------------------------------------------------------
// Visit source modules and their syntax events without allocating a collection.
//
// @parsed: Live parsed-source handle, borrowed for the duration of the visit.
// @visitor: Non-NULL callback invoked for each source-ordered syntax event.
// @context: Caller-provided context borrowed for the duration of the visit.
// @returns: COMPLETE after traversal, STOPPED after visitor stop, FAILED after
//     visitor failure, or INVALID_INPUT for a null handle or visitor.
//--------------------------------------------------------
lemma_syntax_visit_result lemma_frontend_visit_syntax(
    const lemma_parsed_source *parsed, lemma_syntax_visitor visitor,
    void *context)
{
    if (parsed == NULL || parsed->tree == NULL || parsed->source == NULL ||
        visitor == NULL) {
        return LEMMA_SYNTAX_VISIT_INVALID_INPUT;
    }
    syntax_walk walk = {
        parsed, visitor, context, LEMMA_SYNTAX_VISIT_COMPLETE
    };
    TSNode root = ts_tree_root_node(parsed->tree);
    source_visit visit = {&walk};
    lemma_ts_child_visit_result result = lemma_frontend_ts_visit_children(
        root, LEMMA_TS_CHILD_FILTER_NAMED_EXCLUDING_EXTRAS,
        visit_source_child, &visit);
    if (result == LEMMA_TS_CHILD_VISIT_INVALID_INPUT) {
        return LEMMA_SYNTAX_VISIT_INVALID_INPUT;
    }
    if (result == LEMMA_TS_CHILD_VISIT_FAILED) {
        return LEMMA_SYNTAX_VISIT_FAILED;
    }
    if (walk.result != LEMMA_SYNTAX_VISIT_COMPLETE) {
        return walk.result;
    }
    return result == LEMMA_TS_CHILD_VISIT_STOPPED
               ? LEMMA_SYNTAX_VISIT_STOPPED
               : LEMMA_SYNTAX_VISIT_COMPLETE;
}
