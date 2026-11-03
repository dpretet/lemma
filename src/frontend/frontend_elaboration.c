#include "frontend_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Count direct named children whose syntax type matches type.
static size_t named_child_type_count(TSNode node, const char *type)
{
    size_t count = 0U;
    uint32_t child_count = ts_node_named_child_count(node);
    for (uint32_t index = 0U; index < child_count; ++index) {
        if (strcmp(ts_node_type(ts_node_named_child(node, index)), type) == 0) {
            ++count;
        }
    }
    return count;
}

// Check that a port declaration has one of the supported ANSI port shapes.
static int valid_port_syntax(TSNode declaration)
{
    // Require one supported header; parse_port checks the identifier separately.
    if (ts_node_named_child_count(declaration) != 2U) {
        return 0;
    }
    TSNode header = {0};
    uint32_t declaration_children = ts_node_named_child_count(declaration);
    for (uint32_t index = 0U; index < declaration_children; ++index) {
        TSNode child = ts_node_named_child(declaration, index);
        const char *type = ts_node_type(child);
        if (strcmp(type, "variable_port_header") == 0 ||
            strcmp(type, "net_port_header") == 0) {
            header = child;
        }
    }
    if (ts_node_is_null(header) || ts_node_named_child_count(header) != 2U ||
        named_child_type_count(header, "port_direction") != 1U) {
        return 0;
    }
    // Variable ports need an integer-vector type with an optional packed dimension.
    if (strcmp(ts_node_type(header), "variable_port_header") == 0) {
        if (named_child_type_count(header, "variable_port_type") != 1U) {
            return 0;
        }
        TSNode type = {0};
        for (uint32_t index = 0U; index < ts_node_named_child_count(header); ++index) {
            TSNode child = ts_node_named_child(header, index);
            if (strcmp(ts_node_type(child), "variable_port_type") == 0) {
                type = child;
            }
        }
        if (ts_node_named_child_count(type) != 1U ||
            named_child_type_count(type, "data_type") != 1U) {
            return 0;
        }
        TSNode data_type = ts_node_named_child(type, 0U);
        if (ts_node_named_child_count(data_type) < 1U ||
            ts_node_named_child_count(data_type) > 2U ||
            named_child_type_count(data_type, "integer_vector_type") != 1U ||
            ts_node_named_child_count(data_type) !=
                1U + named_child_type_count(data_type, "packed_dimension")) {
            return 0;
        }
        return 1;
    }

    // Net ports allow a net type with at most one supported implicit range.
    if (named_child_type_count(header, "net_port_type") != 1U) {
        return 0;
    }
    TSNode type = {0};
    for (uint32_t index = 0U; index < ts_node_named_child_count(header); ++index) {
        TSNode child = ts_node_named_child(header, index);
        if (strcmp(ts_node_type(child), "net_port_type") == 0) {
            type = child;
        }
    }
    size_t type_children = ts_node_named_child_count(type);
    size_t implicit_count = named_child_type_count(type, "data_type_or_implicit");
    if (type_children < 1U || type_children > 2U ||
        named_child_type_count(type, "net_type") != 1U || implicit_count > 1U ||
        type_children != 1U + implicit_count) {
        return 0;
    }
    if (implicit_count == 1U) {
        TSNode implicit = {0};
        for (uint32_t index = 0U; index < type_children; ++index) {
            TSNode child = ts_node_named_child(type, index);
            if (strcmp(ts_node_type(child), "data_type_or_implicit") == 0) {
                implicit = child;
            }
        }
        if (ts_node_named_child_count(implicit) != 1U) {
            return 0;
        }
        TSNode implicit_type = ts_node_named_child(implicit, 0U);
        if (strcmp(ts_node_type(implicit_type), "implicit_data_type") != 0 ||
            ts_node_named_child_count(implicit_type) > 1U ||
            (ts_node_named_child_count(implicit_type) == 1U &&
             named_child_type_count(implicit_type, "packed_dimension") != 1U)) {
            return 0;
        }
    }
    return 1;
}

// Parse a nonempty decimal source slice as uint32_t; leave value unchanged on failure.
static int parse_decimal_node(const lemma_parsed_design *parsed, TSNode node,
                              uint32_t *value)
{
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    if (start == end) {
        return 0;
    }
    uint32_t result = 0U;
    for (uint32_t index = start; index < end; ++index) {
        unsigned char character = (unsigned char)parsed->source[index];
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

// Parse one packed range with decimal bounds and return its inclusive width.
static int range_width(const lemma_parsed_design *parsed, TSNode dimension,
                       uint32_t *width)
{
    size_t range_count = 0U;
    TSNode range = frontend_find_descendant(dimension, "constant_range", &range_count);
    if (range_count != 1U) {
        return 0;
    }
    TSNode bounds[2] = {{0}};
    size_t bound_count = 0U;
    uint32_t children = ts_node_named_child_count(range);
    for (uint32_t index = 0U; index < children && bound_count < 2U; ++index) {
        TSNode child = ts_node_named_child(range, index);
        if (strcmp(ts_node_type(child), "constant_expression") == 0) {
            bounds[bound_count++] = child;
        }
    }
    if (bound_count != 2U) {
        return 0;
    }
    uint32_t first = 0U;
    uint32_t second = 0U;
    if (!parse_decimal_node(parsed, bounds[0], &first) ||
        !parse_decimal_node(parsed, bounds[1], &second)) {
        return 0;
    }
    uint32_t difference = first >= second ? first - second : second - first;
    if (difference == UINT32_MAX) {
        return 0;
    }
    *width = difference + 1U;
    return 1;
}

// Find direct simple-identifier children, succeeding only when exactly one exists.
static int find_port_identifier(TSNode declaration, TSNode *identifier)
{
    size_t count = 0U;
    uint32_t children = ts_node_child_count(declaration);
    for (uint32_t index = 0U; index < children; ++index) {
        TSNode child = ts_node_child(declaration, index);
        if (strcmp(ts_node_type(child), "simple_identifier") == 0) {
            *identifier = child;
            ++count;
        }
    }
    return count == 1U;
}

/* Resolve a supported input/output wire or logic port into signal facts.
 * A single packed decimal range is allowed; failures populate diagnostic.
 */
static int parse_port(const lemma_parsed_design *parsed, TSNode declaration,
                      lemma_resolved_signal *signal,
                      lemma_frontend_diagnostic *diagnostic)
{
    // Check declaration shape before interpreting its semantic fields.
    TSNode identifier = {0};
    size_t var_count = 0U;
    (void)frontend_find_descendant(declaration, "var", &var_count);
    if (!find_port_identifier(declaration, &identifier) ||
        !valid_port_syntax(declaration) || var_count != 0U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_form", "port must have one simple identifier", declaration);
        return 0;
    }

    // Resolve direction, dimensions, and concrete width.
    TSNode direction_node = frontend_find_descendant(declaration, "port_direction", &(size_t){0U});
    if (ts_node_is_null(direction_node)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_direction", "port direction must be explicit", declaration);
        return 0;
    }
    if (frontend_node_text_equals(parsed, direction_node, "input")) {
        signal->direction = LEMMA_IR_PORT_INPUT;
    } else if (frontend_node_text_equals(parsed, direction_node, "output")) {
        signal->direction = LEMMA_IR_PORT_OUTPUT;
    } else {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_direction", "only input and output ports are supported",
                       direction_node);
        return 0;
    }

    size_t packed_count = 0U;
    TSNode packed = frontend_find_descendant(declaration, "packed_dimension", &packed_count);
    size_t unpacked_count = 0U;
    (void)frontend_find_descendant(declaration, "unpacked_dimension", &unpacked_count);
    if (packed_count > 1U || unpacked_count != 0U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_dimensions", "only one packed dimension is supported",
                       declaration);
        return 0;
    }
    signal->width = 1U;
    if (packed_count == 1U && !range_width(parsed, packed, &signal->width)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.concrete_width", "packed range must have concrete decimal bounds",
                       packed);
        return 0;
    }
    if (signal->width == 0U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.concrete_width", "port width must be positive", declaration);
        return 0;
    }

    // Accept only unambiguous supported signedness and net/variable type.
    size_t signed_count = 0U;
    (void)frontend_find_descendant(declaration, "signed", &signed_count);
    size_t unsigned_count = 0U;
    (void)frontend_find_descendant(declaration, "unsigned", &unsigned_count);
    if (signed_count > 1U || unsigned_count > 1U ||
        (signed_count != 0U && unsigned_count != 0U)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_signedness", "port signedness declaration is invalid",
                       declaration);
        return 0;
    }
    signal->is_signed = signed_count != 0U;

    size_t logic_count = 0U;
    (void)frontend_find_descendant(declaration, "logic", &logic_count);
    size_t wire_count = 0U;
    TSNode wire = frontend_find_descendant(declaration, "net_type", &wire_count);
    if ((logic_count == 1U && wire_count == 0U) ||
        (logic_count == 0U && wire_count == 1U && frontend_node_text_equals(parsed, wire, "wire"))) {
        // These are the explicit scalar/vector types used by the initial profile.
    } else {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_type", "only wire and logic ports are supported", declaration);
        return 0;
    }

    // Preserve source offsets for later name resolution and IR construction.
    signal->name_start = ts_node_start_byte(identifier);
    signal->name_end = ts_node_end_byte(identifier);
    signal->source = frontend_span_for_node(declaration);
    return 1;
}

// Compare two source byte slices without allocating temporary strings.
static int names_equal(const lemma_parsed_design *parsed,
                       uint32_t left_start, uint32_t left_end,
                       uint32_t right_start, uint32_t right_end)
{
    uint32_t left_length = left_end - left_start;
    uint32_t right_length = right_end - right_start;
    return left_length == right_length &&
           memcmp(parsed->source + left_start, parsed->source + right_start,
                  (size_t)left_length) == 0;
}

// Look up an identifier among resolved ports and store its signal index.
static int resolve_signal(const lemma_parsed_design *parsed,
                          const lemma_elaborated_design *elaborated,
                          TSNode identifier, uint32_t *signal_id)
{
    uint32_t start = ts_node_start_byte(identifier);
    uint32_t end = ts_node_end_byte(identifier);
    for (size_t index = 0U; index < elaborated->signal_count; ++index) {
        const lemma_resolved_signal *signal = &elaborated->signals[index];
        if (names_equal(parsed, start, end, signal->name_start, signal->name_end)) {
            *signal_id = (uint32_t)index;
            return 1;
        }
    }
    return 0;
}

// Extract a simple identifier only from the direct grammar shapes used here.
static int direct_identifier(TSNode node, const char *parent_type,
                             TSNode *identifier)
{
    if (strcmp(ts_node_type(node), parent_type) != 0 ||
        ts_node_named_child_count(node) != 1U) {
        return 0;
    }
    TSNode child = ts_node_named_child(node, 0U);
    if (strcmp(ts_node_type(child), "simple_identifier") == 0) {
        *identifier = child;
        return 1;
    }
    if (strcmp(parent_type, "expression") == 0 &&
        strcmp(ts_node_type(child), "primary") == 0 &&
        ts_node_named_child_count(child) == 1U) {
        TSNode primary_child = ts_node_named_child(child, 0U);
        if (strcmp(ts_node_type(primary_child), "hierarchical_identifier") == 0 &&
            ts_node_named_child_count(primary_child) == 1U) {
            TSNode name = ts_node_named_child(primary_child, 0U);
            if (strcmp(ts_node_type(name), "simple_identifier") == 0) {
                *identifier = name;
                return 1;
            }
        }
    }
    return 0;
}

/* Parse one direct continuous signal alias and record its resolved driver.
 * Unsupported forms or invalid references fail with a diagnostic.
 */
static int parse_assignment(const lemma_parsed_design *parsed,
                            lemma_elaborated_design *elaborated,
                            TSNode continuous_assign,
                            unsigned char *has_driver,
                            lemma_frontend_diagnostic *diagnostic)
{
    // Narrow the syntax to one net assignment with a direct signal alias.
    if (ts_node_named_child_count(continuous_assign) != 1U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.assignment_form", "continuous assignment timing controls are unsupported",
                       continuous_assign);
        return 0;
    }
    TSNode assignments = {0};
    uint32_t children = ts_node_named_child_count(continuous_assign);
    for (uint32_t index = 0U; index < children; ++index) {
        TSNode child = ts_node_named_child(continuous_assign, index);
        if (strcmp(ts_node_type(child), "list_of_net_assignments") == 0) {
            assignments = child;
            break;
        }
    }
    if (ts_node_is_null(assignments) || ts_node_named_child_count(assignments) != 1U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.assignment_form", "each continuous assignment must contain one alias",
                       continuous_assign);
        return 0;
    }

    TSNode assignment = ts_node_named_child(assignments, 0U);
    if (strcmp(ts_node_type(assignment), "net_assignment") != 0 ||
        ts_node_named_child_count(assignment) != 2U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.assignment_form", "continuous assignment must be a simple signal alias",
                       assignment);
        return 0;
    }
    TSNode left = ts_node_named_child(assignment, 0U);
    TSNode right = ts_node_named_child(assignment, 1U);
    TSNode target_name = {0};
    TSNode source_name = {0};
    if (!direct_identifier(left, "net_lvalue", &target_name) ||
        !direct_identifier(right, "expression", &source_name)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.assignment_rhs", "assignment must connect simple signals directly",
                       assignment);
        return 0;
    }

    // Resolve both identifiers and validate the output-driver relationship.
    uint32_t target_id = 0U;
    uint32_t source_id = 0U;
    if (!resolve_signal(parsed, elaborated, target_name, &target_id) ||
        !resolve_signal(parsed, elaborated, source_name, &source_id)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.undeclared_signal", "assignment references a signal outside the port list",
                       assignment);
        return 0;
    }
    lemma_resolved_signal *target = &elaborated->signals[target_id];
    lemma_resolved_signal *source = &elaborated->signals[source_id];
    if (target->direction != LEMMA_IR_PORT_OUTPUT) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.input_driver", "continuous assignment target must be an output port",
                       left);
        return 0;
    }
    if (target->width != source->width) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.alias_width", "alias source and output widths must match", assignment);
        return 0;
    }
    if (has_driver[target_id] != 0U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.multiple_drivers", "output port has more than one driver", left);
        return 0;
    }
    has_driver[target_id] = 1U;

    // Record resolved IDs and source spans for the later IR lowering pass.
    lemma_resolved_driver *driver = &elaborated->drivers[elaborated->driver_count++];
    driver->target_signal_id = target_id;
    driver->source_signal_id = source_id;
    driver->source = frontend_span_for_node(assignment);
    driver->expression_source = frontend_span_for_node(source_name);
    return 1;
}

/* Detect cycles among output aliases: 1 means a cycle, 0 none, and -1 allocation
 * or size failure. Temporary traversal storage is released on every return.
 */
static int has_alias_cycle(const lemma_elaborated_design *elaborated)
{
    // Allocate state, adjacency lookup, and a bounded path for the traversal.
    if (elaborated->signal_count > SIZE_MAX / sizeof(uint32_t)) {
        return -1;
    }
    unsigned char *state = calloc(elaborated->signal_count, sizeof(*state));
    uint32_t *source_by_target = malloc(elaborated->signal_count * sizeof(*source_by_target));
    uint32_t *path = malloc(elaborated->signal_count * sizeof(*path));
    if (state == NULL || source_by_target == NULL || path == NULL) {
        free(state);
        free(source_by_target);
        free(path);
        return -1;
    }

    // Map each driven output to its source signal.
    for (size_t index = 0U; index < elaborated->signal_count; ++index) {
        source_by_target[index] = UINT32_MAX;
    }
    for (size_t index = 0U; index < elaborated->driver_count; ++index) {
        const lemma_resolved_driver *driver = &elaborated->drivers[index];
        source_by_target[driver->target_signal_id] = driver->source_signal_id;
    }

    // Follow output-to-output edges, marking complete paths for reuse.
    int has_cycle = 0;
    for (size_t start = 0U; start < elaborated->signal_count && has_cycle == 0; ++start) {
        if (elaborated->signals[start].direction != LEMMA_IR_PORT_OUTPUT || state[start] != 0U) {
            continue;
        }
        size_t path_count = 0U;
        uint32_t current = (uint32_t)start;
        while (elaborated->signals[current].direction == LEMMA_IR_PORT_OUTPUT) {
            if (state[current] == 1U) {
                has_cycle = 1;
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

    // Release temporary traversal storage for both cycle outcomes.
    free(state);
    free(source_by_target);
    free(path);
    return has_cycle;
}

/* Elaborate a live parsed design from lemma_frontend_parse for a non-NULL,
 * NUL-terminated top name. Inputs are borrowed; diagnostic may be NULL and is
 * cleared on entry. Return an owned elaboration, or NULL for invalid input,
 * unsupported RTL, or allocation failure.
 */
lemma_elaborated_design *lemma_frontend_elaborate(
    const lemma_parsed_design *parsed, const char *top_name,
    lemma_frontend_diagnostic *diagnostic)
{
    frontend_clear_diagnostic(diagnostic);
    if (parsed == NULL || top_name == NULL) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.invalid_input", "parsed design and selected top name are required",
                       (TSNode){0});
        return NULL;
    }

    // Select and identify the sole flat ANSI module for the requested top.
    TSNode root = ts_tree_root_node(parsed->tree);
    if (ts_node_named_child_count(root) != 1U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.flat_design", "source must contain exactly one flat module",
                       root);
        return NULL;
    }
    TSNode module = ts_node_named_child(root, 0U);
    if (strcmp(ts_node_type(module), "module_declaration") != 0 ||
        ts_node_named_child_count(module) < 1U) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.design_unit", "source must contain one module declaration", module);
        return NULL;
    }
    TSNode header = ts_node_named_child(module, 0U);
    if (strcmp(ts_node_type(header), "module_ansi_header") != 0) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.module_header", "module must use an ANSI port header", header);
        return NULL;
    }
    TSNode module_name = {0};
    TSNode port_list = {0};
    uint32_t header_children = ts_node_named_child_count(header);
    for (uint32_t index = 0U; index < header_children; ++index) {
        TSNode child = ts_node_named_child(header, index);
        const char *type = ts_node_type(child);
        if (strcmp(type, "simple_identifier") == 0) {
            module_name = child;
        } else if (strcmp(type, "list_of_port_declarations") == 0) {
            port_list = child;
        } else if (strcmp(type, "module_keyword") == 0) {
            continue;
        } else {
            frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.parameters", "module parameters are outside the first profile", child);
            return NULL;
        }
    }
    if (ts_node_is_null(module_name) || ts_node_is_null(port_list)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.module_header", "module name and explicit port list are required", header);
        return NULL;
    }
    uint32_t module_start = ts_node_start_byte(module_name);
    uint32_t module_end = ts_node_end_byte(module_name);
    size_t top_length = strlen(top_name);
    if ((size_t)(module_end - module_start) != top_length ||
        memcmp(parsed->source + module_start, top_name, top_length) != 0) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.top_not_found", "selected top module does not match the source module",
                       module_name);
        return NULL;
    }

    // Validate declared ports and module items, including allocation bounds.
    size_t port_count = 0U;
    uint32_t port_children = ts_node_named_child_count(port_list);
    for (uint32_t index = 0U; index < port_children; ++index) {
        TSNode child = ts_node_named_child(port_list, index);
        if (strcmp(ts_node_type(child), "ansi_port_declaration") != 0) {
            frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.port_form", "only explicit ANSI port declarations are supported", child);
            return NULL;
        }
        ++port_count;
    }
    if (port_count == 0U || port_count > SIZE_MAX / sizeof(lemma_resolved_signal)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.port_count", "module must declare a supported port list", port_list);
        return NULL;
    }

    size_t continuous_count = 0U;
    uint32_t module_children = ts_node_named_child_count(module);
    for (uint32_t index = 1U; index < module_children; ++index) {
        TSNode child = ts_node_named_child(module, index);
        if (strcmp(ts_node_type(child), "continuous_assign") != 0) {
            frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.unsupported_construct",
                           "only direct continuous assignments are supported in this profile", child);
            return NULL;
        }
        ++continuous_count;
    }
    if (continuous_count > SIZE_MAX / sizeof(lemma_resolved_driver)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.assignment_count", "too many continuous assignments", module);
        return NULL;
    }

    // Allocate temporary resolved facts; these are owned by the elaboration.
    lemma_elaborated_design *elaborated = calloc(1U, sizeof(*elaborated));
    if (elaborated == NULL) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.out_of_memory", "unable to allocate elaboration context", module);
        return NULL;
    }
    elaborated->signals = calloc(port_count, sizeof(*elaborated->signals));
    elaborated->drivers = calloc(continuous_count, sizeof(*elaborated->drivers));
    unsigned char *has_driver = calloc(port_count, sizeof(*has_driver));
    if (elaborated->signals == NULL ||
        (continuous_count != 0U && elaborated->drivers == NULL) || has_driver == NULL) {
        free(has_driver);
        lemma_frontend_elaborate_destroy(elaborated);
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                       "rtl.out_of_memory", "unable to allocate resolved signal facts", module);
        return NULL;
    }
    elaborated->signal_count = port_count;
    elaborated->module_name_start = module_start;
    elaborated->module_name_end = module_end;

    // Resolve port fields and reject duplicate names before assignments.
    for (size_t index = 0U; index < port_count; ++index) {
        TSNode declaration = ts_node_named_child(port_list, (uint32_t)index);
        if (!parse_port(parsed, declaration, &elaborated->signals[index], diagnostic)) {
            free(has_driver);
            lemma_frontend_elaborate_destroy(elaborated);
            return NULL;
        }
        for (size_t prior = 0U; prior < index; ++prior) {
            const lemma_resolved_signal *left = &elaborated->signals[index];
            const lemma_resolved_signal *right = &elaborated->signals[prior];
            if (names_equal(parsed, left->name_start, left->name_end,
                            right->name_start, right->name_end)) {
                free(has_driver);
                lemma_frontend_elaborate_destroy(elaborated);
                frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                               "rtl.duplicate_signal", "port name is declared more than once",
                               declaration);
                return NULL;
            }
        }
    }

    // Resolve aliases, then require every output to have exactly one driver.
    for (uint32_t index = 1U; index < module_children; ++index) {
        TSNode child = ts_node_named_child(module, index);
        if (!parse_assignment(parsed, elaborated, child, has_driver, diagnostic)) {
            free(has_driver);
            lemma_frontend_elaborate_destroy(elaborated);
            return NULL;
        }
    }
    for (size_t index = 0U; index < elaborated->signal_count; ++index) {
        if (elaborated->signals[index].direction == LEMMA_IR_PORT_OUTPUT &&
            has_driver[index] == 0U) {
            TSNode declaration = ts_node_named_child(port_list, (uint32_t)index);
            free(has_driver);
            lemma_frontend_elaborate_destroy(elaborated);
            frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.output_undriven", "each output must have one direct signal driver",
                           declaration);
            return NULL;
        }
    }
    free(has_driver);

    // Ensure the resolved output-alias graph is acyclic before returning it.
    int cycle_result = has_alias_cycle(elaborated);
    if (cycle_result != 0) {
        lemma_frontend_elaborate_destroy(elaborated);
        if (cycle_result < 0) {
            frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.out_of_memory", "unable to validate alias dependencies", module);
        } else {
            frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                           "rtl.combinational_cycle", "output aliases form a combinational cycle",
                           module);
        }
        return NULL;
    }
    return elaborated;
}

// Destroy the elaboration and its owned storage; NULL is accepted.
void lemma_frontend_elaborate_destroy(lemma_elaborated_design *elaborated)
{
    if (elaborated != NULL) {
        free(elaborated->signals);
        free(elaborated->drivers);
        free(elaborated);
    }
}
