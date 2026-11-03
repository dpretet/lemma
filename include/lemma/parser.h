#ifndef LEMMA_PARSER_H
#define LEMMA_PARSER_H

#include <stddef.h>

#include <tree_sitter/api.h>

/*
 * Parse length bytes from a non-NULL source buffer borrowed for this call;
 * length must not exceed UINT32_MAX. Return a tree owned by the caller, or NULL
 * if input validation, parser setup, or parsing fails. Syntax errors may be
 * represented in the returned tree and must be checked by the caller.
 */
TSTree *lemma_parse_source(const char *source, size_t length);

#endif
