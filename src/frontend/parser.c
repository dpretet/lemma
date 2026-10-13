#include "lemma/parser.h"

#include <stdint.h>

#include <tree_sitter/tree-sitter-systemverilog.h>

TSTree *lemma_parse_source(const char *source, size_t length)
{
    if (source == NULL || length > UINT32_MAX) {
        return NULL;
    }

    TSParser *parser = ts_parser_new();
    if (parser == NULL) {
        return NULL;
    }

    if (!ts_parser_set_language(parser, tree_sitter_systemverilog())) {
        ts_parser_delete(parser);
        return NULL;
    }

    TSTree *tree = ts_parser_parse_string(parser, NULL, source, (uint32_t)length);
    ts_parser_delete(parser);
    return tree;
}
