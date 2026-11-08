#ifndef LEMMA_VERIFICATION_H
#define LEMMA_VERIFICATION_H

#include <stdbool.h>
#include <stddef.h>

#include "lemma/trace.h"

/*
 * A named source buffer. size is the number of bytes in data. The name and data
 * are borrowed only for the duration of a job call.
 */
typedef struct lemma_source {
    const char *name;
    const unsigned char *data;
    size_t size;
} lemma_source;

// Parameter values use the explicitly sized bit-vector representation.
typedef struct lemma_parameter_override {
    const char *name;
    lemma_bit_vector value;
} lemma_parameter_override;

/*
 * A job selects one property in a named top-level design. All strings, arrays,
 * and byte buffers referenced by a job are borrowed by lemma_run_job and are
 * not retained after return.
 */
typedef struct lemma_job {
    const lemma_source *sources;
    size_t source_count;
    const char *top_name;
    const char *property_label;
    const lemma_parameter_override *parameter_overrides;
    size_t parameter_override_count;
} lemma_job;

typedef enum lemma_property_kind {
    LEMMA_PROPERTY_ASSERTION,
    LEMMA_PROPERTY_COVER
} lemma_property_kind;

/* Assertions use proven/failed/inconclusive; covers use covered/unreachable/
 * inconclusive, as specified in doc/tool-flow.md. */
typedef enum lemma_proof_outcome {
    LEMMA_PROOF_OUTCOME_PROVEN,
    LEMMA_PROOF_OUTCOME_FAILED,
    LEMMA_PROOF_OUTCOME_INCONCLUSIVE,
    LEMMA_PROOF_OUTCOME_COVERED,
    LEMMA_PROOF_OUTCOME_UNREACHABLE
} lemma_proof_outcome;

typedef enum lemma_job_status {
    LEMMA_JOB_STATUS_COMPLETED,
    LEMMA_JOB_STATUS_REJECTED,
    LEMMA_JOB_STATUS_INTERNAL_ERROR
} lemma_job_status;

typedef enum lemma_diagnostic_stage {
    LEMMA_DIAGNOSTIC_STAGE_JOB,
    LEMMA_DIAGNOSTIC_STAGE_PARSING,
    LEMMA_DIAGNOSTIC_STAGE_RTL_VALIDATION_ELABORATION,
    LEMMA_DIAGNOSTIC_STAGE_CIRCUIT_IR,
    LEMMA_DIAGNOSTIC_STAGE_PROPERTY_MODEL,
    LEMMA_DIAGNOSTIC_STAGE_PROOF_MODEL,
    LEMMA_DIAGNOSTIC_STAGE_PROOF_EXECUTION
} lemma_diagnostic_stage;

typedef enum lemma_diagnostic_severity {
    LEMMA_DIAGNOSTIC_SEVERITY_WARNING,
    LEMMA_DIAGNOSTIC_SEVERITY_ERROR
} lemma_diagnostic_severity;

/*
 * Source positions preserve Tree-sitter's zero-based rows and byte columns.
 * The byte range is [start_byte, end_byte); the end position is exclusive.
 */
typedef struct lemma_source_span {
    size_t start_byte;
    size_t end_byte;
    size_t start_row;
    size_t start_column;
    size_t end_row;
    size_t end_column;
} lemma_source_span;

/*
 * The stable code, message, and source_name, when present, are owned by the
 * result. source_name is NULL for job-level diagnostics without a source;
 * has_span is false when no source span is available. If has_span is true,
 * span records the source range in byte offsets and zero-based row/byte-column
 * positions.
 */
typedef struct lemma_diagnostic {
    lemma_diagnostic_stage stage;
    lemma_diagnostic_severity severity;
    const char *code;
    const char *message;
    const char *source_name;
    bool has_span;
    lemma_source_span span;
} lemma_diagnostic;

// The target label and optional trace are owned by the containing result.
typedef struct lemma_proof_result {
    const char *target_label;
    lemma_property_kind target_kind;
    lemma_proof_outcome outcome;
    const lemma_trace *trace;
} lemma_proof_result;

/*
 * Result records, diagnostic strings, proof data, and trace data are read-only
 * and owned by this result until lemma_result_destroy is called. The proof
 * pointer is NULL when no proof result is available.
 */
typedef struct lemma_result {
    lemma_job_status status;
    size_t diagnostic_count;
    const lemma_diagnostic *diagnostics;
    const lemma_proof_result *proof;
} lemma_result;

//--------------------------------------------------------
// Run a verification job; execution is not yet implemented by the parser-stage
// executable.
//
// @job: Job whose referenced inputs are borrowed until this call returns.
// @returns: Caller-owned result with the job outcome, or NULL if a result object
//     could not be created; returned data remains owned until destruction.
//--------------------------------------------------------
lemma_result *lemma_run_job(const lemma_job *job);

//--------------------------------------------------------
// Destroy a caller-owned job result and release its storage.
//
// @result: Caller-owned result to destroy; do not access it after destruction.
// @returns: None.
//--------------------------------------------------------
void lemma_result_destroy(lemma_result *result);

//--------------------------------------------------------
// Return the status from a valid, live caller-owned result.
//
// @result: Valid, live result owned by the caller.
// @returns: The job status stored in the result.
//--------------------------------------------------------
lemma_job_status lemma_result_status(const lemma_result *result);

//--------------------------------------------------------
// Return the diagnostic count from a valid, live caller-owned result.
//
// @result: Valid, live result owned by the caller.
// @returns: Number of diagnostics stored in the result.
//--------------------------------------------------------
size_t lemma_result_diagnostic_count(const lemma_result *result);

//--------------------------------------------------------
// Return the read-only diagnostics array from a valid, live caller-owned result.
// Its contents remain valid until result destruction.
//
// @result: Valid, live result owned by the caller.
// @returns: Read-only diagnostics array owned by the result, or NULL when it
//     contains no diagnostics.
//--------------------------------------------------------
const lemma_diagnostic *lemma_result_diagnostics(const lemma_result *result);

//--------------------------------------------------------
// Return the optional proof from a valid, live caller-owned result; its data
// remains valid until result destruction.
//
// @result: Valid, live result owned by the caller.
// @returns: Read-only proof result owned by the result, or NULL when no proof is
//     available.
//--------------------------------------------------------
const lemma_proof_result *lemma_result_proof(const lemma_result *result);

#endif
