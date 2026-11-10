#ifndef LEMMA_SOURCE_RANGE_H
#define LEMMA_SOURCE_RANGE_H

#include <stddef.h>

// Source coordinates use half-open byte offsets and zero-based rows and byte
// columns, matching Tree-sitter. The end byte offset and position are exclusive.
typedef struct lemma_source_range {
    size_t start_byte;
    size_t end_byte;
    size_t start_row;
    size_t start_column;
    size_t end_row;
    size_t end_column;
} lemma_source_range;

#endif
