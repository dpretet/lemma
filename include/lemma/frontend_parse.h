#ifndef LEMMA_FRONTEND_PARSE_H
#define LEMMA_FRONTEND_PARSE_H

#include "frontend_types.h"

#include <stddef.h>

typedef struct lemma_parsed_source lemma_parsed_source;

//--------------------------------------------------------
// Parse source bytes into a caller-owned handle; the buffer remains borrowed
// until that handle is destroyed.
//
// @source: Non-NULL source buffer with at least length readable bytes; borrowed
//     until the returned handle is destroyed.
// @length: Number of source bytes to parse; must not exceed UINT32_MAX.
// @diagnostic: Optional diagnostic output, cleared on entry when provided.
// @returns: Caller-owned parsed-source handle, or NULL for invalid input,
//     syntax or parser failure, or allocation failure.
//--------------------------------------------------------
lemma_parsed_source *lemma_frontend_parse(const char *source, size_t length,
                                          lemma_frontend_diagnostic *diagnostic);

//--------------------------------------------------------
// Destroy a parsed-source handle.
//
// @parsed: Parsed-source handle to destroy, or NULL.
// @returns: None.
//--------------------------------------------------------
void lemma_parsed_source_destroy(lemma_parsed_source *parsed);

#endif
