#include "lemma/frontend.h"

#include "lemma/circuit_ir_builder_internal.h"
#include "lemma/frontend_elaboration.h"
#include "lemma/frontend_parse.h"

#include <stdio.h>
#include <string.h>

//--------------------------------------------------------
// Set an optional circuit-IR diagnostic after builder allocation/finalization failure.
//
// @diagnostic: Optional caller-owned diagnostic output.
// @code: Static diagnostic code string.
// @message: Concise diagnostic message.
// @returns: None.
//--------------------------------------------------------
static void set_ir_diagnostic(lemma_frontend_diagnostic *diagnostic,
                              const char *code, const char *message)
{
    if (diagnostic == NULL) {
        return;
    }
    memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->stage = LEMMA_FRONTEND_STAGE_CIRCUIT_IR;
    diagnostic->code = code;
    (void)snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", message);
}

//--------------------------------------------------------
// Run every implemented frontend stage and return the sole-module circuit.
//
// @source: Non-NULL source buffer borrowed for the duration of this call.
// @length: Number of source bytes; must fit Tree-sitter's uint32_t limit.
// @diagnostic: Optional diagnostic output, cleared by the parsing stage.
// @returns: Caller-owned finalized circuit, or NULL after parse, RTL, or IR failure.
//--------------------------------------------------------
lemma_circuit *lemma_frontend_build(const char *source, size_t length,
                                    lemma_frontend_diagnostic *diagnostic)
{
    lemma_parsed_source *parsed = lemma_frontend_parse(source, length, diagnostic);
    if (parsed == NULL) {
        return NULL;
    }

    lemma_circuit_builder *builder = lemma_circuit_builder_create();
    if (builder == NULL) {
        set_ir_diagnostic(diagnostic, "ir.out_of_memory",
                          "unable to allocate circuit builder");
        lemma_parsed_source_destroy(parsed);
        return NULL;
    }

    if (!lemma_frontend_elaborate(parsed, builder, diagnostic)) {
        lemma_circuit_builder_destroy(builder);
        lemma_parsed_source_destroy(parsed);
        return NULL;
    }
    lemma_parsed_source_destroy(parsed);

    lemma_circuit *circuit = lemma_circuit_builder_finalize(builder);
    if (circuit == NULL) {
        set_ir_diagnostic(diagnostic, "ir.out_of_memory",
                          "unable to finalize circuit storage");
        lemma_circuit_builder_destroy(builder);
        return NULL;
    }
    return circuit;
}
