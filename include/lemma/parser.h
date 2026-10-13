#ifndef LEMMA_PARSER_H
#define LEMMA_PARSER_H

#include <stddef.h>

#include <tree_sitter/api.h>

/* Parse SystemVerilog syntax. The caller owns the returned tree. */
TSTree *lemma_parse_source(const char *source, size_t length);

#endif
