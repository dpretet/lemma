#include "lemma/frontend.h"

#include "lemma/frontend_ir_builder.h"
#include "lemma/frontend_elaboration.h"
#include "lemma/frontend_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct frontend_result_storage {
    lemma_frontend_result result;
    lemma_frontend_diagnostic diagnostic;
} frontend_result_storage;

//---------------------------------------------------------------------
// Copy one stage diagnostic into the result-owned error slot.
//
// @storage: Result allocation whose diagnostic slot receives the copy.
// @diagnostic: Stage diagnostic copied into the result.
// @returns: None.
//---------------------------------------------------------------------
static void set_result_diagnostic(frontend_result_storage *storage,
                                  const lemma_frontend_diagnostic *diagnostic)
{
    storage->diagnostic = *diagnostic;
    storage->diagnostic.severity = LEMMA_FRONTEND_DIAGNOSTIC_SEVERITY_ERROR;
    storage->result.diagnostic_count = 1U;
    storage->result.diagnostics = &storage->diagnostic;
}

//----------------------------------------------------------------------
// Set a frontend error diagnostic for a failure without a stage record.
//
// @diagnostic: Diagnostic record receiving the error.
// @stage: Frontend stage that detected the failure.
// @code: Static diagnostic code string.
// @message: Concise diagnostic message.
// @returns: None.
//----------------------------------------------------------------------
static void set_frontend_diagnostic(lemma_frontend_diagnostic *diagnostic,
                                    lemma_frontend_stage stage, const char *code,
                                    const char *message)
{
    memset(diagnostic, 0, sizeof(*diagnostic));
    diagnostic->stage = stage;
    diagnostic->severity = LEMMA_FRONTEND_DIAGNOSTIC_SEVERITY_ERROR;
    diagnostic->code = code;
    (void)snprintf(
        diagnostic->message,
        sizeof(diagnostic->message),
        "%s",
        message
    );
}

//---------------------------------------------------------------------------
// Run every implemented frontend stage and return its circuit or diagnostic.
//
// @source: Non-NULL source buffer borrowed for the duration of this call.
// @length: Number of source bytes; must fit Tree-sitter's uint32_t limit.
// @returns: Caller-owned result, or NULL only when result allocation fails.
//---------------------------------------------------------------------------
lemma_frontend_result *lemma_frontend_build(const char *source, size_t length)
{
    frontend_result_storage *storage = calloc(1U, sizeof(*storage));
    if (storage == NULL) {
        return NULL;
    }
    lemma_frontend_diagnostic diagnostic = {0};
    lemma_parsed_source *parsed = lemma_frontend_parse(source, length, &diagnostic);
    if (parsed == NULL) {
        if (diagnostic.code == NULL) {
            set_frontend_diagnostic(
                &diagnostic,
                LEMMA_FRONTEND_STAGE_PARSING,
                "frontend.missing_diagnostic",
                "parsing failed without a diagnostic"
            );
        }
        set_result_diagnostic(storage, &diagnostic);
        return &storage->result;
    }

    lemma_circuit_builder *builder = lemma_circuit_builder_create();
    if (builder == NULL) {
        set_frontend_diagnostic(
            &diagnostic,
            LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
            "ir.out_of_memory",
            "unable to allocate circuit builder"
        );
        set_result_diagnostic(storage, &diagnostic);
        lemma_parsed_source_destroy(parsed);
        return &storage->result;
    }

    if (!lemma_frontend_elaborate(parsed, builder, &diagnostic)) {
        lemma_circuit_builder_destroy(builder);
        lemma_parsed_source_destroy(parsed);
        if (diagnostic.code == NULL) {
            set_frontend_diagnostic(
                &diagnostic,
                LEMMA_FRONTEND_STAGE_RTL_VALIDATION_ELABORATION,
                "frontend.missing_diagnostic",
                "RTL elaboration failed without a diagnostic"
            );
        }
        set_result_diagnostic(storage, &diagnostic);
        return &storage->result;
    }
    lemma_parsed_source_destroy(parsed);

    storage->result.circuit = lemma_circuit_builder_finalize(builder);
    if (storage->result.circuit == NULL) {
        set_frontend_diagnostic(
            &diagnostic,
            LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
            "ir.out_of_memory",
            "unable to finalize circuit storage"
        );
        set_result_diagnostic(storage, &diagnostic);
        lemma_circuit_builder_destroy(builder);
        return &storage->result;
    }
    return &storage->result;
}

//---------------------------------------------------------------------------
// Destroy a frontend result and the storage containing its diagnostic array.
//
// @result: Caller-owned result, or NULL; do not access it after destruction.
// @returns: None.
//---------------------------------------------------------------------------
void lemma_frontend_result_destroy(lemma_frontend_result *result)
{
    if (result != NULL) {
        frontend_result_storage *storage = (frontend_result_storage *)result;
        lemma_circuit_destroy(result->circuit);
        free(storage);
    }
}
