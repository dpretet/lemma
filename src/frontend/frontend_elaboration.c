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

typedef struct elaboration_context {
    lemma_circuit_builder *builder;
    lemma_frontend_diagnostic *diagnostic;
    lemma_ir_source_span module_span;
    lemma_ir_source_span pending_group_span;
    port_attributes current_port;
    size_t module_count;
    int has_pending_port;
    int failed;
} elaboration_context;

//--------------------------------------------------------
// Compare two source spans field by field without relying on structure padding.
//
// @left: First half-open source span.
// @right: Second half-open source span.
// @returns: Nonzero when every byte offset and point field matches.
//--------------------------------------------------------
static int spans_equal(lemma_ir_source_span left, lemma_ir_source_span right)
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
// @span: Source span attached when has_span is nonzero.
// @has_span: Whether a source span is available.
// @returns: None.
//--------------------------------------------------------
static void set_diagnostic(lemma_frontend_diagnostic *diagnostic,
                           lemma_frontend_stage stage, const char *code,
                           const char *message, lemma_ir_source_span span,
                           int has_span)
{
    if (diagnostic == NULL) {
        return;
    }
    memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->stage = stage;
    diagnostic->code = code;
    (void)snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
    if (has_span != 0) {
        diagnostic->has_span = 1;
        diagnostic->span = span;
    }
}

//--------------------------------------------------------
// Report an RTL validation/elaboration failure to the visitor caller.
//
// @context: Active elaboration state receiving the diagnostic.
// @code: Static diagnostic code string.
// @message: Concise diagnostic message.
// @span: Source span attached when available.
// @has_span: Whether a source span is available.
// @returns: FAIL so syntax visitation stops immediately.
//--------------------------------------------------------
static lemma_syntax_visit_action reject_syntax(
    elaboration_context *context, const char *code, const char *message,
    lemma_ir_source_span span, int has_span)
{
    context->failed = 1;
    set_diagnostic(context->diagnostic,
                   LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                   code, message, span, has_span);
    return LEMMA_SYNTAX_VISIT_FAIL;
}

//--------------------------------------------------------
// Report a circuit-builder failure and stop syntax elaboration.
//
// @context: Active elaboration state receiving the diagnostic.
// @code: Static circuit-IR diagnostic code.
// @message: Concise diagnostic message.
// @span: Source span attached when available.
// @has_span: Whether a source span is available.
// @returns: FAIL so syntax visitation stops immediately.
//--------------------------------------------------------
static lemma_syntax_visit_action reject_builder(
    elaboration_context *context, const char *code, const char *message,
    lemma_ir_source_span span, int has_span)
{
    context->failed = 1;
    set_diagnostic(context->diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                   code, message, span, has_span);
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
// Check whether a source slice contains a whole token.
//
// @text: Borrowed source slice to inspect.
// @token: NUL-terminated token to find.
// @returns: Nonzero when token appears at identifier boundaries.
//--------------------------------------------------------
static int text_has_token(lemma_syntax_text text, const char *token)
{
    size_t token_length = strlen(token);
    if (text.data == NULL || token_length == 0U || token_length > text.length) {
        return 0;
    }
    for (size_t start = 0U; start <= text.length - token_length; ++start) {
        if (memcmp(text.data + start, token, token_length) != 0) {
            continue;
        }
        int left_boundary = start == 0U ||
            !identifier_character((unsigned char)text.data[start - 1U], 0);
        size_t end = start + token_length;
        int right_boundary = end == text.length ||
            !identifier_character((unsigned char)text.data[end], 0);
        if (left_boundary && right_boundary) {
            return 1;
        }
    }
    return 0;
}

//--------------------------------------------------------
// Skip whitespace and comments in a packed range slice.
//
// @text: Borrowed source slice containing the range.
// @index: Required current byte index, advanced over trivia on success.
// @returns: Nonzero when trivia ends cleanly; zero for an invalid slice/index or
//     an unterminated block comment.
//--------------------------------------------------------
static int skip_range_trivia(lemma_syntax_text text, size_t *index)
{
    if (text.data == NULL || index == NULL || *index > text.length) {
        return 0;
    }
    for (;;) {
        while (*index < text.length &&
               (text.data[*index] == ' ' || text.data[*index] == '\t' ||
                text.data[*index] == '\r' || text.data[*index] == '\n')) {
            ++*index;
        }
        if (text.length - *index < 2U || text.data[*index] != '/') {
            return 1;
        }
        if (text.data[*index + 1U] == '/') {
            *index += 2U;
            while (*index < text.length && text.data[*index] != '\r' &&
                   text.data[*index] != '\n') {
                ++*index;
            }
            continue;
        }
        if (text.data[*index + 1U] != '*') {
            return 1;
        }

        *index += 2U;
        int closed = 0;
        while (*index < text.length) {
            if (text.length - *index >= 2U && text.data[*index] == '*' &&
                text.data[*index + 1U] == '/') {
                *index += 2U;
                closed = 1;
                break;
            }
            ++*index;
        }
        if (closed == 0) {
            return 0;
        }
    }
}

//--------------------------------------------------------
// Parse an unsigned decimal bound from a packed range slice.
//
// @text: Borrowed range text.
// @index: Current offset, advanced past the decimal digits on success.
// @value: Required output for the parsed bound.
// @returns: Nonzero for a nonempty, in-range decimal number.
//--------------------------------------------------------
static int parse_decimal_bound(lemma_syntax_text text, size_t *index,
                               uint32_t *value)
{
    uint32_t result = 0U;
    size_t start = *index;
    while (*index < text.length) {
        unsigned char character = (unsigned char)text.data[*index];
        if (character < (unsigned char)'0' || character > (unsigned char)'9') {
            break;
        }
        uint32_t digit = (uint32_t)(character - (unsigned char)'0');
        if (result > (UINT32_MAX - digit) / 10U) {
            return 0;
        }
        result = result * 10U + digit;
        ++*index;
    }
    if (*index == start) {
        return 0;
    }
    *value = result;
    return 1;
}

//--------------------------------------------------------
// Resolve the first profile's single packed decimal range to a positive width.
//
// @range: Packed-dimension source slice, or empty for a scalar.
// @width: Required output for the resolved bit width.
// @returns: Nonzero for a scalar or one valid decimal range.
//--------------------------------------------------------
static int packed_width(lemma_syntax_text range, uint32_t *width)
{
    if (width == NULL) {
        return 0;
    }
    if (range.length == 0U) {
        *width = 1U;
        return 1;
    }
    if (range.data == NULL) {
        return 0;
    }
    size_t index = 0U;
    if (!skip_range_trivia(range, &index)) {
        return 0;
    }
    if (index >= range.length || range.data[index++] != '[') {
        return 0;
    }
    if (!skip_range_trivia(range, &index)) {
        return 0;
    }
    uint32_t first = 0U;
    if (!parse_decimal_bound(range, &index, &first)) {
        return 0;
    }
    if (!skip_range_trivia(range, &index)) {
        return 0;
    }
    if (index >= range.length || range.data[index++] != ':') {
        return 0;
    }
    if (!skip_range_trivia(range, &index)) {
        return 0;
    }
    uint32_t last = 0U;
    if (!parse_decimal_bound(range, &index, &last)) {
        return 0;
    }
    if (!skip_range_trivia(range, &index)) {
        return 0;
    }
    if (index >= range.length || range.data[index++] != ']') {
        return 0;
    }
    if (!skip_range_trivia(range, &index)) {
        return 0;
    }
    if (index != range.length) {
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
    if (text_has_token(group->source, "var")) {
        return 0;
    }
    if (text_equals(group->direction, "input")) {
        attributes->direction = LEMMA_IR_PORT_INPUT;
    } else if (text_equals(group->direction, "output")) {
        attributes->direction = LEMMA_IR_PORT_OUTPUT;
    } else {
        return 0;
    }
    int has_wire = text_has_token(group->type, "wire");
    int has_logic = text_has_token(group->type, "logic");
    if (has_wire == has_logic || text_has_token(group->type, "reg")) {
        return 0;
    }
    int has_signed = text_has_token(group->type, "signed");
    int has_unsigned = text_has_token(group->type, "unsigned");
    if (has_signed != 0 && has_unsigned != 0) {
        return 0;
    }
    attributes->is_signed = has_signed != 0;
    if (!packed_width(group->packed_dimensions, &attributes->width)) {
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
        if (driver != NULL && driver->target_signal_id == signal_id) {
            ++count;
        }
    }
    return count;
}

//--------------------------------------------------------
// Detect cycles in the final direct-alias graph using temporary color state.
//
// @builder: Builder whose signal, expression, and driver arrays are inspected.
// @cycle_signal_id: Optional output receiving a signal in the detected cycle.
// @returns: 1 for acyclic, 0 for a cycle, or -1 for invalid data/allocation failure.
//--------------------------------------------------------
static int check_alias_cycles(const lemma_circuit_builder *builder,
                              uint32_t *cycle_signal_id)
{
    size_t count = lemma_circuit_builder_signal_count(builder);
    if (cycle_signal_id != NULL) {
        *cycle_signal_id = 0U;
    }
    if (count == 0U) {
        return 1;
    }
    if (count > SIZE_MAX / sizeof(uint32_t)) {
        return -1;
    }
    uint32_t *source_by_target = malloc(count * sizeof(*source_by_target));
    uint32_t *path = malloc(count * sizeof(*path));
    unsigned char *state = calloc(count, sizeof(*state));
    if (source_by_target == NULL || path == NULL || state == NULL) {
        free(source_by_target);
        free(path);
        free(state);
        return -1;
    }
    for (size_t index = 0U; index < count; ++index) {
        source_by_target[index] = UINT32_MAX;
    }
    size_t drivers = lemma_circuit_builder_driver_count(builder);
    for (size_t index = 0U; index < drivers; ++index) {
        const lemma_ir_driver *driver = lemma_circuit_builder_driver_at(builder, index);
        const lemma_ir_expression *expression = driver == NULL
            ? NULL
            : lemma_circuit_builder_expression_at(builder, driver->expression_id);
        if (driver == NULL || expression == NULL ||
            driver->target_signal_id >= count || expression->signal_id >= count) {
            free(source_by_target);
            free(path);
            free(state);
            return -1;
        }
        source_by_target[driver->target_signal_id] = expression->signal_id;
    }

    int result = 1;
    for (size_t start = 0U; start < count && result == 1; ++start) {
        size_t path_count = 0U;
        uint32_t current = (uint32_t)start;
        while (current != UINT32_MAX) {
            if (state[current] == 1U) {
                if (cycle_signal_id != NULL) {
                    *cycle_signal_id = current;
                }
                result = 0;
                break;
            }
            if (state[current] == 2U) {
                break;
            }
            state[current] = 1U;
            path[path_count++] = current;
            current = source_by_target[current];
        }
        for (size_t index = 0U; index < path_count; ++index) {
            state[path[index]] = 2U;
        }
    }
    free(source_by_target);
    free(path);
    free(state);
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
                                     boundary->module_span, 1);
            }
            state->module_span = boundary->module_span;
            if (boundary->name.data == NULL || boundary->name.length == 0U) {
                return reject_syntax(state, "rtl.module_name",
                                     "module name is required",
                                     boundary->module_span, 1);
            }
            if (!lemma_circuit_builder_set_module_name(
                    state->builder, boundary->name.data, boundary->name.length)) {
                return reject_builder(state, "ir.module_name",
                                      "unable to store module name in circuit IR",
                                      boundary->module_span, 1);
            }
        }
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_DECLARATION_GROUP) {
        if (state->has_pending_port != 0) {
            return reject_syntax(state, "rtl.port_form",
                                 "port declaration has no name",
                                 state->pending_group_span, 1);
        }
        const lemma_syntax_declaration_group *group = &view->as.declaration_group;
        if (group->kind != LEMMA_SYNTAX_DECLARATION_GROUP_PORT) {
                return reject_syntax(state, "rtl.unsupported_construct",
                                 "parameters and internal declarations are outside the first profile",
                                 group->source.span, 1);
        }
        if (group->has_explicit_port_header != 0) {
            if (!parse_explicit_port_attributes(group, &state->current_port)) {
                return reject_syntax(state, "rtl.port_form",
                                     "port must have a supported input/output wire or logic type",
                                     group->source.span, 1);
            }
        } else if (state->current_port.valid == 0) {
            return reject_syntax(state, "rtl.port_inheritance",
                                 "bare port identifier must follow a fully specified port",
                                 group->source.span, 1);
        }
        state->has_pending_port = 1;
        state->pending_group_span = group->source.span;
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_DECLARATOR) {
        const lemma_syntax_declarator *declarator = &view->as.declarator;
        if (state->has_pending_port == 0 ||
            !spans_equal(state->pending_group_span, declarator->group_span)) {
            return reject_syntax(state, "rtl.port_form",
                                 "port declarator is not attached to its declaration",
                                 declarator->declarator_span, 1);
        }
        state->has_pending_port = 0;
        uint32_t prior_id = 0U;
        if (!is_simple_identifier(declarator->name)) {
            return reject_syntax(state, "rtl.port_name",
                                 "only simple port identifiers are supported",
                                 declarator->declarator_span, 1);
        }
        if (lemma_circuit_builder_find_signal(
                state->builder, declarator->name.data, declarator->name.length,
                &prior_id)) {
            return reject_syntax(state, "rtl.duplicate_signal",
                                 "signal name is declared more than once",
                                 declarator->declarator_span, 1);
        }
        uint32_t signal_id = 0U;
        if (!lemma_circuit_builder_add_signal(
                state->builder, declarator->name.data, declarator->name.length,
                state->current_port.width, state->current_port.is_signed, true,
                declarator->declarator_span, &signal_id) ||
            !lemma_circuit_builder_add_port(
                state->builder, signal_id, state->current_port.direction)) {
            return reject_builder(state, "ir.out_of_memory",
                                  "unable to store a port in the circuit IR",
                                  declarator->declarator_span, 1);
        }
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_CONTINUOUS_ASSIGNMENT) {
        const lemma_syntax_continuous_assignment *assignment =
            &view->as.continuous_assignment;
        uint32_t target_id = 0U;
        uint32_t source_id = 0U;
        if (!is_simple_identifier(assignment->destination_signal) ||
            !is_simple_identifier(assignment->expression)) {
            return reject_syntax(state, "rtl.assignment_form",
                                 "assignment must connect simple signals directly",
                                 assignment->assignment.span, 1);
        }
        if (!lemma_circuit_builder_find_signal(
                state->builder, assignment->destination_signal.data,
                assignment->destination_signal.length,
                &target_id) ||
            !lemma_circuit_builder_find_signal(
                state->builder, assignment->expression.data,
                assignment->expression.length, &source_id)) {
            return reject_syntax(state, "rtl.undeclared_signal",
                                 "assignment references a signal outside the port list",
                                 assignment->assignment.span, 1);
        }
        const lemma_ir_signal *target =
            lemma_circuit_builder_signal_at(state->builder, target_id);
        const lemma_ir_signal *source =
            lemma_circuit_builder_signal_at(state->builder, source_id);
        if (target == NULL || source == NULL) {
            return reject_syntax(state, "ir.invalid_signal",
                                 "assignment references an invalid signal ID",
                                 assignment->assignment.span, 1);
        }
        lemma_ir_port_direction target_direction = LEMMA_IR_PORT_INPUT;
        int target_is_port = 0;
        for (size_t index = 0U;
             index < lemma_circuit_builder_port_count(state->builder); ++index) {
            const lemma_ir_port *port =
                lemma_circuit_builder_port_at(state->builder, index);
            if (port != NULL && port->signal_id == target_id) {
                target_direction = port->direction;
                target_is_port = 1;
                break;
            }
        }
        if (target_is_port == 0 || target_direction != LEMMA_IR_PORT_OUTPUT) {
            return reject_syntax(state, "rtl.input_driver",
                                 "continuous assignment target must be an output port",
                                 assignment->lvalue.span, 1);
        }
        if (target->width != source->width) {
            return reject_syntax(state, "rtl.alias_width",
                                 "alias source and output widths must match",
                                 assignment->assignment.span, 1);
        }
        if (count_signal_drivers(state->builder, target_id) != 0U) {
            return reject_syntax(state, "rtl.multiple_drivers",
                                 "output port has more than one driver",
                                 assignment->lvalue.span, 1);
        }
        uint32_t expression_id = 0U;
        if (!lemma_circuit_builder_add_read_signal(
                state->builder, source_id, assignment->expression.span,
                &expression_id) ||
            !lemma_circuit_builder_add_driver(
                state->builder, target_id, expression_id, assignment->assignment.span)) {
            return reject_builder(state, "ir.out_of_memory",
                                  "unable to store a continuous driver in the circuit IR",
                                  assignment->assignment.span, 1);
        }
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    if (view->kind == LEMMA_SYNTAX_PROCESS ||
        view->kind == LEMMA_SYNTAX_PROCESS_EVENT_EXPRESSION ||
        view->kind == LEMMA_SYNTAX_STATEMENT) {
        lemma_ir_source_span span = view->kind == LEMMA_SYNTAX_PROCESS
            ? view->as.process.process_span
            : view->kind == LEMMA_SYNTAX_PROCESS_EVENT_EXPRESSION
                  ? view->as.process_event_expression.process_span
                  : view->as.statement.source.span;
        return reject_syntax(state, "rtl.unsupported_procedural_logic",
                             "procedural logic is outside the first profile",
                             span, 1);
    }
    if (view->kind == LEMMA_SYNTAX_OPAQUE_MODULE_ITEM) {
        return reject_syntax(state, "rtl.unsupported_construct",
                             "module item is outside the first profile",
                             view->as.opaque_module_item.source.span, 1);
    }
    return reject_syntax(state, "rtl.invalid_syntax_event",
                         "syntax extractor returned an invalid event",
                         state->module_span, 1);
}

//--------------------------------------------------------
// Check that every output has one driver and aliases are acyclic.
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
        if (port == NULL) {
            set_diagnostic(context->diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                           "ir.invalid_port", "circuit contains an invalid port record",
                           context->module_span, 1);
            return 0;
        }
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
                       signal != NULL ? signal->source : context->module_span,
                       signal != NULL);
        return 0;
    }
    uint32_t cycle_signal_id = 0U;
    int cycle_result = check_alias_cycles(context->builder, &cycle_signal_id);
    if (cycle_result == 0) {
        const lemma_ir_signal *signal =
            lemma_circuit_builder_signal_at(context->builder, cycle_signal_id);
        set_diagnostic(context->diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.combinational_cycle",
                       "output aliases form a combinational cycle",
                       signal != NULL ? signal->source : context->module_span,
                       signal != NULL);
        return 0;
    }
    if (cycle_result < 0) {
        set_diagnostic(context->diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                       "ir.validation_failed",
                       "unable to validate circuit alias dependencies",
                       context->module_span, 1);
        return 0;
    }
    return 1;
}

//--------------------------------------------------------
// Validate and elaborate the sole flat module into builder-owned IR storage.
//
// @parsed: Live parsed-source handle borrowed for the call.
// @builder: Fresh caller-owned builder to receive the final IR storage.
// @diagnostic: Optional diagnostic output, cleared on entry when provided.
// @returns: Nonzero on success; zero for invalid input, unsupported RTL, or
//     allocation failure; destroy, rather than finalize, the builder on failure.
//--------------------------------------------------------
int lemma_frontend_elaborate(const lemma_parsed_source *parsed,
                             lemma_circuit_builder *builder,
                             lemma_frontend_diagnostic *diagnostic)
{
    if (diagnostic != NULL) {
        memset(diagnostic, 0, sizeof(*diagnostic));
    }
    if (parsed == NULL || builder == NULL) {
        set_diagnostic(diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.invalid_input", "parsed source and builder are required",
                       (lemma_ir_source_span){0}, 0);
        return 0;
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
                           context.module_span, context.module_count != 0U);
        }
        return 0;
    }
    if (context.module_count != 1U) {
        set_diagnostic(diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.flat_design", "source must contain exactly one module",
                       context.module_span, context.module_count != 0U);
        return 0;
    }
    if (lemma_circuit_builder_port_count(builder) == 0U) {
        set_diagnostic(diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_count", "module must declare at least one ANSI port",
                       context.module_span, 1);
        return 0;
    }
    if (context.has_pending_port != 0) {
        set_diagnostic(diagnostic,
                       LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_form", "port declaration has no name",
                       context.pending_group_span, 1);
        return 0;
    }
    return validate_drivers(&context);
}
