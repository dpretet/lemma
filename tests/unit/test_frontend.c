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

typedef struct expected_operator {
    lemma_ir_expression_kind kind;
    uint32_t expression_id;
    uint32_t left_expression_id;
    uint32_t right_expression_id;
    uint32_t left_signal_id;
    uint32_t right_signal_id;
    uint32_t target_signal_id;
    uint32_t result_width;
    const char *expression_text;
    const char *assignment_text;
} expected_operator;

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
    lemma_source_range last_group_range;
    int has_group;
} syntax_counts;

typedef struct declaration_facts_capture {
    lemma_syntax_builtin_type_kind builtin_types[3];
    lemma_syntax_signedness_kind signedness[3];
    size_t dimension_counts[3];
    size_t groups;
    int has_var[3];
    int first_bounds_match;
    int first_raw_dimensions_match;
} declaration_facts_capture;

typedef struct expression_event_capture {
    lemma_syntax_expression_kind kinds[8];
    lemma_source_range sources[8];
    lemma_source_range tokens[8];
    lemma_source_range parents[8];
    size_t depths[8];
    size_t child_indexes[8];
    size_t operand_counts[8];
    size_t count;
} expression_event_capture;

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
// Build a result from a fixture whose module is the sole selected module.
//
// @name: RTL fixture file name.
// @returns: Caller-owned frontend result, or NULL when loading or result allocation fails.
//--------------------------------------------------------
static lemma_frontend_result *build_fixture(const char *name)
{
    size_t length = 0U;
    char *source = read_fixture(name, &length);
    if (source == NULL) {
        return NULL;
    }
    lemma_frontend_result *result = lemma_frontend_build(source, length);
    free(source);
    return result;
}

//--------------------------------------------------------
// Compare two source ranges without depending on structure padding.
//
// @left: First half-open source range.
// @right: Second half-open source range.
// @returns: Nonzero when every byte offset and source point matches.
//--------------------------------------------------------
static int ranges_equal(lemma_source_range left, lemma_source_range right)
{
    return left.start_byte == right.start_byte && left.end_byte == right.end_byte &&
           left.start_row == right.start_row && left.start_column == right.start_column &&
           left.end_row == right.end_row && left.end_column == right.end_column;
}

//--------------------------------------------------------
// Compare one borrowed syntax slice with expected text during its callback.
//
// @text: Borrowed source slice to compare.
// @expected: NUL-terminated expected spelling.
// @returns: Nonzero when the source slice exactly matches expected.
//--------------------------------------------------------
static int syntax_text_equals(lemma_syntax_text text, const char *expected)
{
    size_t length = strlen(expected);
    return text.data != NULL && text.length == length &&
           memcmp(text.data, expected, length) == 0;
}

//--------------------------------------------------------
// Check that a source range identifies the expected bytes in a test buffer.
//
// @range: Half-open source range to validate.
// @source: NUL-terminated source buffer containing the range.
// @expected: Expected source bytes.
// @returns: Nonzero when offsets and bytes exactly match the expectation.
//--------------------------------------------------------
static int range_matches_text(lemma_source_range range, const char *source,
                              const char *expected)
{
    size_t expected_length = strlen(expected);
    return source != NULL && range.start_byte <= range.end_byte &&
           range.end_byte <= strlen(source) &&
           range.end_byte - range.start_byte == expected_length &&
           memcmp(source + range.start_byte, expected, expected_length) == 0;
}

//--------------------------------------------------------
// Capture expression syntax metadata without retaining borrowed text slices.
//
// @view: Borrowed syntax event emitted by extraction.
// @context: Bounded expression-event capture owned by the test.
// @returns: CONTINUE to collect every expression node, or FAIL on overflow.
//--------------------------------------------------------
static lemma_syntax_visit_action capture_expression_events(
    const lemma_syntax_view *view, void *context)
{
    if (view->kind != LEMMA_SYNTAX_EXPRESSION) {
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    expression_event_capture *capture = context;
    if (capture->count >= sizeof(capture->kinds) / sizeof(capture->kinds[0])) {
        return LEMMA_SYNTAX_VISIT_FAIL;
    }
    size_t index = capture->count++;
    const lemma_syntax_expression *expression = &view->as.expression;
    capture->kinds[index] = expression->kind;
    capture->sources[index] = expression->source.source_range;
    capture->tokens[index] = expression->token.source_range;
    capture->parents[index] = expression->parent_range;
    capture->depths[index] = expression->depth;
    capture->child_indexes[index] = expression->child_index;
    capture->operand_counts[index] = expression->operand_count;
    return LEMMA_SYNTAX_VISIT_CONTINUE;
}

//--------------------------------------------------------
// Count typed syntax events and verify declarators link to their group range.
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
        counts->last_group_range = view->as.declaration_group.source.source_range;
        counts->has_group = 1;
        if (view->as.declaration_group.kind == LEMMA_SYNTAX_DECLARATION_GROUP_PORT &&
            !view->as.declaration_group.has_explicit_port_header) {
            ++counts->shorthand_ports;
        }
    } else if (view->kind == LEMMA_SYNTAX_DECLARATOR) {
        ++counts->declarators;
        if (counts->has_group == 0 ||
            !ranges_equal(counts->last_group_range, view->as.declarator.group_range)) {
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
// Capture typed declaration facts without retaining callback-borrowed slices.
//
// @view: Borrowed syntax event emitted by extraction.
// @context: Three-group fact capture owned by the test.
// @returns: CONTINUE so all declaration groups are inspected.
//--------------------------------------------------------
static lemma_syntax_visit_action capture_declaration_facts(
    const lemma_syntax_view *view, void *context)
{
    declaration_facts_capture *capture = context;
    if (view->kind != LEMMA_SYNTAX_DECLARATION_GROUP || capture->groups >= 3U) {
        return LEMMA_SYNTAX_VISIT_CONTINUE;
    }
    const lemma_syntax_declaration_group *group = &view->as.declaration_group;
    size_t index = capture->groups++;
    capture->builtin_types[index] = group->builtin_type;
    capture->signedness[index] = group->signedness;
    capture->dimension_counts[index] = group->packed_dimension_count;
    capture->has_var[index] = group->has_var_keyword;
    if (index == 0U) {
        capture->first_bounds_match =
            syntax_text_equals(group->packed_left_bound, "3") &&
            syntax_text_equals(group->packed_right_bound, "0");
        capture->first_raw_dimensions_match =
            syntax_text_equals(group->packed_dimensions, "[3:0]");
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
    lemma_frontend_result *result =
        build_fixture("comb_continuous_assign_alias.sv");

    TEST_ASSERT_NOT_NULL(result);
    lemma_circuit *circuit = result->circuit;
    TEST_ASSERT_NOT_NULL(circuit);
    TEST_ASSERT_EQUAL_UINT(0U, result->diagnostic_count);
    TEST_ASSERT_NULL(result->diagnostics);
    TEST_ASSERT_EQUAL_STRING("continuous_assign_alias", circuit->module_name);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->signal_count);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->port_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->expression_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->driver_count);
    TEST_ASSERT_EQUAL_STRING("a", circuit->signals[0].name);
    TEST_ASSERT_EQUAL_STRING("y", circuit->signals[1].name);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->expressions[0].signal_id);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->drivers[0].target_signal_id);
    lemma_frontend_result_destroy(result);
}

//--------------------------------------------------------
// Expose the boolean fixture as a source-ordered expression tree with parent links.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_exposes_boolean_expression_syntax_tree(void)
{
    size_t source_length = 0U;
    char *source = read_fixture("comb_continuous_assign_boolean.sv",
                                &source_length);
    TEST_ASSERT_NOT_NULL(source);
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed =
        lemma_frontend_parse(source, source_length, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);

    expression_event_capture capture = {0};
    lemma_syntax_visit_result result = lemma_frontend_visit_syntax(
        parsed, capture_expression_events, &capture);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_COMPLETE, result);
    TEST_ASSERT_EQUAL_UINT(7U, capture.count);

    const char *root_text = strstr(source, "(a && b) || !c");
    const char *assignment_text = strstr(source, "y = (a && b) || !c");
    TEST_ASSERT_NOT_NULL(root_text);
    TEST_ASSERT_NOT_NULL(assignment_text);
    size_t root_offset = (size_t)(root_text - source);
    size_t assignment_offset = (size_t)(assignment_text - source);
    static const lemma_syntax_expression_kind expected_kinds[] = {
        LEMMA_SYNTAX_EXPRESSION_IDENTIFIER,
        LEMMA_SYNTAX_EXPRESSION_IDENTIFIER,
        LEMMA_SYNTAX_EXPRESSION_LOGICAL_AND,
        LEMMA_SYNTAX_EXPRESSION_PARENTHESIZED,
        LEMMA_SYNTAX_EXPRESSION_IDENTIFIER,
        LEMMA_SYNTAX_EXPRESSION_LOGICAL_NOT,
        LEMMA_SYNTAX_EXPRESSION_LOGICAL_OR
    };
    static const char *const expected_sources[] = {
        "a", "b", "a && b", "(a && b)", "c", "!c", "(a && b) || !c"
    };
    for (size_t index = 0U; index < capture.count; ++index) {
        TEST_ASSERT_EQUAL(expected_kinds[index], capture.kinds[index]);
        TEST_ASSERT_TRUE(range_matches_text(
            capture.sources[index], source, expected_sources[index]));
    }
    static const size_t expected_operand_counts[] = {0U, 0U, 2U, 1U, 0U, 1U, 2U};
    for (size_t index = 0U; index < capture.count; ++index) {
        TEST_ASSERT_EQUAL_UINT(expected_operand_counts[index],
                              capture.operand_counts[index]);
    }
    TEST_ASSERT_EQUAL_UINT(root_offset + 3U, capture.tokens[2].start_byte);
    TEST_ASSERT_EQUAL_UINT(root_offset + 5U, capture.tokens[2].end_byte);
    TEST_ASSERT_EQUAL_UINT(root_offset + 12U, capture.tokens[5].start_byte);
    TEST_ASSERT_EQUAL_UINT(root_offset + 13U, capture.tokens[5].end_byte);
    TEST_ASSERT_EQUAL_UINT(root_offset + 9U, capture.tokens[6].start_byte);
    TEST_ASSERT_EQUAL_UINT(root_offset + 11U, capture.tokens[6].end_byte);

    TEST_ASSERT_EQUAL_UINT(3U, capture.depths[0]);
    TEST_ASSERT_EQUAL_UINT(0U, capture.child_indexes[0]);
    TEST_ASSERT_TRUE(range_matches_text(capture.parents[0], source, "a && b"));
    TEST_ASSERT_EQUAL_UINT(3U, capture.depths[1]);
    TEST_ASSERT_EQUAL_UINT(1U, capture.child_indexes[1]);
    TEST_ASSERT_TRUE(range_matches_text(capture.parents[1], source, "a && b"));
    TEST_ASSERT_EQUAL_UINT(2U, capture.depths[2]);
    TEST_ASSERT_EQUAL_UINT(0U, capture.child_indexes[2]);
    TEST_ASSERT_TRUE(range_matches_text(capture.parents[2], source, "(a && b)"));
    TEST_ASSERT_EQUAL_UINT(1U, capture.depths[3]);
    TEST_ASSERT_EQUAL_UINT(0U, capture.child_indexes[3]);
    TEST_ASSERT_TRUE(range_matches_text(
        capture.parents[3], source, "(a && b) || !c"));
    TEST_ASSERT_EQUAL_UINT(2U, capture.depths[4]);
    TEST_ASSERT_EQUAL_UINT(0U, capture.child_indexes[4]);
    TEST_ASSERT_TRUE(range_matches_text(capture.parents[4], source, "!c"));
    TEST_ASSERT_EQUAL_UINT(1U, capture.depths[5]);
    TEST_ASSERT_EQUAL_UINT(1U, capture.child_indexes[5]);
    TEST_ASSERT_TRUE(range_matches_text(
        capture.parents[5], source, "(a && b) || !c"));
    TEST_ASSERT_EQUAL_UINT(0U, capture.depths[6]);
    TEST_ASSERT_EQUAL_UINT(0U, capture.child_indexes[6]);
    TEST_ASSERT_EQUAL_UINT(assignment_offset, capture.parents[6].start_byte);
    TEST_ASSERT_EQUAL_UINT(assignment_offset + strlen("y = (a && b) || !c"),
                          capture.parents[6].end_byte);

    lemma_parsed_source_destroy(parsed);
    free(source);
}

//--------------------------------------------------------
// Lower the boolean fixture to typed logical operators and its output driver.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_lowers_boolean_expression_fixture(void)
{
    size_t source_length = 0U;
    char *source = read_fixture("comb_continuous_assign_boolean.sv",
                                &source_length);
    TEST_ASSERT_NOT_NULL(source);
    lemma_frontend_result *result =
        lemma_frontend_build(source, source_length);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->circuit);
    TEST_ASSERT_EQUAL_UINT(0U, result->diagnostic_count);
    TEST_ASSERT_NULL(result->diagnostics);

    lemma_circuit *circuit = result->circuit;
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signal_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->driver_count);
    TEST_ASSERT_EQUAL_UINT(6U, circuit->expression_count);
    TEST_ASSERT_EQUAL_UINT(3U, circuit->drivers[0].target_signal_id);
    TEST_ASSERT_EQUAL_UINT(5U, circuit->drivers[0].expression_id);

    TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_READ_SIGNAL,
                      circuit->expressions[0].kind);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->expressions[0].signal_id);
    TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_READ_SIGNAL,
                      circuit->expressions[1].kind);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->expressions[1].signal_id);
    TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_LOGICAL_AND,
                      circuit->expressions[2].kind);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->expressions[2].operand_ids[0]);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->expressions[2].operand_ids[1]);
    TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_READ_SIGNAL,
                      circuit->expressions[3].kind);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->expressions[3].signal_id);
    TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_LOGICAL_NOT,
                      circuit->expressions[4].kind);
    TEST_ASSERT_EQUAL_UINT(3U, circuit->expressions[4].operand_ids[0]);
    TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_LOGICAL_OR,
                      circuit->expressions[5].kind);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->expressions[5].operand_ids[0]);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->expressions[5].operand_ids[1]);

    static const char *const expression_texts[] = {
        "a", "b", "a && b", "c", "!c", "(a && b) || !c"
    };
    for (size_t index = 0U; index < circuit->expression_count; ++index) {
        const lemma_ir_expression *expression = &circuit->expressions[index];
        TEST_ASSERT_EQUAL_UINT(1U, expression->width);
        TEST_ASSERT_FALSE(expression->is_signed);
        TEST_ASSERT_TRUE(range_matches_text(
            expression->source, source, expression_texts[index]));
    }
    TEST_ASSERT_TRUE(range_matches_text(
        circuit->drivers[0].source, source, "y = (a && b) || !c"));

    lemma_frontend_result_destroy(result);
    free(source);
}

//--------------------------------------------------------
// Build one operator fixture and compare its typed nodes, IDs, and provenance.
//
// @fixture: RTL fixture file name.
// @expected: Source-ordered operator expectations for every continuous driver.
// @expected_count: Number of operators and drivers in the fixture.
// @expression_count: Exact count including the signal-read leaves.
// @returns: None.
//--------------------------------------------------------
static void assert_operator_fixture(const char *fixture,
                                    const expected_operator *expected,
                                    size_t expected_count,
                                    size_t expression_count)
{
    size_t source_length = 0U;
    char *source = read_fixture(fixture, &source_length);
    TEST_ASSERT_NOT_NULL(source);
    lemma_frontend_result *result =
        lemma_frontend_build(source, source_length);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL_MESSAGE(result->circuit, fixture);
    TEST_ASSERT_EQUAL_UINT(0U, result->diagnostic_count);
    TEST_ASSERT_NULL(result->diagnostics);

    lemma_circuit *circuit = result->circuit;
    TEST_ASSERT_EQUAL_UINT(expected_count, circuit->driver_count);
    TEST_ASSERT_EQUAL_UINT(expression_count, circuit->expression_count);
    for (size_t index = 0U; index < expected_count; ++index) {
        const expected_operator *item = &expected[index];
        const lemma_ir_expression *root =
            &circuit->expressions[item->expression_id];
        TEST_ASSERT_EQUAL(item->kind, root->kind);
        TEST_ASSERT_EQUAL_UINT(item->left_expression_id,
                               root->operand_ids[0]);
        TEST_ASSERT_EQUAL_UINT(item->right_expression_id,
                               root->operand_ids[1]);
        TEST_ASSERT_EQUAL_UINT(item->result_width, root->width);
        TEST_ASSERT_FALSE(root->is_signed);
        TEST_ASSERT_TRUE(range_matches_text(
            root->source, source, item->expression_text));

        const lemma_ir_expression *left =
            &circuit->expressions[item->left_expression_id];
        TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_READ_SIGNAL, left->kind);
        TEST_ASSERT_EQUAL_UINT(item->left_signal_id, left->signal_id);
        TEST_ASSERT_EQUAL_UINT(4U, left->width);
        TEST_ASSERT_FALSE(left->is_signed);
        const char *left_name = circuit->signals[item->left_signal_id].name;
        TEST_ASSERT_TRUE(range_matches_text(left->source, source, left_name));
        if (item->right_signal_id != UINT32_MAX) {
            const lemma_ir_expression *right =
                &circuit->expressions[item->right_expression_id];
            TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_READ_SIGNAL, right->kind);
            TEST_ASSERT_EQUAL_UINT(item->right_signal_id, right->signal_id);
            TEST_ASSERT_EQUAL_UINT(4U, right->width);
            TEST_ASSERT_FALSE(right->is_signed);
            const char *right_name = circuit->signals[item->right_signal_id].name;
            TEST_ASSERT_TRUE(range_matches_text(right->source, source, right_name));
        } else {
            TEST_ASSERT_EQUAL_UINT(0U, root->operand_ids[1]);
        }

        const lemma_ir_driver *driver = &circuit->drivers[index];
        TEST_ASSERT_EQUAL_UINT(item->target_signal_id, driver->target_signal_id);
        TEST_ASSERT_EQUAL_UINT(item->expression_id, driver->expression_id);
        TEST_ASSERT_EQUAL_UINT(item->result_width,
                               circuit->signals[item->target_signal_id].width);
        TEST_ASSERT_FALSE(circuit->signals[item->target_signal_id].is_signed);
        TEST_ASSERT_TRUE(range_matches_text(
            driver->source, source, item->assignment_text));
    }
    lemma_frontend_result_destroy(result);
    free(source);
}

//--------------------------------------------------------
// Lower the six vector operator fixtures to typed, source-located circuit IR.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_lowers_vector_operator_fixtures(void)
{
    static const expected_operator bitwise[] = {
        {LEMMA_IR_EXPRESSION_BITWISE_AND, 2U, 0U, 1U, 0U, 1U, 2U, 4U,
         "a & b", "and_result = a & b"},
        {LEMMA_IR_EXPRESSION_BITWISE_OR, 5U, 3U, 4U, 0U, 1U, 3U, 4U,
         "a | b", "or_result = a | b"},
        {LEMMA_IR_EXPRESSION_BITWISE_XOR, 8U, 6U, 7U, 0U, 1U, 4U, 4U,
         "a ^ b", "xor_result = a ^ b"},
        {LEMMA_IR_EXPRESSION_BITWISE_XNOR, 11U, 9U, 10U, 0U, 1U, 5U, 4U,
         "a ~^ b", "xnor_tilde_caret_result = a ~^ b"},
        {LEMMA_IR_EXPRESSION_BITWISE_XNOR, 14U, 12U, 13U, 0U, 1U, 6U, 4U,
         "a ^~ b", "xnor_caret_tilde_result = a ^~ b"},
        {LEMMA_IR_EXPRESSION_BITWISE_NOT, 16U, 15U, 0U, 0U, UINT32_MAX, 7U, 4U,
         "~a", "complement_result = ~a"}
    };
    static const expected_operator reductions[] = {
        {LEMMA_IR_EXPRESSION_REDUCTION_AND, 1U, 0U, 0U, 0U, UINT32_MAX, 1U, 1U,
         "&value", "and_result = &value"},
        {LEMMA_IR_EXPRESSION_REDUCTION_NAND, 3U, 2U, 0U, 0U, UINT32_MAX, 2U, 1U,
         "~&value", "nand_result = ~&value"},
        {LEMMA_IR_EXPRESSION_REDUCTION_OR, 5U, 4U, 0U, 0U, UINT32_MAX, 3U, 1U,
         "|value", "or_result = |value"},
        {LEMMA_IR_EXPRESSION_REDUCTION_NOR, 7U, 6U, 0U, 0U, UINT32_MAX, 4U, 1U,
         "~|value", "nor_result = ~|value"},
        {LEMMA_IR_EXPRESSION_REDUCTION_XOR, 9U, 8U, 0U, 0U, UINT32_MAX, 5U, 1U,
         "^value", "xor_result = ^value"},
        {LEMMA_IR_EXPRESSION_REDUCTION_XNOR, 11U, 10U, 0U, 0U, UINT32_MAX, 6U, 1U,
         "~^value", "xnor_tilde_caret_result = ~^value"},
        {LEMMA_IR_EXPRESSION_REDUCTION_XNOR, 13U, 12U, 0U, 0U, UINT32_MAX, 7U, 1U,
         "^~value", "xnor_caret_tilde_result = ^~value"}
    };
    static const expected_operator addition[] = {
        {LEMMA_IR_EXPRESSION_ADD, 2U, 0U, 1U, 0U, 1U, 2U, 4U,
         "a + b", "sum = a + b"}
    };
    static const expected_operator subtraction[] = {
        {LEMMA_IR_EXPRESSION_SUBTRACT, 2U, 0U, 1U, 0U, 1U, 2U, 4U,
         "a - b", "difference = a - b"}
    };
    static const expected_operator multiplication[] = {
        {LEMMA_IR_EXPRESSION_MULTIPLY, 2U, 0U, 1U, 0U, 1U, 2U, 4U,
         "a * b", "product = a * b"}
    };
    static const expected_operator comparisons[] = {
        {LEMMA_IR_EXPRESSION_EQUAL, 2U, 0U, 1U, 0U, 1U, 2U, 1U,
         "a == b", "equal_result = a == b"},
        {LEMMA_IR_EXPRESSION_NOT_EQUAL, 5U, 3U, 4U, 0U, 1U, 3U, 1U,
         "a != b", "not_equal_result = a != b"},
        {LEMMA_IR_EXPRESSION_LESS, 8U, 6U, 7U, 0U, 1U, 4U, 1U,
         "a < b", "less_result = a < b"},
        {LEMMA_IR_EXPRESSION_LESS_EQUAL, 11U, 9U, 10U, 0U, 1U, 5U, 1U,
         "a <= b", "less_equal_result = a <= b"},
        {LEMMA_IR_EXPRESSION_GREATER, 14U, 12U, 13U, 0U, 1U, 6U, 1U,
         "a > b", "greater_result = a > b"},
        {LEMMA_IR_EXPRESSION_GREATER_EQUAL, 17U, 15U, 16U, 0U, 1U, 7U, 1U,
         "a >= b", "greater_equal_result = a >= b"}
    };

    assert_operator_fixture("comb_vector_bitwise_operators.sv", bitwise, 6U, 17U);
    assert_operator_fixture("comb_vector_reduction_operators.sv", reductions, 7U, 14U);
    assert_operator_fixture("comb_vector_unsigned_add.sv", addition, 1U, 3U);
    assert_operator_fixture("comb_vector_unsigned_subtract.sv", subtraction, 1U, 3U);
    assert_operator_fixture("comb_vector_unsigned_multiply.sv", multiplication, 1U, 3U);
    assert_operator_fixture("comb_vector_unsigned_comparisons.sv", comparisons, 6U, 18U);
}

//--------------------------------------------------------
// Return an owned result containing a circuit and an empty read-only diagnostic array.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_result_owns_successful_circuit(void)
{
    static const char source[] =
        "module top(input wire a, output wire y); assign y = a; endmodule";
    lemma_frontend_result *result =
        lemma_frontend_build(source, sizeof(source) - 1U);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result->circuit);
    TEST_ASSERT_EQUAL_STRING("top", result->circuit->module_name);
    TEST_ASSERT_EQUAL_UINT(0U, result->diagnostic_count);
    TEST_ASSERT_NULL(result->diagnostics);
    lemma_frontend_result_destroy(result);
    lemma_frontend_result_destroy(NULL);
}

//--------------------------------------------------------
// Return parse rejection as an error diagnostic with parser stage and source range.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_result_reports_parse_error(void)
{
    static const char source[] = "module broken (";
    lemma_frontend_result *result =
        lemma_frontend_build(source, sizeof(source) - 1U);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NULL(result->circuit);
    TEST_ASSERT_EQUAL_UINT(1U, result->diagnostic_count);
    TEST_ASSERT_NOT_NULL(result->diagnostics);
    const lemma_frontend_diagnostic *diagnostic = result->diagnostics;
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_DIAGNOSTIC_SEVERITY_ERROR,
                      diagnostic->severity);
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_STAGE_PARSING, diagnostic->stage);
    TEST_ASSERT_NOT_NULL(diagnostic->code);
    TEST_ASSERT_TRUE(diagnostic->has_source_range);
    TEST_ASSERT_TRUE(diagnostic->source_range.start_byte <
                     diagnostic->source_range.end_byte);
    lemma_frontend_result_destroy(result);
}

//--------------------------------------------------------
// Return unsupported RTL as an error diagnostic at the RTL stage with a source range.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_result_reports_rtl_error(void)
{
    static const char source[] =
        "module top(inout wire a, output wire y); endmodule";
    lemma_frontend_result *result =
        lemma_frontend_build(source, sizeof(source) - 1U);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NULL(result->circuit);
    TEST_ASSERT_EQUAL_UINT(1U, result->diagnostic_count);
    TEST_ASSERT_NOT_NULL(result->diagnostics);
    const lemma_frontend_diagnostic *diagnostic = result->diagnostics;
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_DIAGNOSTIC_SEVERITY_ERROR,
                      diagnostic->severity);
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                      diagnostic->stage);
    TEST_ASSERT_NOT_NULL(diagnostic->code);
    TEST_ASSERT_TRUE(diagnostic->has_source_range);
    TEST_ASSERT_TRUE(diagnostic->source_range.start_byte <
                     diagnostic->source_range.end_byte);
    lemma_frontend_result_destroy(result);
}

//--------------------------------------------------------
// Preserve vector width and signedness through the alias expression and driver.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_builds_signed_vector_alias_fixture(void)
{
    lemma_frontend_result *result =
        build_fixture("comb_vector_width_signedness_alias.sv");

    TEST_ASSERT_NOT_NULL(result);
    lemma_circuit *circuit = result->circuit;
    TEST_ASSERT_NOT_NULL(circuit);
    TEST_ASSERT_EQUAL_UINT(0U, result->diagnostic_count);
    TEST_ASSERT_NULL(result->diagnostics);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signal_count);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[0].width);
    TEST_ASSERT_TRUE(circuit->signals[0].is_signed);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[1].width);
    TEST_ASSERT_FALSE(circuit->signals[1].is_signed);
    TEST_ASSERT_TRUE(circuit->signals[2].is_signed);
    TEST_ASSERT_FALSE(circuit->signals[3].is_signed);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->driver_count);
    lemma_frontend_result_destroy(result);
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
    lemma_frontend_result *result =
        lemma_frontend_build(source, sizeof(source) - 1U);

    TEST_ASSERT_NOT_NULL(result);
    lemma_circuit *circuit = result->circuit;
    TEST_ASSERT_NOT_NULL(circuit);
    TEST_ASSERT_EQUAL_UINT(3U, circuit->port_count);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[0].width);
    TEST_ASSERT_TRUE(circuit->signals[0].is_signed);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[1].width);
    TEST_ASSERT_TRUE(circuit->signals[1].is_signed);
    TEST_ASSERT_EQUAL_STRING("b", circuit->signals[1].name);
    lemma_frontend_result_destroy(result);

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
        lemma_frontend_result *case_result = lemma_frontend_build(
            cases[index].source, strlen(cases[index].source));

        TEST_ASSERT_NOT_NULL_MESSAGE(case_result, cases[index].label);
        TEST_ASSERT_NOT_NULL(case_result->circuit);
        TEST_ASSERT_EQUAL_UINT(4U, case_result->circuit->signals[0].width);
        lemma_frontend_result_destroy(case_result);
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
        {"escaped user-defined type matching built-in spelling",
         "module top(input \\logic a); endmodule"},
        {"additional packed dimension beyond first-profile shape",
         "module top(input wire [1:0][3:0] a); endmodule"},
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
        // Unsupported expression forms and delayed assignments cannot be lowered.
        {"division assignment expression",
         "module top(input wire a, output wire y); assign y=a / a; endmodule"},
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
        lemma_frontend_result *result = lemma_frontend_build(
            cases[index].source, strlen(cases[index].source));
        TEST_ASSERT_NOT_NULL_MESSAGE(result, cases[index].label);
        TEST_ASSERT_NULL_MESSAGE(result->circuit, cases[index].label);
        TEST_ASSERT_EQUAL_UINT(1U, result->diagnostic_count);
        TEST_ASSERT_NOT_NULL(result->diagnostics);
        const lemma_frontend_diagnostic *diagnostic = result->diagnostics;
        TEST_ASSERT_EQUAL(LEMMA_FRONTEND_DIAGNOSTIC_SEVERITY_ERROR,
                          diagnostic->severity);
        TEST_ASSERT_NOT_NULL(diagnostic->code);
        TEST_ASSERT_TRUE(diagnostic->has_source_range);
        if (strcmp(cases[index].label,
                   "escaped user-defined type matching built-in spelling") == 0 ||
             strcmp(cases[index].label,
                    "additional packed dimension beyond first-profile shape") == 0) {
            TEST_ASSERT_EQUAL_STRING("rtl.port_form", diagnostic->code);
        }
        lemma_frontend_result_destroy(result);
    }
}

//--------------------------------------------------------
// Reject unsupported bitwise/ternary expressions and detect nested-read cycles.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_rejects_unsupported_expressions_and_nested_cycles(void)
{
    // Boundary cases must reject nearby syntax at RTL validation, not parse time.
    static const struct {
        const char *label;
        const char *expression;
    } unsupported_cases[] = {
        {"division is outside the supported operator slice", "a / b"},
        {"ternary expression is not lowered in this slice", "a ? b : c"}
    };
    for (size_t index = 0U;
         index < sizeof(unsupported_cases) / sizeof(unsupported_cases[0]);
         ++index) {
        char source[256];
        int written = snprintf(
            source, sizeof(source),
            "module top(input wire a, input wire b, input wire c, "
            "output wire y); assign y = %s; endmodule",
            unsupported_cases[index].expression);
        TEST_ASSERT_TRUE_MESSAGE(
            written >= 0 && (size_t)written < sizeof(source),
            unsupported_cases[index].label);
        lemma_frontend_result *result =
            lemma_frontend_build(source, (size_t)written);
        TEST_ASSERT_NOT_NULL_MESSAGE(result, unsupported_cases[index].label);
        TEST_ASSERT_NULL_MESSAGE(result->circuit, unsupported_cases[index].label);
        TEST_ASSERT_EQUAL_UINT(1U, result->diagnostic_count);
        const lemma_frontend_diagnostic *diagnostic = result->diagnostics;
        TEST_ASSERT_EQUAL(
            LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION, diagnostic->stage);
        TEST_ASSERT_EQUAL_STRING("rtl.expression_form", diagnostic->code);
        TEST_ASSERT_TRUE(diagnostic->has_source_range);
        TEST_ASSERT_TRUE(range_matches_text(
            diagnostic->source_range, source,
            unsupported_cases[index].expression));
        lemma_frontend_result_destroy(result);
    }

    // A signal read under a logical operator remains a dependency of the driver.
    static const char cycle_source[] =
        "module top(input wire a, output wire y); "
        "assign y = y && a; endmodule";
    lemma_frontend_result *cycle_result =
        lemma_frontend_build(cycle_source, sizeof(cycle_source) - 1U);
    TEST_ASSERT_NOT_NULL(cycle_result);
    TEST_ASSERT_NULL(cycle_result->circuit);
    TEST_ASSERT_EQUAL_UINT(1U, cycle_result->diagnostic_count);
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                      cycle_result->diagnostics[0].stage);
    TEST_ASSERT_EQUAL_STRING("rtl.combinational_cycle",
                            cycle_result->diagnostics[0].code);
    lemma_frontend_result_destroy(cycle_result);
}

//--------------------------------------------------------
// Reject operator type mismatches and nearby forms outside the supported slice.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_rejects_unsupported_operator_boundaries(void)
{
    // Operator categories reject unsupported math, sizing, and signed operands.
    // Syntax outside this slice is rejected at RTL validation with a stable range.
    static const struct {
        const char *label;
        const char *source;
        const char *code;
        const char *range_text;
    } cases[] = {
        {"division operator",
         "module top(input wire [3:0] a, input wire [3:0] b, "
         "output wire [3:0] y); assign y = a / b; endmodule",
         "rtl.expression_form", "a / b"},
        {"remainder operator",
         "module top(input wire [3:0] a, input wire [3:0] b, "
         "output wire [3:0] y); assign y = a % b; endmodule",
         "rtl.expression_form", "a % b"},
        {"case equality",
         "module top(input wire [3:0] a, input wire [3:0] b, "
         "output wire y); assign y = a === b; endmodule",
         "rtl.expression_form", "a === b"},
        {"mixed-width operator operands",
         "module top(input wire [3:0] a, input wire [2:0] b, "
         "output wire [3:0] y); assign y = a + b; endmodule",
         "rtl.operator_width", "a + b"},
        {"signed operator operands",
         "module top(input logic signed [3:0] a, "
         "input logic signed [3:0] b, output wire [3:0] y); "
         "assign y = a + b; endmodule",
         "rtl.operator_signedness", "a + b"},
        // Constants, concatenation, selects, and conversions are separate excluded forms.
        {"sized constant operand",
         "module top(input wire [3:0] a, output wire [3:0] y); "
         "assign y = a + 4'd1; endmodule",
         "rtl.expression_form", "4'd1"},
        {"packed concatenation",
         "module top(input wire [3:0] a, input wire [3:0] b, "
         "output wire [7:0] y); assign y = {a, b}; endmodule",
         "rtl.expression_form", "{a, b}"},
        {"packed selection",
         "module top(input wire [3:0] a, output wire y); "
         "assign y = a[0]; endmodule",
         "rtl.expression_form", "a[0]"},
        {"signedness cast",
         "module top(input wire [3:0] a, output wire [3:0] y); "
         "assign y = unsigned'(a); endmodule",
         "rtl.expression_form", "unsigned'(a)"},
        {"system signedness conversion",
         "module top(input wire [3:0] a, output wire [3:0] y); "
         "assign y = $unsigned(a); endmodule",
         "rtl.expression_form", "$unsigned(a)"},
        // Narrowing assignment would need warning and explicit conversion support.
        {"narrowing assignment requiring a warning",
         "module top(input wire [3:0] a, output wire [2:0] y); "
         "assign y = a; endmodule",
         "rtl.alias_width", "a"},
        // Processes and FFDs are not part of this continuous-expression slice.
        {"combinational process",
         "module top(input wire a, output logic y); "
         "always_comb y = a; endmodule",
         "rtl.unsupported_procedural_logic", "always_comb y = a;"},
        {"clocked FFD process",
         "module top(input wire clk, input wire a, output logic y); "
         "always_ff @(posedge clk) y <= a; endmodule",
         "rtl.unsupported_procedural_logic", "always_ff @(posedge clk) y <= a;"}
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        lemma_frontend_result *result = lemma_frontend_build(
            cases[index].source, strlen(cases[index].source));
        TEST_ASSERT_NOT_NULL_MESSAGE(result, cases[index].label);
        TEST_ASSERT_NULL_MESSAGE(result->circuit, cases[index].label);
        TEST_ASSERT_EQUAL_UINT(1U, result->diagnostic_count);
        TEST_ASSERT_NOT_NULL(result->diagnostics);
        const lemma_frontend_diagnostic *diagnostic = result->diagnostics;
        TEST_ASSERT_EQUAL(
            LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION, diagnostic->stage);
        TEST_ASSERT_EQUAL_STRING(cases[index].code, diagnostic->code);
        TEST_ASSERT_TRUE(diagnostic->has_source_range);
        TEST_ASSERT_TRUE(range_matches_text(
            diagnostic->source_range, cases[index].source,
            cases[index].range_text));
        lemma_frontend_result_destroy(result);
    }
}

//--------------------------------------------------------
// Extract built-in type, var, signedness, and first-dimension bounds as syntax.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_extracts_typed_declaration_facts(void)
{
    static const char source[] =
        "module facts(input wire signed [3:0] a, input var logic unsigned b, "
        "input \\logic c); endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(
        source, sizeof(source) - 1U, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);

    declaration_facts_capture capture = {0};
    lemma_syntax_visit_result result = lemma_frontend_visit_syntax(
        parsed, capture_declaration_facts, &capture);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_COMPLETE, result);
    TEST_ASSERT_EQUAL_UINT(3U, capture.groups);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_BUILTIN_TYPE_WIRE, capture.builtin_types[0]);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_SIGNEDNESS_SIGNED, capture.signedness[0]);
    TEST_ASSERT_EQUAL_UINT(1U, capture.dimension_counts[0]);
    TEST_ASSERT_FALSE(capture.has_var[0]);
    TEST_ASSERT_TRUE(capture.first_bounds_match);
    TEST_ASSERT_TRUE(capture.first_raw_dimensions_match);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_BUILTIN_TYPE_LOGIC, capture.builtin_types[1]);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_SIGNEDNESS_UNSIGNED, capture.signedness[1]);
    TEST_ASSERT_EQUAL_UINT(0U, capture.dimension_counts[1]);
    TEST_ASSERT_TRUE(capture.has_var[1]);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_BUILTIN_TYPE_OTHER, capture.builtin_types[2]);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_SIGNEDNESS_UNSPECIFIED, capture.signedness[2]);
    TEST_ASSERT_EQUAL_UINT(0U, capture.dimension_counts[2]);
    TEST_ASSERT_FALSE(capture.has_var[2]);
    lemma_parsed_source_destroy(parsed);
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

//--------------------------------------------------------
// Fail syntax visitation safely for deeply nested expressions and process syntax.
//
// @returns: None.
//--------------------------------------------------------
void test_frontend_rejects_overdeep_syntax_walks(void)
{
    static const char prefix[] =
        "module deep(input logic a, output logic y); always_comb ";
    static const char suffix[] = "y = a; endmodule";
    char source[4096];
    size_t offset = 0U;
    memcpy(source + offset, prefix, sizeof(prefix) - 1U);
    offset += sizeof(prefix) - 1U;
    for (size_t index = 0U; index < 160U; ++index) {
        memcpy(source + offset, "if (a) ", 7U);
        offset += 7U;
    }
    memcpy(source + offset, suffix, sizeof(suffix));

    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(
        source, offset + sizeof(suffix) - 1U, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);

    syntax_counts counts = {0};
    lemma_syntax_visit_result result = lemma_frontend_visit_syntax(
        parsed, count_syntax_event, &counts);
    lemma_parsed_source_destroy(parsed);

    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_FAILED, result);

    static const char loop_prefix[] =
        "module deep(input logic a, output logic y); always_comb ";
    static const char loop_suffix[] = "y = a; endmodule";
    offset = 0U;
    memcpy(source + offset, loop_prefix, sizeof(loop_prefix) - 1U);
    offset += sizeof(loop_prefix) - 1U;
    for (size_t index = 0U; index < 160U; ++index) {
        memcpy(source + offset, "repeat (1) ", 11U);
        offset += 11U;
    }
    memcpy(source + offset, loop_suffix, sizeof(loop_suffix));
    parsed = lemma_frontend_parse(
        source, offset + sizeof(loop_suffix) - 1U, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);
    result = lemma_frontend_visit_syntax(parsed, count_syntax_event, &counts);
    lemma_parsed_source_destroy(parsed);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_FAILED, result);

    static const char event_prefix[] =
        "module deep(input logic a, output logic y); always @(posedge a";
    static const char event_suffix[] = ") y = a; endmodule";
    offset = 0U;
    memcpy(source + offset, event_prefix, sizeof(event_prefix) - 1U);
    offset += sizeof(event_prefix) - 1U;
    for (size_t index = 0U; index < 160U; ++index) {
        memcpy(source + offset, " or posedge a", 13U);
        offset += 13U;
    }
    memcpy(source + offset, event_suffix, sizeof(event_suffix));
    parsed = lemma_frontend_parse(
        source, offset + sizeof(event_suffix) - 1U, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);
    memset(&counts, 0, sizeof(counts));
    result = lemma_frontend_visit_syntax(parsed, count_syntax_event, &counts);
    lemma_parsed_source_destroy(parsed);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_FAILED, result);

    static const char expression_prefix[] =
        "module deep(input wire a, output wire y); assign y = ";
    static const char expression_middle[] = "a";
    static const char expression_suffix[] = "; endmodule";
    offset = 0U;
    memcpy(source + offset, expression_prefix, sizeof(expression_prefix) - 1U);
    offset += sizeof(expression_prefix) - 1U;
    for (size_t index = 0U; index < 160U; ++index) {
        source[offset++] = '(';
    }
    memcpy(source + offset, expression_middle, sizeof(expression_middle) - 1U);
    offset += sizeof(expression_middle) - 1U;
    for (size_t index = 0U; index < 160U; ++index) {
        source[offset++] = ')';
    }
    memcpy(source + offset, expression_suffix, sizeof(expression_suffix));
    parsed = lemma_frontend_parse(
        source, offset + sizeof(expression_suffix) - 1U, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);
    result = lemma_frontend_visit_syntax(parsed, count_syntax_event, &counts);
    lemma_parsed_source_destroy(parsed);
    TEST_ASSERT_EQUAL(LEMMA_SYNTAX_VISIT_FAILED, result);
}
