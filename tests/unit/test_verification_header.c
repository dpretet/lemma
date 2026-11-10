#include "unity.h"

#include "lemma/verification.h"

// These compile-time type checks ensure the public function declarations keep
// their intended signatures; they do not require or link implementations.
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

//--------------------------------------------------------
// Check representative public records with live pointer targets, key field
// values, and distinct diagnostic stages; this is not a job or proof test.
//
// @returns: None.
//--------------------------------------------------------
void test_verification_header_declarations(void)
{
    const lemma_diagnostic_stage diagnostic_stages[] = {
        LEMMA_DIAGNOSTIC_STAGE_JOB,
        LEMMA_DIAGNOSTIC_STAGE_PARSING,
        LEMMA_DIAGNOSTIC_STAGE_RTL_VALIDATION_ELABORATION,
        LEMMA_DIAGNOSTIC_STAGE_CIRCUIT_IR,
        LEMMA_DIAGNOSTIC_STAGE_PROPERTY_MODEL,
        LEMMA_DIAGNOSTIC_STAGE_PROOF_MODEL,
        LEMMA_DIAGNOSTIC_STAGE_PROOF_EXECUTION
    };
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
        LEMMA_DIAGNOSTIC_STAGE_PARSING,
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
    TEST_ASSERT_EQUAL(LEMMA_DIAGNOSTIC_STAGE_PARSING, diagnostic.stage);
    TEST_ASSERT_TRUE(diagnostic.has_source_range);
    TEST_ASSERT_EQUAL_UINT(13U, diagnostic.source_range.end_byte);
    TEST_ASSERT_NULL(job_diagnostic.source_name);
    TEST_ASSERT_FALSE(job_diagnostic.has_source_range);
    TEST_ASSERT_EQUAL(LEMMA_PROPERTY_ASSERTION, proof.target_kind);
    TEST_ASSERT_EQUAL(LEMMA_PROOF_OUTCOME_INCONCLUSIVE, proof.outcome);
    TEST_ASSERT_EQUAL(LEMMA_JOB_STATUS_REJECTED, result.status);
    TEST_ASSERT_NULL(result.proof);

    for (size_t i = 0U;
         i < sizeof(diagnostic_stages) / sizeof(diagnostic_stages[0]);
         ++i) {
        for (size_t j = i + 1U;
             j < sizeof(diagnostic_stages) / sizeof(diagnostic_stages[0]);
             ++j) {
            TEST_ASSERT_TRUE(diagnostic_stages[i] != diagnostic_stages[j]);
        }
    }
}
