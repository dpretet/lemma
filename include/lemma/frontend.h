#ifndef LEMMA_FRONTEND_H
#define LEMMA_FRONTEND_H

#include "circuit_ir.h"
#include "frontend_types.h"

#include <stddef.h>

//--------------------------------------------------------
// Parse, extract, validate, elaborate, and construct the sole-module circuit.
// The input buffer is borrowed only for this synchronous call.
//
// @source: Non-NULL source buffer with at least length readable bytes.
// @length: Number of source bytes; must not exceed UINT32_MAX.
// @diagnostic: Optional diagnostic output, cleared on entry when provided.
// @returns: Caller-owned circuit on success, or NULL with a diagnostic for parse,
//     RTL rejection, or allocation failure.
//--------------------------------------------------------
lemma_circuit *lemma_frontend_build(const char *source, size_t length,
                                    lemma_frontend_diagnostic *diagnostic);

#endif
