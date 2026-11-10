#ifndef LEMMA_FRONTEND_SYNTAX_H
#define LEMMA_FRONTEND_SYNTAX_H

#include "frontend_parse.h"
#include "lemma/source_range.h"

#include <stdbool.h>
#include <stddef.h>

// Borrowed, non-NUL-terminated source text with a half-open source range.
// An absent optional slice has data == NULL and length == 0; a present empty
// slice may have a non-NULL data pointer and length == 0.
typedef struct lemma_syntax_text {
    const char *data;
    size_t length;
    lemma_source_range source_range;
} lemma_syntax_text;

// Kinds of module-boundary callbacks.
typedef enum lemma_syntax_module_boundary_kind {
    LEMMA_SYNTAX_MODULE_BEGIN,
    LEMMA_SYNTAX_MODULE_END
} lemma_syntax_module_boundary_kind;

// A module boundary identifies the full module and its borrowed name slice.
typedef struct lemma_syntax_module_boundary {
    lemma_syntax_module_boundary_kind kind;
    lemma_syntax_text name;
    lemma_source_range module_range;
} lemma_syntax_module_boundary;

// Syntactic declaration categories; these do not imply RTL acceptance or
// inferred signal semantics.
typedef enum lemma_syntax_declaration_group_kind {
    LEMMA_SYNTAX_DECLARATION_GROUP_PARAMETER,
    LEMMA_SYNTAX_DECLARATION_GROUP_LOCALPARAM,
    LEMMA_SYNTAX_DECLARATION_GROUP_PORT,
    LEMMA_SYNTAX_DECLARATION_GROUP_NET,
    LEMMA_SYNTAX_DECLARATION_GROUP_DATA,
    LEMMA_SYNTAX_DECLARATION_GROUP_OPAQUE
} lemma_syntax_declaration_group_kind;

// Syntax-only type category. OTHER covers unsupported built-in, user-defined,
// and escaped type forms; classification never comes from raw spelling.
typedef enum lemma_syntax_builtin_type_kind {
    LEMMA_SYNTAX_BUILTIN_TYPE_UNSPECIFIED,
    LEMMA_SYNTAX_BUILTIN_TYPE_WIRE,
    LEMMA_SYNTAX_BUILTIN_TYPE_LOGIC,
    LEMMA_SYNTAX_BUILTIN_TYPE_OTHER
} lemma_syntax_builtin_type_kind;

// Syntax-only signedness marker present in a declaration type.
typedef enum lemma_syntax_signedness_kind {
    LEMMA_SYNTAX_SIGNEDNESS_UNSPECIFIED,
    LEMMA_SYNTAX_SIGNEDNESS_SIGNED,
    LEMMA_SYNTAX_SIGNEDNESS_UNSIGNED,
    LEMMA_SYNTAX_SIGNEDNESS_OTHER
} lemma_syntax_signedness_kind;

// Common raw syntax for a declaration group. source includes the full group
// range; parameter values remain unevaluated source slices. The explicit port
// header flag applies only to ANSI port syntax, records only syntax present on
// this group, is false for other forms, and never signals inherited direction
// or type attributes. type, source, and packed_dimensions retain raw provenance.
// The typed facts describe only syntax: bound slices name the first packed
// dimension, while packed_dimension_count preserves additional dimensions.
typedef struct lemma_syntax_declaration_group {
    lemma_syntax_declaration_group_kind kind;
    lemma_syntax_text direction;
    lemma_syntax_text type;
    lemma_syntax_text packed_dimensions;
    lemma_syntax_text packed_left_bound;
    lemma_syntax_text packed_right_bound;
    lemma_syntax_text source;
    lemma_source_range module_range;
    lemma_syntax_builtin_type_kind builtin_type;
    lemma_syntax_signedness_kind signedness;
    size_t packed_dimension_count;
    bool has_var_keyword;
    bool has_explicit_port_header;
} lemma_syntax_declaration_group;

// One declared name in source order. suffix (including any unpacked dimensions)
// and initializer_or_default_expression are raw per-name slices; group_range and
// module_range identify its group context.
typedef struct lemma_syntax_declarator {
    lemma_syntax_text name;
    lemma_syntax_text suffix;
    lemma_syntax_text initializer_or_default_expression;
    lemma_source_range declarator_range;
    lemma_source_range group_range;
    lemma_source_range module_range;
} lemma_syntax_declarator;

// One continuous assignment pair. lvalue/expression preserve their full syntax;
// destination_signal is set only for a simple identifier, and condition is set
// only for a top-level ternary expression condition.
typedef struct lemma_syntax_continuous_assignment {
    lemma_syntax_text lvalue;
    lemma_syntax_text destination_signal;
    lemma_syntax_text expression;
    lemma_syntax_text condition;
    lemma_syntax_text assignment;
    lemma_source_range module_range;
} lemma_syntax_continuous_assignment;

// Syntactic process keyword categories; OTHER preserves an unclassified
// process keyword without assigning it behavioral meaning.
typedef enum lemma_syntax_process_kind {
    LEMMA_SYNTAX_PROCESS_ALWAYS,
    LEMMA_SYNTAX_PROCESS_ALWAYS_COMB,
    LEMMA_SYNTAX_PROCESS_ALWAYS_FF,
    LEMMA_SYNTAX_PROCESS_ALWAYS_LATCH,
    LEMMA_SYNTAX_PROCESS_OTHER
} lemma_syntax_process_kind;

// Syntactic kind of control following a process keyword.
typedef enum lemma_syntax_process_control_kind {
    LEMMA_SYNTAX_PROCESS_CONTROL_NONE,
    LEMMA_SYNTAX_PROCESS_CONTROL_EVENT,
    LEMMA_SYNTAX_PROCESS_CONTROL_DELAY,
    LEMMA_SYNTAX_PROCESS_CONTROL_OTHER
} lemma_syntax_process_control_kind;

// A process construct and its optional raw control syntax.
typedef struct lemma_syntax_process {
    lemma_syntax_process_kind kind;
    lemma_syntax_process_control_kind control_kind;
    lemma_syntax_text control;
    lemma_syntax_text source;
    lemma_source_range module_range;
    lemma_source_range process_range;
} lemma_syntax_process;

// Syntactic categories for individual expressions in an event control.
typedef enum lemma_syntax_event_expression_kind {
    LEMMA_SYNTAX_EVENT_EXPRESSION_LEVEL,
    LEMMA_SYNTAX_EVENT_EXPRESSION_POSEDGE,
    LEMMA_SYNTAX_EVENT_EXPRESSION_NEGEDGE,
    LEMMA_SYNTAX_EVENT_EXPRESSION_OTHER
} lemma_syntax_event_expression_kind;

// One event-control expression, in source order, with its raw expression text.
typedef struct lemma_syntax_process_event_expression {
    lemma_syntax_event_expression_kind kind;
    lemma_syntax_text expression;
    lemma_source_range module_range;
    lemma_source_range process_range;
} lemma_syntax_process_event_expression;

// Syntactic statement classes; OPAQUE marks unrecognized statement syntax.
typedef enum lemma_syntax_statement_kind {
    LEMMA_SYNTAX_STATEMENT_BLOCK,
    LEMMA_SYNTAX_STATEMENT_CONDITIONAL,
    LEMMA_SYNTAX_STATEMENT_CASE,
    LEMMA_SYNTAX_STATEMENT_CASE_ITEM,
    LEMMA_SYNTAX_STATEMENT_BLOCKING_ASSIGNMENT,
    LEMMA_SYNTAX_STATEMENT_NONBLOCKING_ASSIGNMENT,
    LEMMA_SYNTAX_STATEMENT_LOOP,
    LEMMA_SYNTAX_STATEMENT_OTHER,
    LEMMA_SYNTAX_STATEMENT_OPAQUE
} lemma_syntax_statement_kind;

// One statement syntax event. header is the raw construct-specific non-body
// syntax when available (for example a condition, case label, or loop header).
// Statement events are preorder and retain their immediate syntactic parent
// and depth so callers can reconstruct structure.
typedef struct lemma_syntax_statement {
    lemma_syntax_statement_kind kind;
    lemma_syntax_text source;
    lemma_syntax_text header;
    lemma_syntax_text lvalue;
    lemma_syntax_text expression;
    lemma_source_range module_range;
    lemma_source_range process_range;
    lemma_source_range parent_range;
    size_t depth;
} lemma_syntax_statement;

// One module-scope item that the extractor cannot classify; the source slice
// preserves the item without interpreting or dropping it.
typedef struct lemma_syntax_opaque_module_item {
    lemma_syntax_text source;
    lemma_source_range module_range;
} lemma_syntax_opaque_module_item;

// Synchronous extraction events are ordered within module and process groups.
typedef enum lemma_syntax_kind {
    LEMMA_SYNTAX_MODULE_BOUNDARY,
    LEMMA_SYNTAX_DECLARATION_GROUP,
    LEMMA_SYNTAX_DECLARATOR,
    LEMMA_SYNTAX_CONTINUOUS_ASSIGNMENT,
    LEMMA_SYNTAX_PROCESS,
    LEMMA_SYNTAX_PROCESS_EVENT_EXPRESSION,
    LEMMA_SYNTAX_STATEMENT,
    LEMMA_SYNTAX_OPAQUE_MODULE_ITEM
} lemma_syntax_kind;

// Typed borrowed view of one syntax event; the active union member is selected
// by kind and remains valid only during the visitor callback.
typedef struct lemma_syntax_view {
    lemma_syntax_kind kind;
    union {
        lemma_syntax_module_boundary module_boundary;
        lemma_syntax_declaration_group declaration_group;
        lemma_syntax_declarator declarator;
        lemma_syntax_continuous_assignment continuous_assignment;
        lemma_syntax_process process;
        lemma_syntax_process_event_expression process_event_expression;
        lemma_syntax_statement statement;
        lemma_syntax_opaque_module_item opaque_module_item;
    } as;
} lemma_syntax_view;

// The visitor chooses whether extraction continues, stops normally, or fails.
typedef enum lemma_syntax_visit_action {
    LEMMA_SYNTAX_VISIT_CONTINUE,
    LEMMA_SYNTAX_VISIT_STOP,
    LEMMA_SYNTAX_VISIT_FAIL
} lemma_syntax_visit_action;

//--------------------------------------------------------
// Handle one syntax event synchronously in source order; views, slices, and
// ranges must not be retained after the callback.
//
// @view: Borrowed typed view of the current syntax event, valid only in the call.
// @context: Caller-provided context borrowed for the duration of the visit.
// @returns: CONTINUE to proceed, STOP to end normally without another callback,
//     or FAIL to return a failed visit result.
//--------------------------------------------------------
typedef lemma_syntax_visit_action (*lemma_syntax_visitor)(
    const lemma_syntax_view *view, void *context);

// Result distinguishes full traversal, visitor-requested stop, failure, and
// invalid arguments.
typedef enum lemma_syntax_visit_result {
    LEMMA_SYNTAX_VISIT_COMPLETE,
    LEMMA_SYNTAX_VISIT_STOPPED,
    LEMMA_SYNTAX_VISIT_FAILED,
    LEMMA_SYNTAX_VISIT_INVALID_INPUT
} lemma_syntax_visit_result;

//--------------------------------------------------------
// Visit ordered syntax events synchronously without allocation or inferring RTL
// meaning. A declaration-group event precedes its per-name declarator events;
// names retain source order and each declarator carries its group range. ANSI
// shorthand port identifiers have a group with a false explicit-header flag
// and absent common attribute slices, rather than copied or inferred attributes.
// Module events group source-ordered items; process events precede event
// expressions and preorder statements. Root statements name their process as
// parent at depth zero; nested statements name their immediate parent and
// increase depth by one. Unknown module items and statements retain opaque
// source slices.
//
// @parsed: Live parsed-source handle, borrowed for the duration of the visit.
// @visitor: Non-NULL callback invoked synchronously for each emitted event.
// @context: Caller-provided context borrowed for the duration of the visit.
// @returns: COMPLETE after traversal, STOPPED after visitor stop, FAILED after
//     visitor failure or an invalid action, or INVALID_INPUT for a NULL handle
//     or visitor; invalid input does not invoke the visitor.
//--------------------------------------------------------
lemma_syntax_visit_result lemma_frontend_visit_syntax(
    const lemma_parsed_source *parsed, lemma_syntax_visitor visitor,
    void *context);

#endif
