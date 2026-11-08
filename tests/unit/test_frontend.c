#include "lemma/frontend.h"
#include "lemma/frontend_parse.h"
#include "lemma/frontend_syntax.h"

#include "unity.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct rejected_case {
    const char *label;
    const char *source;
} rejected_case;

typedef struct syntax_counts {
    size_t modules;
    size_t groups;
    size_t declarators;
    size_t assignments;
    size_t conditions;
    size_t processes;
    size_t event_expressions;
    size_t statements;
    size_t opaque_items;
    size_t shorthand_ports;
    size_t group_mismatches;
    lemma_ir_source_span last_group_span;
    int has_group;
} syntax_counts;

//--------------------------------------------------------
// Read a named RTL fixture into caller-owned NUL-terminated storage.
//
// @name: Fixture file name relative to tests/fixtures/rtl.
// @length: Receives the byte length on success.
// @returns: Caller-owned contents, or NULL on path, I/O, size, or allocation failure.
//--------------------------------------------------------
static char *read_fixture(const char *name, size_t *length)
{
    char path[512];
    int written = snprintf(path, sizeof(path), "%s/tests/fixtures/rtl/%s",
                           LEMMA_SOURCE_DIR, name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return NULL;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0) {
        if (file != NULL) {
            fclose(file);
        }
        return NULL;
    }
    long file_size = ftell(file);
    if (file_size < 0 || (uintmax_t)file_size > UINT32_MAX ||
        (uintmax_t)file_size >= SIZE_MAX) {
        fclose(file);
        return NULL;
    }
    rewind(file);
    size_t size = (size_t)file_size;
    char *contents = malloc(size + 1U);
    if (contents == NULL || fread(contents, 1U, size, file) != size || ferror(file)) {
        free(contents);
        fclose(file);
        return NULL;
    }
    if (fclose(file) != 0) {
        free(contents);
        return NULL;
    }
    contents[size] = '\0';
    *length = size;
    return contents;
}

//--------------------------------------------------------
// Build a circuit from a fixture whose module is the sole selected module.
//
// @name: RTL fixture file name.
// @diagnostic: Optional output populated by the frontend on failure.
// @returns: Caller-owned circuit, or NULL when loading or frontend analysis fails.
//--------------------------------------------------------
static lemma_circuit *build_fixture(const char *name,
                                    lemma_frontend_diagnostic *diagnostic)
{
    size_t length = 0U;
    char *source = read_fixture(name, &length);
    if (source == NULL) {
        return NULL;
    }
    lemma_circuit *circuit = lemma_frontend_build(source, length, diagnostic);
    free(source);
    return circuit;
}

//--------------------------------------------------------
// Compare two source spans without depending on structure padding.
//
// @left: First half-open source span.
// @right: Second half-open source span.
// @returns: Nonzero when every byte offset and source point matches.
//--------------------------------------------------------
static int spans_equal(lemma_ir_source_span left, lemma_ir_source_span right)
{
    return left.start_byte == right.start_byte && left.end_byte == right.end_byte &&
           left.start_row == right.start_row && left.start_column == right.start_column &&
           left.end_row == right.end_row && left.end_column == right.end_column;
}

//--------------------------------------------------------
// Count typed syntax events and verify declarators link to their group span.
//
// @view: Borrowed event emitted by the syntax visitor.
// @context: Mutable counters owned by the test.
// @returns: CONTINUE so the visitor examines every syntax event.
//--------------------------------------------------------
static lemma_syntax_visit_action count_syntax_event(
    const lemma_syntax_view *view, void *context)
{
    syntax_counts *counts = context;
    if (view->kind == LEMMA_SYNTAX_MODULE_BOUNDARY) {
        if (view->as.module_boundary.kind == LEMMA_SYNTAX_MODULE_BEGIN) {
            ++counts->modules;
        }
        counts->has_group = 0;
    } else if (view->kind == LEMMA_SYNTAX_DECLARATION_GROUP) {
        ++counts->groups;
        counts->last_group_span = view->as.declaration_group.source.span;
        counts->has_group = 1;
        if (view->as.declaration_group.kind == LEMMA_SYNTAX_DECLARATION_GROUP_PORT &&
            !view->as.declaration_group.has_explicit_port_header) {
            ++counts->shorthand_ports;
        }
    } else if (view->kind == LEMMA_SYNTAX_DECLARATOR) {
        ++counts->declarators;
        if (counts->has_group == 0 ||
            !spans_equal(counts->last_group_span, view->as.declarator.group_span)) {
            ++counts->group_mismatches;
        }
    } else {
        counts->has_group = 0;
        if (view->kind == LEMMA_SYNTAX_CONTINUOUS_ASSIGNMENT) {
            ++counts->assignments;
            if (view->as.continuous_assignment.condition.data != NULL) {
                ++counts->conditions;
            }
        } else if (view->kind == LEMMA_SYNTAX_PROCESS) {
            ++counts->processes;
        } else if (view->kind == LEMMA_SYNTAX_PROCESS_EVENT_EXPRESSION) {
            ++counts->event_expressions;
        } else if (view->kind == LEMMA_SYNTAX_STATEMENT) {
            ++counts->statements;
        } else if (view->kind == LEMMA_SYNTAX_OPAQUE_MODULE_ITEM) {
            ++counts->opaque_items;
        }
    }
    return LEMMA_SYNTAX_VISIT_CONTINUE;
}

//--------------------------------------------------------
// Stop extraction on the first event to verify callback stop propagation.
//
// @view: First borrowed syntax event.
// @context: Counter incremented for the callback.
// @returns: STOP to end traversal immediately.
//--------------------------------------------------------
static lemma_syntax_visit_action stop_at_first_syntax_event(
    const lemma_syntax_view *view, void *context)
{
    (void)view;
    size_t *count = context;
    ++*count;
    return LEMMA_SYNTAX_VISIT_STOP;
}

//--------------------------------------------------------
// Fail extraction on the first event to verify callback failure propagation.
//
// @view: First borrowed syntax event.
// @context: Counter incremented for the callback.
// @returns: FAIL to report a failed visit.
//--------------------------------------------------------
static lemma_syntax_visit_action fail_at_first_syntax_event(
    const lemma_syntax_view *view, void *context)
{
    (void)view;
    size_t *count = context;
    ++*count;
    return LEMMA_SYNTAX_VISIT_FAIL;
}

//--------------------------------------------------------
// Build the scalar alias fixture as a two-signal, one-driver circuit.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_builds_scalar_alias_fixture(void)
{
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_circuit *circuit = build_fixture("comb_continuous_assign_alias.sv",
                                           &diagnostic);

    TEST_ASSERT_NOT_NULL_MESSAGE(circuit, diagnostic.message);
    TEST_ASSERT_EQUAL_STRING("continuous_assign_alias", circuit->module_name);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->signal_count);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->port_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->expression_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->driver_count);
    TEST_ASSERT_EQUAL_STRING("a", circuit->signals[0].name);
    TEST_ASSERT_EQUAL_STRING("y", circuit->signals[1].name);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->expressions[0].signal_id);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->drivers[0].target_signal_id);
    lemma_circuit_destroy(circuit);
}

//--------------------------------------------------------
// Preserve vector width and signedness through the alias expression and driver.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_builds_signed_vector_alias_fixture(void)
{
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_circuit *circuit = build_fixture(
        "comb_vector_width_signedness_alias.sv", &diagnostic);

    TEST_ASSERT_NOT_NULL_MESSAGE(circuit, diagnostic.message);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signal_count);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[0].width);
    TEST_ASSERT_TRUE(circuit->signals[0].is_signed);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[1].width);
    TEST_ASSERT_FALSE(circuit->signals[1].is_signed);
    TEST_ASSERT_TRUE(circuit->signals[2].is_signed);
    TEST_ASSERT_FALSE(circuit->signals[3].is_signed);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->driver_count);
    lemma_circuit_destroy(circuit);
}

//--------------------------------------------------------
// Preserve inherited ANSI attributes and accept comments between range tokens.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_inherits_complete_ansi_port_attributes(void)
{
    static const char source[] =
        "module shorthand (input logic signed [3:0] a, b, "
        "output logic signed [3:0] y); assign y = b; endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_circuit *circuit = lemma_frontend_build(source, sizeof(source) - 1U,
                                                  &diagnostic);

    TEST_ASSERT_NOT_NULL_MESSAGE(circuit, diagnostic.message);
    TEST_ASSERT_EQUAL_UINT(3U, circuit->port_count);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[0].width);
    TEST_ASSERT_TRUE(circuit->signals[0].is_signed);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[1].width);
    TEST_ASSERT_TRUE(circuit->signals[1].is_signed);
    TEST_ASSERT_EQUAL_STRING("b", circuit->signals[1].name);
    lemma_circuit_destroy(circuit);

    static const struct {
        const char *label;
        const char *source;
    } cases[] = {
        {"block comment in range",
         "module top(input wire [3 /* comment */ : 0] a); endmodule"},
        {"line comment before next range token",
         "module top(input wire [3 // comment\n : 0] a); endmodule"}
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        lemma_frontend_diagnostic diagnostic = {0};
        lemma_circuit *circuit = lemma_frontend_build(
            cases[index].source, strlen(cases[index].source), &diagnostic);

        TEST_ASSERT_NOT_NULL_MESSAGE(circuit, cases[index].label);
        TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[0].width);
        lemma_circuit_destroy(circuit);
    }
}

//--------------------------------------------------------
// Reject partial ANSI attribute omission and RTL outside the first profile.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_rejects_unsupported_constructs(void)
{
    // Port/header cases reject partial inherited attributes and inout direction.
    static const rejected_case cases[] = {
        {"partial inherited port attributes",
         "module top(input wire a, input b, output wire y); assign y = a; endmodule"},
        {"inout port direction",
         "module top(inout wire a, output wire y); assign y=a; endmodule"},
        // An unterminated range comment must never be treated as valid trivia.
        {"unterminated packed-range block comment",
         "module top(input wire [3 /* comment : 0] a); endmodule"},
        // Parameters, internal declarations, hierarchy, and processes are outside the profile.
        {"parameter declaration",
         "module top #(parameter N=1)(input wire a, output wire y); assign y=a; endmodule"},
        {"internal net declaration",
         "module top(input wire a, output wire y); wire x; assign y=a; endmodule"},
        {"module instantiation",
         "module top(input wire a, output wire y); child u(); assign y=a; endmodule"},
        {"procedural process",
         "module top(input wire a, output logic y); always_comb y=a; endmodule"},
        // Continuous assignments must be direct and undelayed signal aliases.
        {"compound assignment expression",
         "module top(input wire a, output wire y); assign y=a & a; endmodule"},
        {"delayed continuous assignment",
         "module top(input wire a, output wire y); assign #1 y=a; endmodule"},
        // Flat-design and driver rules reject malformed circuit relationships.
        {"multiple modules",
         "module top(input wire a, output wire y); assign y=a; endmodule "
         "module other(input wire b); endmodule"},
        {"undriven output",
         "module top(input wire a, output wire y); endmodule"},
        {"driven input",
         "module top(input wire a, output wire y); assign a=y; assign y=a; endmodule"},
        {"multiple output drivers",
         "module top(input wire a, output wire y); assign y=a; assign y=a; endmodule"},
        {"width mismatch",
         "module top(input wire [3:0] a, output wire [2:0] y); assign y=a; endmodule"},
        {"combinational alias cycle",
         "module top(output wire y, output wire z); assign y=z; assign z=y; endmodule"}
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        lemma_frontend_diagnostic diagnostic = {0};
        lemma_circuit *circuit = lemma_frontend_build(
            cases[index].source, strlen(cases[index].source), &diagnostic);
        TEST_ASSERT_NULL_MESSAGE(circuit, cases[index].label);
        TEST_ASSERT_NOT_NULL(diagnostic.code);
        TEST_ASSERT_TRUE(diagnostic.has_span);
    }
}

//--------------------------------------------------------
// Extract syntax events in source order without accepting process semantics.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_visits_module_declarations_and_process_statements(void)
{
    static const char source[] =
        "module view #(parameter int N=2) (input logic [3:0] a, b, "
        "output logic [3:0] y); wire [3:0] x, z; assign y = a ? b : a; "
        "always_ff @(posedge clk) begin if (a[0]) y <= b; else y <= x; end "
        "child u(); endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(
        source, sizeof(source) - 1U, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);

    syntax_counts counts = {0};
    lemma_syntax_visit_result result = lemma_frontend_visit_syntax(
        parsed, count_syntax_event, &counts);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_COMPLETE, result);
    TEST_ASSERT_EQUAL_UINT(1U, counts.modules);
    TEST_ASSERT_EQUAL_UINT(5U, counts.groups);
    TEST_ASSERT_EQUAL_UINT(6U, counts.declarators);
    TEST_ASSERT_EQUAL_UINT(1U, counts.shorthand_ports);
    TEST_ASSERT_EQUAL_UINT(1U, counts.assignments);
    TEST_ASSERT_EQUAL_UINT(1U, counts.conditions);
    TEST_ASSERT_EQUAL_UINT(1U, counts.processes);
    TEST_ASSERT_EQUAL_UINT(1U, counts.event_expressions);
    TEST_ASSERT_TRUE(counts.statements >= 4U);
    TEST_ASSERT_EQUAL_UINT(1U, counts.opaque_items);
    TEST_ASSERT_EQUAL_UINT(0U, counts.group_mismatches);
    lemma_parsed_source_destroy(parsed);
}

//--------------------------------------------------------
// Stop or fail a syntax visit immediately when requested by its callback.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_syntax_visitor_stops_and_fails_immediately(void)
{
    static const char source[] = "module top(input wire a); endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(
        source, sizeof(source) - 1U, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);

    size_t callback_count = 0U;
    lemma_syntax_visit_result result = lemma_frontend_visit_syntax(
        parsed, stop_at_first_syntax_event, &callback_count);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_STOPPED, result);
    TEST_ASSERT_EQUAL_UINT(1U, callback_count);

    callback_count = 0U;
    result = lemma_frontend_visit_syntax(
        parsed, fail_at_first_syntax_event, &callback_count);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_FAILED, result);
    TEST_ASSERT_EQUAL_UINT(1U, callback_count);
    lemma_parsed_source_destroy(parsed);
}
