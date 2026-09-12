#include "unity.h"
#include "memory_pool.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define TEST_POOL_SIZE (64U * 1024U)
static uint8_t s_test_pool_storage[TEST_POOL_SIZE] __attribute__((aligned(8)));

void setUp(void)
{
    /* Reset and initialize memory pool before each test case */
    (void)memory_pool_init(s_test_pool_storage, sizeof(s_test_pool_storage));
}

void tearDown(void)
{
    /* Global teardown, no persistent host OS cleanup needed */
}

static void test_memory_pool_init_null_or_invalid_size(void)
{
    uint8_t tiny_buf[8];
    TEST_ASSERT_FALSE(memory_pool_init(NULL, 1024U));
    TEST_ASSERT_FALSE(memory_pool_init(s_test_pool_storage, 0U));
    TEST_ASSERT_FALSE(memory_pool_init(tiny_buf, sizeof(tiny_buf)));
}

static void test_memory_pool_init_unaligned_buffer(void)
{
    uint8_t local_buf[1024] __attribute__((aligned(8)));
    TEST_ASSERT_TRUE(memory_pool_init(&local_buf[1], sizeof(local_buf) - 1U));

    void* ptr = memory_pool_malloc(32U);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQUAL_UINT(0U, ((uintptr_t)ptr) % 8U);
    memory_pool_free(ptr);
}

static void test_memory_pool_malloc_basic(void)
{
    void* ptr1 = memory_pool_malloc(64U);
    TEST_ASSERT_NOT_NULL(ptr1);

    /* Verify 8-byte alignment per MISRA Rule 11.3 */
    TEST_ASSERT_EQUAL_UINT(0U, ((uintptr_t)ptr1) % 8U);

    /* Write and verify pattern */
    uint8_t* byte_ptr = (uint8_t*)ptr1;
    (void)memset(byte_ptr, 0x5AU, 64U);
    for (size_t i = 0U; i < 64U; ++i) {
        TEST_ASSERT_EQUAL_UINT8(0x5AU, byte_ptr[i]);
    }

    memory_pool_free(ptr1);
}

static void test_memory_pool_malloc_zero_size(void)
{
    void* ptr = memory_pool_malloc(0U);
    TEST_ASSERT_NULL(ptr);
}

static void test_memory_pool_malloc_exhaustion(void)
{
    /* Attempt to allocate more memory than the pool capacity */
    void* ptr = memory_pool_malloc(TEST_POOL_SIZE + 1024U);
    TEST_ASSERT_NULL(ptr);
}

static void test_memory_pool_free_null(void)
{
    /* Freeing NULL should safely do nothing */
    memory_pool_free(NULL);
}

static void test_memory_pool_coalescing(void)
{
    /* Allocate three sequential blocks */
    void* p1 = memory_pool_malloc(128U);
    void* p2 = memory_pool_malloc(128U);
    void* p3 = memory_pool_malloc(128U);

    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);

    /* Free middle block first, then first block */
    memory_pool_free(p2);
    memory_pool_free(p1);

    /* Now allocate a block large enough that requires coalesced p1 + p2 */
    void* p_large = memory_pool_malloc(256U);
    TEST_ASSERT_NOT_NULL(p_large);

    memory_pool_free(p3);
    memory_pool_free(p_large);
}

static void test_memory_pool_calloc_valid(void)
{
    const size_t num_elements = 16U;
    const size_t element_size = sizeof(uint32_t);

    uint32_t* arr = (uint32_t*)memory_pool_calloc(num_elements, element_size);
    TEST_ASSERT_NOT_NULL(arr);

    /* Verify 8-byte alignment */
    TEST_ASSERT_EQUAL_UINT(0U, ((uintptr_t)arr) % 8U);

    /* Verify all memory is cleared to zero */
    for (size_t i = 0U; i < num_elements; ++i) {
        TEST_ASSERT_EQUAL_UINT32(0U, arr[i]);
    }

    memory_pool_free(arr);
}

static void test_memory_pool_calloc_invalid(void)
{
    /* Zero num */
    TEST_ASSERT_NULL(memory_pool_calloc(0U, 16U));

    /* Zero size */
    TEST_ASSERT_NULL(memory_pool_calloc(16U, 0U));

    /* Overflow calculation */
    TEST_ASSERT_NULL(memory_pool_calloc(SIZE_MAX, 2U));
}

static void test_memory_pool_realloc_null_ptr(void)
{
    /* When ptr is NULL, realloc behaves like malloc */
    void* ptr = memory_pool_realloc(NULL, 64U);
    TEST_ASSERT_NOT_NULL(ptr);

    memory_pool_free(ptr);
}

static void test_memory_pool_realloc_zero_size(void)
{
    void* ptr = memory_pool_malloc(64U);
    TEST_ASSERT_NOT_NULL(ptr);

    /* When size is 0, realloc frees memory and returns NULL */
    void* result = memory_pool_realloc(ptr, 0U);
    TEST_ASSERT_NULL(result);
}

static void test_memory_pool_realloc_shrink(void)
{
    void* ptr = memory_pool_malloc(128U);
    TEST_ASSERT_NOT_NULL(ptr);

    /* Realloc to smaller size should succeed, return same block, and split remainder */
    void* new_ptr = memory_pool_realloc(ptr, 64U);
    TEST_ASSERT_EQUAL_PTR(ptr, new_ptr);

    /* Verify split remainder block is available in the pool */
    void* extra = memory_pool_malloc(32U);
    TEST_ASSERT_NOT_NULL(extra);

    memory_pool_free(extra);
    memory_pool_free(new_ptr);
}

static void test_memory_pool_realloc_expand_with_copy(void)
{
    char* str = (char*)memory_pool_malloc(16U);
    TEST_ASSERT_NOT_NULL(str);
    (void)strcpy(str, "RTOS Kernel");

    /* Expand allocation to larger size */
    char* new_str = (char*)memory_pool_realloc(str, 128U);
    TEST_ASSERT_NOT_NULL(new_str);
    TEST_ASSERT_EQUAL_STRING("RTOS Kernel", new_str);

    memory_pool_free(new_str);
}

static void test_memory_pool_realloc_exhaustion(void)
{
    void* ptr = memory_pool_malloc(64U);
    TEST_ASSERT_NOT_NULL(ptr);

    /* Requesting size exceeding pool should fail and return NULL */
    void* result = memory_pool_realloc(ptr, TEST_POOL_SIZE + 1024U);
    TEST_ASSERT_NULL(result);

    memory_pool_free(ptr);
}

static void test_memory_pool_realloc_in_place_expansion(void)
{
    /* Allocate two adjacent blocks */
    void* p1 = memory_pool_malloc(64U);
    void* p2 = memory_pool_malloc(64U);
    void* p3 = memory_pool_malloc(64U);

    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_NULL(p3);

    /* Free p2, leaving a free block immediately following p1 */
    memory_pool_free(p2);

    /* Expanding p1 should coalesce in place into p2 without moving address */
    void* p1_expanded = memory_pool_realloc(p1, 100U);
    TEST_ASSERT_EQUAL_PTR(p1, p1_expanded);

    memory_pool_free(p1_expanded);
    memory_pool_free(p3);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_memory_pool_init_null_or_invalid_size);
    RUN_TEST(test_memory_pool_init_unaligned_buffer);
    RUN_TEST(test_memory_pool_malloc_basic);
    RUN_TEST(test_memory_pool_malloc_zero_size);
    RUN_TEST(test_memory_pool_malloc_exhaustion);
    RUN_TEST(test_memory_pool_free_null);
    RUN_TEST(test_memory_pool_coalescing);
    RUN_TEST(test_memory_pool_calloc_valid);
    RUN_TEST(test_memory_pool_calloc_invalid);
    RUN_TEST(test_memory_pool_realloc_null_ptr);
    RUN_TEST(test_memory_pool_realloc_zero_size);
    RUN_TEST(test_memory_pool_realloc_shrink);
    RUN_TEST(test_memory_pool_realloc_expand_with_copy);
    RUN_TEST(test_memory_pool_realloc_in_place_expansion);
    RUN_TEST(test_memory_pool_realloc_exhaustion);

    return UNITY_END();
}
