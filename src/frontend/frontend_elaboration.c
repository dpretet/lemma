#include "lemma/circuit_ir_builder_internal.h"
#include "lemma/frontend_elaboration.h"
#include "lemma/frontend_syntax.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct port_attributes {
    lemma_ir_port_direction direction;
    uint32_t width;
    bool is_signed;
    int valid;
} port_attributes;

#define LEMMA_ELABORATION_EXPRESSION_MAX_DEPTH 128U

typedef struct expression_frame {
    lemma_source_range source_range;
    uint32_t operand_ids[2];
    unsigned char operand_present[2];
    int active;
} expression_frame;

typedef struct elaboration_context {
    lemma_circuit_builder *builder;
    lemma_frontend_diagnostic *diagnostic;
    lemma_source_range module_range;
    lemma_source_range pending_group_range;
    lemma_source_range pending_assignment_range;
    port_attributes current_port;
    expression_frame expression_frames[LEMMA_ELABORATION_EXPRESSION_MAX_DEPTH];
    uint32_t pending_target_signal_id;
    size_t module_count;
    int has_pending_port;
    int has_pending_assignment;
    int failed;
} elaboration_context;

//--------------------------------------------------------
// Compare two source ranges field by field without relying on structure padding.
//
// @left: First half-open source range.
// @right: Second half-open source range.
// @returns: Nonzero when every byte offset and point field matches.
//--------------------------------------------------------
static int ranges_equal(lemma_source_range left, lemma_source_range right)
{
    return left.start_byte == right.start_byte && left.end_byte == right.end_byte &&
           left.start_row == right.start_row && left.start_column == right.start_column &&
           left.end_row == right.end_row && left.end_column == right.end_column;
}

//--------------------------------------------------------
// Clear and set an optional frontend diagnostic.
//
// @diagnostic: Optional caller-owned diagnostic output.
// @stage: Frontend stage that detected the failure.
// @code: Static diagnostic code string.
// @message: Concise diagnostic message.
// @range: Source range attached when has_source_range is nonzero.
// @has_source_range: Whether a source range is available.
// @returns: None.
//--------------------------------------------------------
static void set_diagnostic(lemma_frontend_diagnostic *diagnostic,
                           lemma_frontend_stage stage, const char *code,
                           const char *message, lemma_source_range range,
                           int has_source_range)
{
    if (diagnostic == NULL) {
        return;
    }
    memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->stage = stage;
    diagnostic->severity = LEMMA_FRONTEND_DIAGNOSTIC_SEVERITY_ERROR;
    diagnostic->code = code;
    (void)snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    if (has_source_range != 0) {
        diagnostic->has_source_range = 1;
        diagnostic->source_range = range;
    }
}

//--------------------------------------------------------
// Report an RTL validation/elaboration failure to the visitor caller.
//
// @context: Active elaboration state receiving the diagnostic.
// @code: Static diagnostic code string.
// @message: Concise diagnostic message.
// @range: Source range attached when available.
// @has_source_range: Whether a source range is available.
// @returns: FAIL so syntax visitation stops immediately.
//--------------------------------------------------------
static lemma_syntax_visit_action reject_syntax(
    elaboration_context *context, const char *code, const char *message,
    lemma_source_range range, int has_source_range)
{
    context->failed = 1;
    set_diagnostic(context->diagnostic,
                   LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                   code, message, range, has_source_range);
    return LEMMA_SYNTAX_VISIT_FAIL;
}

//--------------------------------------------------------
// Report a circuit-builder failure and stop syntax elaboration.
//
// @context: Active elaboration state receiving the diagnostic.
// @code: Static circuit-IR diagnostic code.
// @message: Concise diagnostic message.
// @range: Source range attached when available.
// @has_source_range: Whether a source range is available.
// @returns: FAIL so syntax visitation stops immediately.
//--------------------------------------------------------
static lemma_syntax_visit_action reject_builder(
    elaboration_context *context, const char *code, const char *message,
    lemma_source_range range, int has_source_range)
{
    context->failed = 1;
    set_diagnostic(context->diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                   code, message, range, has_source_range);
    return LEMMA_SYNTAX_VISIT_FAIL;
}

//--------------------------------------------------------
// Compare a borrowed source slice with exact text.
//
// @text: Borrowed source slice to compare.
// @expected: NUL-terminated expected token text.
// @returns: Nonzero when the slice exactly matches expected.
//--------------------------------------------------------
static int text_equals(lemma_syntax_text text, const char *expected)
{
    size_t length = strlen(expected);
    return text.data != NULL && text.length == length &&
           memcmp(text.data, expected, length) == 0;
}

//--------------------------------------------------------
// Check whether a byte is a simple SystemVerilog identifier character.
//
// @character: Byte to classify.
// @first: Nonzero when character is at the start of an identifier.
// @returns: Nonzero when character is valid at the requested position.
//--------------------------------------------------------
static int identifier_character(unsigned char character, int first)
{
    if ((character >= (unsigned char)'A' && character <= (unsigned char)'Z') ||
        (character >= (unsigned char)'a' && character <= (unsigned char)'z') ||
        character == (unsigned char)'_') {
        return 1;
    }
    return first == 0 &&
           ((character >= (unsigned char)'0' && character <= (unsigned char)'9') ||
            character == (unsigned char)'$');
}

//--------------------------------------------------------
// Check whether a source slice contains exactly one simple identifier.
//
// @text: Borrowed source slice to validate.
// @returns: Nonzero for a simple identifier, zero for other syntax.
//--------------------------------------------------------
static int is_simple_identifier(lemma_syntax_text text)
{
    if (text.data == NULL || text.length == 0U ||
        !identifier_character((unsigned char)text.data[0], 1)) {
        return 0;
    }
    for (size_t index = 1U; index < text.length; ++index) {
        if (!identifier_character((unsigned char)text.data[index], 0)) {
            return 0;
        }
    }
    return 1;
}

//--------------------------------------------------------
// Store one completed expression as an operand of its syntax parent.
//
// @context: Active elaboration state containing bounded expression frames.
// @expression: Completed syntax node with its parent range and child index.
// @expression_id: Existing IR expression ID produced for this node.
// @returns: Nonzero when the parent link is valid and stored.
//--------------------------------------------------------
static int store_expression_child(elaboration_context *context,
                                  const lemma_syntax_expression *expression,
                                  uint32_t expression_id)
{
    if (expression->depth == 0U ||
        expression->depth >= LEMMA_ELABORATION_EXPRESSION_MAX_DEPTH ||
        expression->child_index >= 2U) {
        return 0;
    }
    expression_frame *frame = &context->expression_frames[expression->depth - 1U];
    if (frame->active == 0) {
        memset(frame, 0, sizeof(*frame));
        frame->active = 1;
        frame->source_range = expression->parent_range;
    } else if (!ranges_equal(frame->source_range, expression->parent_range)) {
        return 0;
    }
    size_t child_index = expression->child_index;
    if (frame->operand_present[child_index] != 0U) {
        return 0;
    }
    frame->operand_ids[child_index] = expression_id;
    frame->operand_present[child_index] = 1U;
    return 1;
}

//--------------------------------------------------------
// Retrieve and consume the source-ordered child expression IDs of one node.
//
// @context: Active elaboration state containing bounded expression frames.
// @expression: Parent syntax node requesting its operands.
// @operand_ids: Receives the one or two child IDs in source order.
// @returns: Nonzero when all declared child links match this parent range.
//--------------------------------------------------------
static int take_expression_children(elaboration_context *context,
                                   const lemma_syntax_expression *expression,
                                   uint32_t operand_ids[2])
{
    if (expression->depth >= LEMMA_ELABORATION_EXPRESSION_MAX_DEPTH ||
        expression->operand_count == 0U || expression->operand_count > 2U) {
        return 0;
    }
    expression_frame *frame = &context->expression_frames[expression->depth];
    if (frame->active == 0 ||
        !ranges_equal(frame->source_range, expression->source.source_range)) {
        return 0;
    }
    for (size_t index = 0U; index < expression->operand_count; ++index) {
        if (frame->operand_present[index] == 0U) {
            return 0;
        }
        operand_ids[index] = frame->operand_ids[index];
    }
    memset(frame, 0, sizeof(*frame));
    return 1;
}

//--------------------------------------------------------
// Map a classified syntax operator to its typed circuit-IR operator kind.
//
// @syntax_kind: Borrowed syntax category to map.
// @ir_kind: Receives the corresponding IR kind on success.
// @returns: Nonzero for a supported typed operator; zero for other syntax.
//--------------------------------------------------------
static int syntax_operator_ir_kind(lemma_syntax_expression_kind syntax_kind,
                                   lemma_ir_expression_kind *ir_kind)
{
    switch (syntax_kind) {
    case LEMMA_SYNTAX_EXPRESSION_BITWISE_NOT:
        *ir_kind = LEMMA_IR_EXPRESSION_BITWISE_NOT;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_BITWISE_AND:
        *ir_kind = LEMMA_IR_EXPRESSION_BITWISE_AND;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_BITWISE_OR:
        *ir_kind = LEMMA_IR_EXPRESSION_BITWISE_OR;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_BITWISE_XOR:
        *ir_kind = LEMMA_IR_EXPRESSION_BITWISE_XOR;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_BITWISE_XNOR:
        *ir_kind = LEMMA_IR_EXPRESSION_BITWISE_XNOR;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_REDUCTION_AND:
        *ir_kind = LEMMA_IR_EXPRESSION_REDUCTION_AND;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_REDUCTION_NAND:
        *ir_kind = LEMMA_IR_EXPRESSION_REDUCTION_NAND;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_REDUCTION_OR:
        *ir_kind = LEMMA_IR_EXPRESSION_REDUCTION_OR;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_REDUCTION_NOR:
        *ir_kind = LEMMA_IR_EXPRESSION_REDUCTION_NOR;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_REDUCTION_XOR:
        *ir_kind = LEMMA_IR_EXPRESSION_REDUCTION_XOR;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_REDUCTION_XNOR:
        *ir_kind = LEMMA_IR_EXPRESSION_REDUCTION_XNOR;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_ADD:
        *ir_kind = LEMMA_IR_EXPRESSION_ADD;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_SUBTRACT:
        *ir_kind = LEMMA_IR_EXPRESSION_SUBTRACT;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_MULTIPLY:
        *ir_kind = LEMMA_IR_EXPRESSION_MULTIPLY;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_EQUAL:
        *ir_kind = LEMMA_IR_EXPRESSION_EQUAL;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_NOT_EQUAL:
        *ir_kind = LEMMA_IR_EXPRESSION_NOT_EQUAL;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_LESS:
        *ir_kind = LEMMA_IR_EXPRESSION_LESS;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_LESS_EQUAL:
        *ir_kind = LEMMA_IR_EXPRESSION_LESS_EQUAL;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_GREATER:
        *ir_kind = LEMMA_IR_EXPRESSION_GREATER;
        return 1;
    case LEMMA_SYNTAX_EXPRESSION_GREATER_EQUAL:
        *ir_kind = LEMMA_IR_EXPRESSION_GREATER_EQUAL;
        return 1;
    default:
        return 0;
    }
}

//--------------------------------------------------------
// Return the arity encoded by one supported circuit-IR expression kind.
//
// @kind: Expression kind inspected.
// @operand_count: Receives zero, one, or two for a recognized kind.
// @returns: Nonzero for read, logical, and fixture-bounded bit-vector kinds.
//--------------------------------------------------------
static int ir_expression_operand_count(lemma_ir_expression_kind kind,
                                      size_t *operand_count)
{
    switch (kind) {
    case LEMMA_IR_EXPRESSION_READ_SIGNAL:
        *operand_count = 0U;
        return 1;
    case LEMMA_IR_EXPRESSION_LOGICAL_NOT:
    case LEMMA_IR_EXPRESSION_BITWISE_NOT:
    case LEMMA_IR_EXPRESSION_REDUCTION_AND:
    case LEMMA_IR_EXPRESSION_REDUCTION_NAND:
    case LEMMA_IR_EXPRESSION_REDUCTION_OR:
    case LEMMA_IR_EXPRESSION_REDUCTION_NOR:
    case LEMMA_IR_EXPRESSION_REDUCTION_XOR:
    case LEMMA_IR_EXPRESSION_REDUCTION_XNOR:
        *operand_count = 1U;
        return 1;
    case LEMMA_IR_EXPRESSION_LOGICAL_AND:
    case LEMMA_IR_EXPRESSION_LOGICAL_OR:
    case LEMMA_IR_EXPRESSION_BITWISE_AND:
    case LEMMA_IR_EXPRESSION_BITWISE_OR:
    case LEMMA_IR_EXPRESSION_BITWISE_XOR:
    case LEMMA_IR_EXPRESSION_BITWISE_XNOR:
    case LEMMA_IR_EXPRESSION_ADD:
    case LEMMA_IR_EXPRESSION_SUBTRACT:
    case LEMMA_IR_EXPRESSION_MULTIPLY:
    case LEMMA_IR_EXPRESSION_EQUAL:
    case LEMMA_IR_EXPRESSION_NOT_EQUAL:
    case LEMMA_IR_EXPRESSION_LESS:
    case LEMMA_IR_EXPRESSION_LESS_EQUAL:
    case LEMMA_IR_EXPRESSION_GREATER:
    case LEMMA_IR_EXPRESSION_GREATER_EQUAL:
        *operand_count = 2U;
        return 1;
    default:
        return 0;
    }
}

//--------------------------------------------------------
// Lower one supported syntax expression node into a typed IR expression.
//
// @context: Active elaboration state and circuit builder.
// @expression: Borrowed syntax expression event with its provenance.
// @expression_id: Receives the resulting IR ID on success.
// @returns: 1 on success, 0 for invalid parent links, or -1 when the builder fails.
//--------------------------------------------------------
static int lower_expression_node(elaboration_context *context,
                                 const lemma_syntax_expression *expression,
                                 uint32_t *expression_id)
{
    uint32_t operands[2] = {0U, 0U};
    if (!take_expression_children(context, expression, operands)) {
        return 0;
    }
    if (expression->kind == LEMMA_SYNTAX_EXPRESSION_PARENTHESIZED) {
        if (expression->operand_count != 1U) {
            return 0;
        }
        *expression_id = operands[0];
        return 1;
    }
    if (expression->kind == LEMMA_SYNTAX_EXPRESSION_LOGICAL_NOT) {
        if (expression->operand_count != 1U) {
            return 0;
        }
        return lemma_circuit_builder_add_logical_not(
                   context->builder, operands[0],
                   expression->source.source_range, expression_id)
            ? 1
            : -1;
    }
    if (expression->kind == LEMMA_SYNTAX_EXPRESSION_LOGICAL_AND ||
        expression->kind == LEMMA_SYNTAX_EXPRESSION_LOGICAL_OR) {
        if (expression->operand_count != 2U) {
            return 0;
        }
        lemma_ir_expression_kind kind =
            expression->kind == LEMMA_SYNTAX_EXPRESSION_LOGICAL_AND
                ? LEMMA_IR_EXPRESSION_LOGICAL_AND
                : LEMMA_IR_EXPRESSION_LOGICAL_OR;
        return lemma_circuit_builder_add_logical_binary(
                   context->builder, kind, operands[0], operands[1],
                   expression->source.source_range, expression_id)
             ? 1
             : -1;
    }
    lemma_ir_expression_kind kind = LEMMA_IR_EXPRESSION_READ_SIGNAL;
    if (syntax_operator_ir_kind(expression->kind, &kind)) {
        size_t expected_count = 0U;
        if (!ir_expression_operand_count(kind, &expected_count) ||
            expression->operand_count != expected_count) {
            return 0;
        }
        const lemma_ir_expression *left =
            lemma_circuit_builder_expression_at(context->builder, operands[0]);
        const lemma_ir_expression *right = expression->operand_count == 2U
            ? lemma_circuit_builder_expression_at(context->builder, operands[1])
            : NULL;
        if (left->is_signed || (right != NULL && right->is_signed)) {
            context->failed = 1;
            set_diagnostic(context->diagnostic,
                           LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.operator_signedness",
                           "bitwise, arithmetic, and comparison operands must be unsigned",
                           expression->source.source_range, 1);
            return -2;
        }
        if (right != NULL && left->width != right->width) {
            context->failed = 1;
            set_diagnostic(context->diagnostic,
                           LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.operator_width",
                           "binary operator operands must have the same width",
                           expression->source.source_range, 1);
            return -2;
        }
        return lemma_circuit_builder_add_operator(
                   context->builder, kind, operands, expression->operand_count,
                   expression->source.source_range, expression_id)
            ? 1
            : -1;
    }
    return 0;
}

//--------------------------------------------------------
// Parse a borrowed unsigned decimal bound and reject non-decimal syntax.
//
// @text: Borrowed bound slice extracted from one packed dimension.
// @value: Non-NULL output for the parsed bound.
// @returns: Nonzero for a nonempty, in-range decimal number with no extra text.
//--------------------------------------------------------
static int parse_decimal_bound(lemma_syntax_text text, uint32_t *value)
{
    if (text.data == NULL || text.length == 0U) {
        return 0;
    }
    uint32_t result = 0U;
    for (size_t index = 0U; index < text.length; ++index) {
        unsigned char character = (unsigned char)text.data[index];
        if (character < (unsigned char)'0' || character > (unsigned char)'9') {
            return 0;
        }
        uint32_t digit = (uint32_t)(character - (unsigned char)'0');
        if (result > (UINT32_MAX - digit) / 10U) {
            return 0;
        }
        result = result * 10U + digit;
    }
    *value = result;
    return 1;
}

//--------------------------------------------------------
// Resolve one supported packed dimension's decimal bounds to a positive width.
//
// @group: Non-NULL declaration group with borrowed first-dimension bound slices.
// @width: Non-NULL output for the resolved bit width.
// @returns: Nonzero for a scalar or exactly one valid decimal dimension.
//--------------------------------------------------------
static int packed_width(const lemma_syntax_declaration_group *group,
                        uint32_t *width)
{
    if (group->packed_dimension_count == 0U) {
        *width = 1U;
        return 1;
    }
    if (group->packed_dimension_count != 1U) {
        return 0;
    }
    uint32_t first = 0U;
    uint32_t last = 0U;
    if (!parse_decimal_bound(group->packed_left_bound, &first) ||
        !parse_decimal_bound(group->packed_right_bound, &last)) {
        return 0;
    }
    uint32_t difference = first >= last ? first - last : last - first;
    if (difference == UINT32_MAX) {
        return 0;
    }
    *width = difference + 1U;
    return 1;
}

//--------------------------------------------------------
// Resolve supported explicit port attributes without inheriting omitted syntax.
//
// @group: Explicit ANSI port declaration group.
// @attributes: Receives direction, width, signedness, and validity.
// @returns: Nonzero for a supported wire/logic input or output declaration.
//--------------------------------------------------------
static int parse_explicit_port_attributes(
    const lemma_syntax_declaration_group *group, port_attributes *attributes)
{
    memset(attributes, 0, sizeof(*attributes));
    if (group->has_var_keyword) {
        return 0;
    }
    if (text_equals(group->direction, "input")) {
        attributes->direction = LEMMA_IR_PORT_INPUT;
    } else if (text_equals(group->direction, "output")) {
        attributes->direction = LEMMA_IR_PORT_OUTPUT;
    } else {
        return 0;
    }
    if (group->builtin_type != LEMMA_SYNTAX_BUILTIN_TYPE_WIRE &&
        group->builtin_type != LEMMA_SYNTAX_BUILTIN_TYPE_LOGIC) {
        return 0;
    }
    if (group->signedness == LEMMA_SYNTAX_SIGNEDNESS_OTHER) {
        return 0;
    }
    attributes->is_signed =
        group->signedness == LEMMA_SYNTAX_SIGNEDNESS_SIGNED;
    if (!packed_width(group, &attributes->width)) {
        return 0;
    }
    attributes->valid = 1;
    return 1;
}

//--------------------------------------------------------
// Count drivers currently targeting one signal in the builder.
//
// @builder: Builder whose final IR driver table is inspected.
// @signal_id: Target signal ID being counted.
// @returns: Number of drivers currently targeting signal_id.
//--------------------------------------------------------
static size_t count_signal_drivers(const lemma_circuit_builder *builder,
                                   uint32_t signal_id)
{
    size_t count = 0U;
    size_t driver_count = lemma_circuit_builder_driver_count(builder);
    for (size_t index = 0U; index < driver_count; ++index) {
        const lemma_ir_driver *driver = lemma_circuit_builder_driver_at(builder, index);
        if (driver->target_signal_id == signal_id) {
            ++count;
        }
    }
    return count;
}

// One adjacency-list edge from a driven signal to a referenced signal.
typedef struct signal_dependency_edge {
    uint32_t source_signal_id;
    size_t next_edge;
} signal_dependency_edge;

//--------------------------------------------------------
// Append one signal dependency edge, growing its storage with checked arithmetic.
//
// @edges: Mutable edge array pointer.
// @edge_count: Number of edges currently stored.
// @edge_capacity: Current edge-array capacity.
// @heads: Per-target adjacency-list heads.
// @target_signal_id: Signal driven by the expression.
// @source_signal_id: Signal read by the expression.
// @returns: Nonzero on success; zero on size overflow or allocation failure.
//--------------------------------------------------------
static int append_dependency_edge(signal_dependency_edge **edges,
                                  size_t *edge_count, size_t *edge_capacity,
                                  size_t *heads, uint32_t target_signal_id,
                                  uint32_t source_signal_id)
{
    if (*edge_count == *edge_capacity) {
        size_t next_capacity = *edge_capacity == 0U ? 8U : *edge_capacity * 2U;
        if (next_capacity < *edge_capacity ||
            next_capacity > SIZE_MAX / sizeof(**edges)) {
            return 0;
        }
        signal_dependency_edge *grown = realloc(
            *edges, next_capacity * sizeof(**edges));
        if (grown == NULL) {
            return 0;
        }
        *edges = grown;
        *edge_capacity = next_capacity;
    }
    size_t index = (*edge_count)++;
    (*edges)[index] = (signal_dependency_edge){
        source_signal_id, heads[target_signal_id]
    };
    heads[target_signal_id] = index;
    return 1;
}

//--------------------------------------------------------
// Detect cycles in the signal dependencies of all driver expressions.
//
// @builder: Valid builder with at least one signal; stored driver, expression,
//     and signal IDs refer to entries in their respective arrays.
// @cycle_signal_id: Required output receiving zero or a signal in the detected cycle.
// @returns: 1 for acyclic, 0 for a cycle, or -1 for temporary storage/stack failure.
//--------------------------------------------------------
static int check_combinational_cycles(const lemma_circuit_builder *builder,
                                      uint32_t *cycle_signal_id)
{
    size_t count = lemma_circuit_builder_signal_count(builder);
    size_t expression_count =
        lemma_circuit_builder_expression_count(builder);
    *cycle_signal_id = 0U;
    if (count > SIZE_MAX / sizeof(size_t) ||
        count > SIZE_MAX / sizeof(uint32_t) ||
        expression_count > SIZE_MAX / sizeof(uint32_t)) {
        return -1;
    }
    size_t *heads = malloc(count * sizeof(*heads));
    uint32_t *signal_stack = malloc(count * sizeof(*signal_stack));
    size_t *edge_stack = malloc(count * sizeof(*edge_stack));
    unsigned char *state = calloc(count, sizeof(*state));
    uint32_t *expression_stack = expression_count == 0U
        ? NULL
        : malloc(expression_count * sizeof(*expression_stack));
    unsigned char *expression_seen = expression_count == 0U
        ? NULL
        : calloc(expression_count, sizeof(*expression_seen));
    if (heads == NULL || signal_stack == NULL || edge_stack == NULL ||
        state == NULL ||
        (expression_count != 0U &&
         (expression_stack == NULL || expression_seen == NULL))) {
        free(heads);
        free(signal_stack);
        free(edge_stack);
        free(state);
        free(expression_stack);
        free(expression_seen);
        return -1;
    }
    for (size_t index = 0U; index < count; ++index) {
        heads[index] = SIZE_MAX;
    }
    signal_dependency_edge *edges = NULL;
    size_t edge_count = 0U;
    size_t edge_capacity = 0U;
    size_t drivers = lemma_circuit_builder_driver_count(builder);
    for (size_t index = 0U; index < drivers; ++index) {
        const lemma_ir_driver *driver = lemma_circuit_builder_driver_at(builder, index);
        size_t stack_count = 0U;
        expression_stack[stack_count++] = driver->expression_id;
        expression_seen[driver->expression_id] = 1U;
        while (stack_count != 0U) {
            uint32_t expression_id = expression_stack[--stack_count];
            const lemma_ir_expression *expression =
                lemma_circuit_builder_expression_at(builder, expression_id);
            size_t operand_count = 0U;
            if (expression->kind == LEMMA_IR_EXPRESSION_READ_SIGNAL) {
                if (!append_dependency_edge(
                        &edges, &edge_count, &edge_capacity, heads,
                        driver->target_signal_id, expression->signal_id)) {
                    free(heads);
                    free(signal_stack);
                    free(edge_stack);
                    free(state);
                    free(expression_stack);
                    free(expression_seen);
                    free(edges);
                    return -1;
                }
                continue;
            }
            (void)ir_expression_operand_count(expression->kind, &operand_count);
            for (size_t operand = 0U; operand < operand_count; ++operand) {
                uint32_t child_id = expression->operand_ids[operand];
                if (expression_seen[child_id] == 0U) {
                    if (stack_count >= expression_count) {
                        free(heads);
                        free(signal_stack);
                        free(edge_stack);
                        free(state);
                        free(expression_stack);
                        free(expression_seen);
                        free(edges);
                        return -1;
                    }
                    expression_seen[child_id] = 1U;
                    expression_stack[stack_count++] = child_id;
                }
            }
        }
        memset(expression_seen, 0, expression_count * sizeof(*expression_seen));
    }

    int result = 1;
    for (size_t start = 0U; start < count && result == 1; ++start) {
        if (state[start] != 0U) {
            continue;
        }
        size_t stack_count = 1U;
        signal_stack[0] = (uint32_t)start;
        edge_stack[0] = heads[start];
        state[start] = 1U;
        while (stack_count != 0U) {
            size_t top = stack_count - 1U;
            size_t edge_index = edge_stack[top];
            if (edge_index == SIZE_MAX) {
                state[signal_stack[top]] = 2U;
                --stack_count;
                continue;
            }
            edge_stack[top] = edges[edge_index].next_edge;
            uint32_t dependency = edges[edge_index].source_signal_id;
            if (state[dependency] == 1U) {
                *cycle_signal_id = dependency;
                result = 0;
                break;
            }
            if (state[dependency] == 0U) {
                if (stack_count >= count) {
                    result = -1;
                    break;
                }
                signal_stack[stack_count] = dependency;
                edge_stack[stack_count] = heads[dependency];
                state[dependency] = 1U;
                ++stack_count;
            }
        }
    }
    free(heads);
    free(signal_stack);
    free(edge_stack);
    free(state);
    free(expression_stack);
    free(expression_seen);
    free(edges);
    return result;
}

//--------------------------------------------------------
// Resolve syntax visitor events into the final circuit builder.
//
// @view: Borrowed syntax event valid for this callback.
// @context: Elaboration state owning the builder and diagnostic output.
// @returns: CONTINUE for accepted syntax, or FAIL after reporting rejection.
//--------------------------------------------------------
static lemma_syntax_visit_action elaborate_syntax_event(
    const lemma_syntax_view *view, void *context)
{
    elaboration_context *state = context;
    if (view->kind == LEMMA_SYNTAX_MODULE_BOUNDARY) {
        const lemma_syntax_module_boundary *boundary = &view->as.module_boundary;
        if (boundary->kind == LEMMA_SYNTAX_MODULE_BEGIN) {
            ++state->module_count;
            if (state->module_count != 1U) {
                return reject_syntax(state, "rtl.flat_design",
                                     "source must contain exactly one module",
                                     boundary->module_range, 1);
            }
            state->module_range = boundary->module_range;
            if (boundary->name.data == NULL || boundary->name.length == 0U) {
                return reject_syntax(state, "rtl.module_name",
                                     "module name is required",
                                     boundary->module_range, 1);
            }
            if (!lemma_circuit_builder_set_module_name(
                    state->builder, boundary->name.data, boundary->name.length)) {
                return reject_builder(state, "ir.module_name",
                                      "unable to store module name in circuit IR",
                                      boundary->module_range, 1);
            }
        }
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_DECLARATION_GROUP) {
        if (state->has_pending_port != 0) {
            return reject_syntax(state, "rtl.port_form",
                                 "port declaration has no name",
                                 state->pending_group_range, 1);
        }
        const lemma_syntax_declaration_group *group = &view->as.declaration_group;
        if (group->kind != LEMMA_SYNTAX_DECLARATION_GROUP_PORT) {
                return reject_syntax(state, "rtl.unsupported_construct",
                                 "parameters and internal declarations are outside the first profile",
                                 group->source.source_range, 1);
        }
        if (group->has_explicit_port_header != 0) {
            if (!parse_explicit_port_attributes(group, &state->current_port)) {
                return reject_syntax(state, "rtl.port_form",
                                     "port must have a supported input/output wire or logic type",
                                     group->source.source_range, 1);
            }
        } else if (state->current_port.valid == 0) {
            return reject_syntax(state, "rtl.port_inheritance",
                                 "bare port identifier must follow a fully specified port",
                                 group->source.source_range, 1);
        }
        state->has_pending_port = 1;
        state->pending_group_range = group->source.source_range;
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_DECLARATOR) {
        const lemma_syntax_declarator *declarator = &view->as.declarator;
        if (state->has_pending_port == 0 ||
            !ranges_equal(state->pending_group_range, declarator->group_range)) {
            return reject_syntax(state, "rtl.port_form",
                                 "port declarator is not attached to its declaration",
                                 declarator->declarator_range, 1);
        }
        state->has_pending_port = 0;
        uint32_t prior_id = 0U;
        if (!is_simple_identifier(declarator->name)) {
            return reject_syntax(state, "rtl.port_name",
                                 "only simple port identifiers are supported",
                                 declarator->declarator_range, 1);
        }
        if (lemma_circuit_builder_find_signal(
                state->builder, declarator->name.data, declarator->name.length,
                &prior_id)) {
            return reject_syntax(state, "rtl.duplicate_signal",
                                 "signal name is declared more than once",
                                 declarator->declarator_range, 1);
        }
        uint32_t signal_id = 0U;
        if (!lemma_circuit_builder_add_signal(
                state->builder, declarator->name.data, declarator->name.length,
                state->current_port.width, state->current_port.is_signed, true,
                declarator->declarator_range, &signal_id) ||
            !lemma_circuit_builder_add_port(
                state->builder, signal_id, state->current_port.direction)) {
            return reject_builder(state, "ir.out_of_memory",
                                  "unable to store a port in the circuit IR",
                                  declarator->declarator_range, 1);
        }
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_CONTINUOUS_ASSIGNMENT) {
        const lemma_syntax_continuous_assignment *assignment =
            &view->as.continuous_assignment;
        uint32_t target_id = 0U;
        if (state->has_pending_assignment != 0 ||
            !is_simple_identifier(assignment->destination_signal)) {
            return reject_syntax(state, "rtl.assignment_form",
                                 "assignment target must be a simple signal",
                                 assignment->assignment.source_range, 1);
        }
        if (!lemma_circuit_builder_find_signal(
                state->builder, assignment->destination_signal.data,
                assignment->destination_signal.length, &target_id)) {
            return reject_syntax(state, "rtl.undeclared_signal",
                                 "assignment target is outside the port list",
                                 assignment->assignment.source_range, 1);
        }
        lemma_ir_port_direction target_direction = LEMMA_IR_PORT_INPUT;
        int target_is_port = 0;
        for (size_t index = 0U;
             index < lemma_circuit_builder_port_count(state->builder); ++index) {
            const lemma_ir_port *port =
                lemma_circuit_builder_port_at(state->builder, index);
            if (port->signal_id == target_id) {
                target_direction = port->direction;
                target_is_port = 1;
                break;
            }
        }
        if (target_is_port == 0 || target_direction != LEMMA_IR_PORT_OUTPUT) {
            return reject_syntax(state, "rtl.input_driver",
                                  "continuous assignment target must be an output port",
                                  assignment->lvalue.source_range, 1);
        }
        if (count_signal_drivers(state->builder, target_id) != 0U) {
            return reject_syntax(state, "rtl.multiple_drivers",
                                 "output port has more than one driver",
                                 assignment->lvalue.source_range, 1);
        }
        state->pending_target_signal_id = target_id;
        state->pending_assignment_range = assignment->assignment.source_range;
        memset(state->expression_frames, 0, sizeof(state->expression_frames));
        state->has_pending_assignment = 1;
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_EXPRESSION) {
        const lemma_syntax_expression *expression = &view->as.expression;
        if (state->has_pending_assignment == 0 ||
            !ranges_equal(state->pending_assignment_range,
                          expression->assignment_range)) {
            return reject_syntax(state, "rtl.expression_context",
                                 "expression is not attached to a continuous assignment",
                                 expression->source.source_range, 1);
        }
        if (expression->kind == LEMMA_SYNTAX_EXPRESSION_OTHER) {
            return reject_syntax(state, "rtl.expression_form",
                                 "continuous assignment expression is unsupported",
                                 expression->source.source_range, 1);
        }
        uint32_t expression_id = 0U;
        if (expression->kind == LEMMA_SYNTAX_EXPRESSION_IDENTIFIER) {
            uint32_t signal_id = 0U;
            if (!is_simple_identifier(expression->token)) {
                return reject_syntax(state, "rtl.expression_form",
                                     "signal read must use a simple identifier",
                                     expression->source.source_range, 1);
            }
            if (!lemma_circuit_builder_find_signal(
                    state->builder, expression->token.data,
                    expression->token.length, &signal_id)) {
                return reject_syntax(state, "rtl.undeclared_signal",
                                     "expression references a signal outside the port list",
                                     expression->token.source_range, 1);
            }
            if (!lemma_circuit_builder_add_read_signal(
                    state->builder, signal_id, expression->source.source_range,
                    &expression_id)) {
                return reject_builder(state, "ir.out_of_memory",
                                      "unable to store a signal read in circuit IR",
                                      expression->source.source_range, 1);
            }
        } else {
            int lower_result =
                lower_expression_node(state, expression, &expression_id);
            if (lower_result < 0) {
                if (lower_result == -2) {
                    return LEMMA_SYNTAX_VISIT_FAIL;
                }
                return reject_builder(state, "ir.out_of_memory",
                                      "unable to store an operator in circuit IR",
                                      expression->source.source_range, 1);
            }
            if (lower_result == 0) {
                return reject_builder(state, "ir.expression_tree",
                                      "expression operator has invalid parent links",
                                      expression->source.source_range, 1);
            }
        }
        if (expression->depth != 0U) {
            if (!store_expression_child(state, expression, expression_id)) {
                return reject_builder(state, "ir.expression_tree",
                                      "expression parent/child links are invalid",
                                      expression->source.source_range, 1);
            }
            return LEMMA_SYNTAX_VISIT_CONTINUE;
        }
        if (expression->child_index != 0U ||
            !ranges_equal(expression->parent_range,
                          state->pending_assignment_range)) {
            return reject_builder(state, "ir.expression_tree",
                                  "expression root is not attached to its assignment",
                                  expression->source.source_range, 1);
        }
        const lemma_ir_signal *target = lemma_circuit_builder_signal_at(
            state->builder, state->pending_target_signal_id);
        const lemma_ir_expression *root = lemma_circuit_builder_expression_at(
            state->builder, expression_id);
        if (target->width != root->width) {
            return reject_syntax(
                state,
                root->kind == LEMMA_IR_EXPRESSION_READ_SIGNAL
                    ? "rtl.alias_width"
                    : "rtl.assignment_width",
                root->kind == LEMMA_IR_EXPRESSION_READ_SIGNAL
                    ? "alias source and output widths must match"
                    : "expression result and output widths must match",
                expression->source.source_range, 1);
        }
        if (!lemma_circuit_builder_add_driver(
                state->builder, state->pending_target_signal_id,
                expression_id, state->pending_assignment_range)) {
            return reject_builder(state, "ir.out_of_memory",
                                  "unable to store a continuous driver in circuit IR",
                                  state->pending_assignment_range, 1);
        }
        state->has_pending_assignment = 0;
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_PROCESS ||
        view->kind == LEMMA_SYNTAX_PROCESS_EVENT_EXPRESSION ||
        view->kind == LEMMA_SYNTAX_STATEMENT) {
        lemma_source_range range = view->kind == LEMMA_SYNTAX_PROCESS
            ? view->as.process.process_range
            : view->kind == LEMMA_SYNTAX_PROCESS_EVENT_EXPRESSION
                  ? view->as.process_event_expression.process_range
                  : view->as.statement.source.source_range;
        return reject_syntax(state, "rtl.unsupported_procedural_logic",
                             "procedural logic is outside the first profile",
                             range, 1);
    }
    if (view->kind == LEMMA_SYNTAX_OPAQUE_MODULE_ITEM) {
        return reject_syntax(state, "rtl.unsupported_construct",
                             "module item is outside the first profile",
                             view->as.opaque_module_item.source.source_range, 1);
    }
    return reject_syntax(state, "rtl.invalid_syntax_event",
                         "syntax extractor returned an invalid event",
                         state->module_range, 1);
}

//--------------------------------------------------------
// Check that every output has one driver and driver dependencies are acyclic.
//
// @context: Completed elaboration context and final IR builder.
// @returns: Nonzero when the circuit satisfies the first-profile driver rules.
//--------------------------------------------------------
static int validate_drivers(elaboration_context *context)
{
    size_t port_count = lemma_circuit_builder_port_count(context->builder);
    for (size_t index = 0U; index < port_count; ++index) {
        const lemma_ir_port *port =
            lemma_circuit_builder_port_at(context->builder, index);
        if (port->direction != LEMMA_IR_PORT_OUTPUT ||
            count_signal_drivers(context->builder, port->signal_id) != 0U) {
            continue;
        }
        const lemma_ir_signal *signal =
            lemma_circuit_builder_signal_at(context->builder, port->signal_id);
        set_diagnostic(context->diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.output_undriven",
                       "each output port must have one direct signal driver",
                       signal->source, 1);
        return 0;
    }
    uint32_t cycle_signal_id = 0U;
    int cycle_result = check_combinational_cycles(context->builder,
                                                  &cycle_signal_id);
    if (cycle_result == 0) {
        const lemma_ir_signal *signal =
            lemma_circuit_builder_signal_at(context->builder, cycle_signal_id);
        set_diagnostic(context->diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.combinational_cycle",
                       "combinational driver dependencies contain a cycle",
                       signal->source, 1);
        return 0;
    }
    if (cycle_result < 0) {
        set_diagnostic(context->diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                       "ir.validation_failed",
                       "unable to validate circuit expression dependencies",
                       context->module_range, 1);
        return 0;
    }
    return 1;
}

//--------------------------------------------------------
// Validate and elaborate the sole flat module into builder-owned IR storage.
//
// @parsed: Successfully parsed, live non-NULL handle borrowed for the call.
// @builder: Non-NULL fresh caller-owned builder to receive the final IR storage.
// @diagnostic: Optional diagnostic output, cleared on entry when provided.
// @returns: Nonzero on success; zero for unsupported RTL or an operation failure;
//     destroy, rather than finalize, the builder on failure.
//--------------------------------------------------------
int lemma_frontend_elaborate(const lemma_parsed_source *parsed,
                             lemma_circuit_builder *builder,
                             lemma_frontend_diagnostic *diagnostic)
{
    if (diagnostic != NULL) {
        memset(diagnostic, 0, sizeof(*diagnostic));
    }
    elaboration_context context = {0};
    context.builder = builder;
    context.diagnostic = diagnostic;
    lemma_syntax_visit_result result = lemma_frontend_visit_syntax(
        parsed, elaborate_syntax_event, &context);
    if (result != LEMMA_SYNTAX_VISIT_COMPLETE || context.failed != 0) {
        if (context.failed == 0) {
            set_diagnostic(diagnostic,
                           LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.syntax_extraction", "unable to extract module syntax",
                           context.module_range, context.module_count != 0U);
        }
        return 0;
    }
    if (context.module_count != 1U) {
        set_diagnostic(diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.flat_design", "source must contain exactly one module",
                       context.module_range, context.module_count != 0U);
        return 0;
    }
    if (lemma_circuit_builder_port_count(builder) == 0U) {
        set_diagnostic(diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_count", "module must declare at least one ANSI port",
                       context.module_range, 1);
        return 0;
    }
    if (context.has_pending_port != 0) {
        set_diagnostic(diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_form", "port declaration has no name",
                       context.pending_group_range, 1);
        return 0;
    }
    return validate_drivers(&context);
}
