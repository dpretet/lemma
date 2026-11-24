#ifndef LEMMA_FRONTEND_H
#define LEMMA_FRONTEND_H

#include "circuit_ir.h"
#include "frontend_types.h"

#include <stddef.h>

// The circuit and diagnostics are owned by the result until it is destroyed.
// diagnostics is NULL when diagnostic_count is zero and read-only otherwise.
typedef struct lemma_frontend_result {
    lemma_circuit *circuit;
    size_t diagnostic_count;
    const lemma_frontend_diagnostic *diagnostics;
} lemma_frontend_result;

//-----------------------------------------------------------------------------
// Parse, extract, validate, elaborate, and construct the sole-module circuit.
// The input buffer is borrowed only for this synchronous call. Parse, RTL, and
// IR failures return a result with no circuit and an error diagnostic.
//
// @source: Non-NULL source buffer with at least length readable bytes.
// @length: Number of source bytes; must not exceed UINT32_MAX.
// @returns: Caller-owned result; its circuit and read-only diagnostics remain
//     valid until destruction. Returns NULL only if the result object cannot
//     be allocated.
//-----------------------------------------------------------------------------
lemma_frontend_result *lemma_frontend_build(const char *source, size_t length);

//------------------------------------------------------------------------------
// Destroy a caller-owned frontend result, circuit, and diagnostic storage.
//
// @result: Caller-owned result to destroy, or NULL; do not access it afterward.
// @returns: None.
//------------------------------------------------------------------------------
void lemma_frontend_result_destroy(lemma_frontend_result *result);

#endif
