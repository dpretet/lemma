#include "unity.h"

#include "lemma/verification.h"

_Static_assert(
    _Generic(&lemma_run_job, lemma_result *(*)(const lemma_job *): 1, default: 0),
    "lemma_run_job declaration has the expected signature");
_Static_assert(
    _Generic(&lemma_result_destroy, void (*)(lemma_result *): 1, default: 0),
    "lemma_result_destroy declaration has the expected signature");
_Static_assert(
    _Generic(&lemma_result_status,
             lemma_job_status (*)(const lemma_result *): 1,
             default: 0),
    "lemma_result_status declaration has the expected signature");
_Static_assert(
    _Generic(&lemma_result_diagnostic_count,
             size_t (*)(const lemma_result *): 1,
             default: 0),
    "lemma_result_diagnostic_count declaration has the expected signature");
_Static_assert(
    _Generic(&lemma_result_diagnostics,
             const lemma_diagnostic *(*)(const lemma_result *): 1,
             default: 0),
    "lemma_result_diagnostics declaration has the expected signature");
_Static_assert(
    _Generic(&lemma_result_proof,
             const lemma_proof_result *(*)(const lemma_result *): 1,
             default: 0),
    "lemma_result_proof declaration has the expected signature");

void test_verification_header_declarations(void)
{
    const unsigned char source_data[] = "module top; endmodule";
    const unsigned char parameter_bytes[] = {0x03U};
    const lemma_parameter_override parameter = {
        "WIDTH",
        {2U, false, parameter_bytes}
    };
    const lemma_source source = {
        "design.sv",
        source_data,
        sizeof(source_data) - 1U
    };
    const lemma_job job = {
        &source,
        1U,
        "top",
        "check_value",
        &parameter,
        1U
    };
    const lemma_diagnostic diagnostic = {
        LEMMA_DIAGNOSTIC_STAGE_PARSE,
        LEMMA_DIAGNOSTIC_SEVERITY_ERROR,
        "parse-error",
        "invalid syntax",
        "design.sv",
        true,
        {12U, 13U, 0U, 12U, 0U, 13U}
    };
    const lemma_diagnostic job_diagnostic = {
        LEMMA_DIAGNOSTIC_STAGE_JOB,
        LEMMA_DIAGNOSTIC_SEVERITY_ERROR,
        "invalid-job",
        "missing top name",
        NULL,
        false,
        {0U, 0U, 0U, 0U, 0U, 0U}
    };
    const lemma_proof_result proof = {
        "check_value",
        LEMMA_PROPERTY_ASSERTION,
        LEMMA_PROOF_OUTCOME_INCONCLUSIVE,
        NULL
    };
    const lemma_result result = {
        LEMMA_JOB_STATUS_REJECTED,
        1U,
        &diagnostic,
        NULL
    };

    TEST_ASSERT_EQUAL_UINT(1U, job.source_count);
    TEST_ASSERT_EQUAL_STRING("WIDTH", job.parameter_overrides[0].name);
    TEST_ASSERT_EQUAL(LEMMA_DIAGNOSTIC_STAGE_PARSE, diagnostic.stage);
    TEST_ASSERT_TRUE(diagnostic.has_span);
    TEST_ASSERT_EQUAL_UINT(13U, diagnostic.span.end_byte);
    TEST_ASSERT_NULL(job_diagnostic.source_name);
    TEST_ASSERT_FALSE(job_diagnostic.has_span);
    TEST_ASSERT_EQUAL(LEMMA_PROPERTY_ASSERTION, proof.target_kind);
    TEST_ASSERT_EQUAL(LEMMA_PROOF_OUTCOME_INCONCLUSIVE, proof.outcome);
    TEST_ASSERT_EQUAL(LEMMA_JOB_STATUS_REJECTED, result.status);
    TEST_ASSERT_NULL(result.proof);
}
