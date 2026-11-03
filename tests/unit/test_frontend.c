#include "frontend.h"

#include "unity.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Read a named RTL fixture into caller-owned, NUL-terminated storage and return
// its byte length. Return NULL on path, I/O, size, or allocation failure; on
// failure no file or buffer remains owned by the caller.
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
    if (file_size < 0 || (uintmax_t)file_size >= SIZE_MAX) {
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
    fclose(file);
    contents[size] = '\0';
    *length = size;
    return contents;
}

// Parse and build IR for one fixture and selected top. The parsed and
// elaborated intermediates and fixture buffer are always released here; the
// returned circuit, if any, belongs to the caller. A failed stage returns NULL.
static lemma_circuit *build_fixture(const char *fixture, const char *top,
                                    lemma_frontend_diagnostic *diagnostic)
{
    size_t length = 0U;
    char *source = read_fixture(fixture, &length);
    if (source == NULL) {
        return NULL;
    }
    lemma_parsed_design *parsed = lemma_frontend_parse(source, length, diagnostic);
    free(source);
    if (parsed == NULL) {
        return NULL;
    }
    lemma_elaborated_design *elaborated = lemma_frontend_elaborate(parsed, top, diagnostic);
    lemma_circuit *circuit = NULL;
    if (elaborated != NULL) {
        circuit = lemma_frontend_build_ir(parsed, elaborated, diagnostic);
    }
    lemma_frontend_elaborate_destroy(elaborated);
    lemma_parsed_design_destroy(parsed);
    return circuit;
}

// Treat either parse failure or elaboration rejection as rejection. On a
// successful parse, destroy both temporary objects before returning; this
// helper intentionally does not attempt IR construction.
static int rejects_source(const char *source, const char *top,
                          lemma_frontend_diagnostic *diagnostic)
{
    lemma_parsed_design *parsed = lemma_frontend_parse(source, strlen(source), diagnostic);
    if (parsed == NULL) {
        return 1;
    }
    lemma_elaborated_design *elaborated = lemma_frontend_elaborate(parsed, top, diagnostic);
    lemma_frontend_elaborate_destroy(elaborated);
    lemma_parsed_design_destroy(parsed);
    return elaborated == NULL;
}

// The scalar direct-alias fixture builds a two-port circuit with one input
// read expression and one output driver, no sequential elements, and source
// spans attached to its signal, expression, and driver records.
// This is one slice of the initial alias profile, not general RTL or proof.
void test_frontend_builds_scalar_alias_fixture_ir(void)
{
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_circuit *circuit = build_fixture("comb_continuous_assign_alias.sv",
                                           "continuous_assign_alias", &diagnostic);

    TEST_ASSERT_NOT_NULL_MESSAGE(circuit, diagnostic.message);
    TEST_ASSERT_EQUAL_STRING("continuous_assign_alias", circuit->module_name);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->port_count);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->signal_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->expression_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->driver_count);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->ffd_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->signals[0].width);
    TEST_ASSERT_FALSE(circuit->signals[0].is_signed);
    TEST_ASSERT_TRUE(circuit->signals[0].source.end_byte >
                     circuit->signals[0].source.start_byte);
    TEST_ASSERT_EQUAL_STRING("a", circuit->signals[0].name);
    TEST_ASSERT_EQUAL(LEMMA_IR_PORT_INPUT, circuit->ports[0].direction);
    TEST_ASSERT_EQUAL_STRING("y", circuit->signals[1].name);
    TEST_ASSERT_EQUAL(LEMMA_IR_PORT_OUTPUT, circuit->ports[1].direction);
    TEST_ASSERT_EQUAL(LEMMA_IR_EXPRESSION_READ_SIGNAL, circuit->expressions[0].kind);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->expressions[0].signal_id);
    TEST_ASSERT_TRUE(circuit->expressions[0].source.end_byte >
                     circuit->expressions[0].source.start_byte);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->drivers[0].target_signal_id);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->drivers[0].expression_id);
    TEST_ASSERT_TRUE(circuit->drivers[0].source.end_byte >
                     circuit->drivers[0].source.start_byte);
    lemma_circuit_destroy(circuit);
}

// The explicitly sized vector fixture preserves four-bit widths and the
// signedness of each endpoint, read expression, and corresponding driver.
// Together the positive fixture tests cover only the first scalar/vector alias
// profile; they do not claim support for all RTL or perform proof.
void test_frontend_builds_signed_and_unsigned_vector_alias_fixture_ir(void)
{
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_circuit *circuit = build_fixture("comb_vector_width_signedness_alias.sv",
                                           "vector_width_signedness_alias", &diagnostic);

    TEST_ASSERT_NOT_NULL_MESSAGE(circuit, diagnostic.message);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->port_count);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signal_count);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->expression_count);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->driver_count);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[0].width);
    TEST_ASSERT_TRUE(circuit->signals[0].is_signed);
    TEST_ASSERT_EQUAL_STRING("signed_in", circuit->signals[0].name);
    TEST_ASSERT_EQUAL_UINT(4U, circuit->signals[1].width);
    TEST_ASSERT_FALSE(circuit->signals[1].is_signed);
    TEST_ASSERT_EQUAL_STRING("unsigned_in", circuit->signals[1].name);
    TEST_ASSERT_TRUE(circuit->signals[2].is_signed);
    TEST_ASSERT_FALSE(circuit->signals[3].is_signed);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->expressions[0].signal_id);
    TEST_ASSERT_TRUE(circuit->expressions[0].is_signed);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->expressions[1].signal_id);
    TEST_ASSERT_FALSE(circuit->expressions[1].is_signed);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->drivers[0].target_signal_id);
    TEST_ASSERT_EQUAL_UINT(3U, circuit->drivers[1].target_signal_id);
    lemma_circuit_destroy(circuit);
}

// Comments around the design unit, port header, body, and file tail are trivia;
// the remaining alias still builds the expected two-port circuit IR.
void test_frontend_ignores_comments_around_alias_module(void)
{
    static const char source[] =
        "// leading line comment\n"
        "module /* header block comment */ commented_alias (\n"
        "    input /* port header comment */ wire a,\n"
        "    /* port-list comment */ output wire y\n"
        ");\n"
        "    // body line comment\n"
        "    assign /* assignment comment */ y = a;\n"
        "endmodule\n"
        "// trailing line comment\n"
        "/* trailing block comment */\n";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_design *parsed = lemma_frontend_parse(source, sizeof(source) - 1U,
                                                       &diagnostic);

    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, diagnostic.message);
    lemma_elaborated_design *elaborated = lemma_frontend_elaborate(
        parsed, "commented_alias", &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(elaborated, diagnostic.message);
    lemma_circuit *circuit = lemma_frontend_build_ir(parsed, elaborated, &diagnostic);
    TEST_ASSERT_NOT_NULL_MESSAGE(circuit, diagnostic.message);
    TEST_ASSERT_EQUAL_STRING("commented_alias", circuit->module_name);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->port_count);
    TEST_ASSERT_EQUAL_UINT(2U, circuit->signal_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->expression_count);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->driver_count);
    TEST_ASSERT_EQUAL_STRING("a", circuit->signals[0].name);
    TEST_ASSERT_EQUAL(LEMMA_IR_PORT_INPUT, circuit->ports[0].direction);
    TEST_ASSERT_EQUAL_STRING("y", circuit->signals[1].name);
    TEST_ASSERT_EQUAL(LEMMA_IR_PORT_OUTPUT, circuit->ports[1].direction);
    TEST_ASSERT_EQUAL_UINT(0U, circuit->expressions[0].signal_id);
    TEST_ASSERT_EQUAL_UINT(1U, circuit->drivers[0].target_signal_id);

    lemma_circuit_destroy(circuit);
    lemma_frontend_elaborate_destroy(elaborated);
    lemma_parsed_design_destroy(parsed);
}

// Malformed syntax is rejected by parsing itself, before RTL validation or IR
// construction, and reports a parsing-stage syntax diagnostic with a span.
void test_frontend_rejects_malformed_source_at_parse_stage(void)
{
    static const char source[] = "module broken(input wire a) endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_design *parsed = lemma_frontend_parse(source, sizeof(source) - 1U,
                                                       &diagnostic);

    TEST_ASSERT_NULL(parsed);
    TEST_ASSERT_EQUAL(LEMMA_FRONTEND_STAGE_PARSING, diagnostic.stage);
    TEST_ASSERT_EQUAL_STRING("parse.syntax_error", diagnostic.code);
    TEST_ASSERT_TRUE(diagnostic.has_span);
}

// Each syntactically parseable example below must be rejected during
// elaboration-stage RTL validation, with a diagnostic code and source span.
// The cases are grouped by the kinds of constructs excluded from the current
// first scalar/vector alias profile; they are not a complete RTL conformance
// suite and do not test proof behavior.
void test_frontend_rejects_out_of_profile_constructs(void)
{
    static const char *cases[] = {
        // Parameters and unsupported declaration forms.
        "module top #(parameter N = 4) (input wire a, output wire y); assign y = a; endmodule",
        "module top(input var logic a, output logic y); assign y = a; endmodule",
        // Hierarchy, generate constructs, and procedural assignments.
        "module top(input wire a, output wire y); child u0(); assign y = a; endmodule",
        "module top(input wire a, output wire y); generate if (1) begin assign y = a; end endgenerate endmodule",
        "module top(input wire a, output logic y); always_comb y = a; endmodule",
        // Unsupported expressions and assignment delays.
        "module top(input wire a, output wire y); assign y = a & a; endmodule",
        "module top(input wire a, output wire y); assign #1 y = a; endmodule",
        // Invalid widths, ports, references, drivers, and combinational cycles.
        "module top(input wire [3:0] a, output wire [2:0] y); assign y = a; endmodule",
        "module top(input wire [N:0] a, output wire [N:0] y); assign y = a; endmodule",
        "module top(input wire a, output wire y); endmodule",
        "module top(inout wire a, output wire y); assign y = a; endmodule",
        "module top(input wire a, output wire y); assign y = missing; endmodule",
        "module top(input wire a, output wire y); assign y = a; assign y = a; endmodule",
        "module top(input wire a, output wire y); assign a = y; endmodule",
        "module top(input wire a, output wire y, output wire z); assign y = z; assign z = y; endmodule"
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        lemma_frontend_diagnostic diagnostic = {0};
        TEST_ASSERT_TRUE_MESSAGE(rejects_source(cases[index], "top", &diagnostic),
                                 cases[index]);
        TEST_ASSERT_EQUAL(LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                          diagnostic.stage);
        TEST_ASSERT_NOT_NULL(diagnostic.code);
        TEST_ASSERT_TRUE(diagnostic.has_span);
    }
}

// A design with multiple modules is rejected as non-flat even when the
// requested top exists, and the diagnostic identifies source text.
void test_frontend_rejects_multiple_modules(void)
{
    static const char source[] =
        "module top(input wire a); endmodule module other(input wire b); endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    TEST_ASSERT_TRUE(rejects_source(source, "top", &diagnostic));
    TEST_ASSERT_EQUAL_STRING("rtl.flat_design", diagnostic.code);
    TEST_ASSERT_TRUE(diagnostic.has_span);
}

// A parsed flat design still requires the caller-selected module to exist;
// failure is reported as a source-spanned top-not-found diagnostic.
void test_frontend_requires_selected_flat_module(void)
{
    static const char source[] = "module top(input wire a); endmodule";
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_design *parsed = lemma_frontend_parse(source, sizeof(source) - 1U,
                                                       &diagnostic);
    TEST_ASSERT_NOT_NULL(parsed);
    lemma_elaborated_design *elaborated = lemma_frontend_elaborate(parsed, "other", &diagnostic);
    TEST_ASSERT_NULL(elaborated);
    TEST_ASSERT_EQUAL_STRING("rtl.top_not_found", diagnostic.code);
    TEST_ASSERT_TRUE(diagnostic.has_span);
    lemma_parsed_design_destroy(parsed);
}
