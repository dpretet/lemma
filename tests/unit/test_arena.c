#include <stdint.h>

#include "arena.h"
#include "unity.h"

void test_arena_grows_without_invalidating_prior_allocations(void)
{
    Arena arena = {0};
    uint64_t *first = arena_alloc(&arena, sizeof(*first));
    TEST_ASSERT_NOT_NULL(first);
    *first = UINT64_C(0x123456789abcdef0);

    size_t growth_size = (ARENA_REGION_DEFAULT_CAPACITY + 1U) * sizeof(uintptr_t);
    uint8_t *larger = arena_alloc(&arena, growth_size);
    TEST_ASSERT_NOT_NULL(larger);
    larger[0] = UINT8_C(0x5a);

    TEST_ASSERT_EQUAL_UINT64(UINT64_C(0x123456789abcdef0), *first);
    TEST_ASSERT_EQUAL_UINT8(UINT8_C(0x5a), larger[0]);

    arena_reset(&arena);
    uint8_t *reused = arena_alloc(&arena, sizeof(*reused));
    TEST_ASSERT_NOT_NULL(reused);
    arena_free(&arena);
}
