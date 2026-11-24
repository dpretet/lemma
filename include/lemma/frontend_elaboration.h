#ifndef LEMMA_FRONTEND_ELABORATION_H
#define LEMMA_FRONTEND_ELABORATION_H

#include "circuit_ir.h"
#include "frontend_parse.h"

//--------------------------------------------------------------------------------
// Validate and elaborate the sole module into fresh builder-owned IR storage.
// No separate owning elaborated-design object is created.
//
// @parsed: Successfully parsed, live non-NULL handle borrowed for the call.
// @builder: Non-NULL fresh caller-owned builder to receive the final IR storage.
// @diagnostic: Optional diagnostic output, cleared on entry when provided.
// @returns: Nonzero on success; zero for unsupported RTL or an operation failure;
//     destroy, rather than finalize, the builder on failure.
//--------------------------------------------------------------------------------
int lemma_frontend_elaborate(const lemma_parsed_source *parsed,
                             lemma_circuit_builder *builder,
                             lemma_frontend_diagnostic *diagnostic);

#endif
