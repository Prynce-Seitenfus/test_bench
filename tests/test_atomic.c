#include "unity.h"
#include "atomic.h"
#include <stdint.h>
#include <stdbool.h>

void setUp(void)
{
    /* Global test setup, empty as each test case manages its own atomic variables */
}

void tearDown(void)
{
    /* Global test teardown, empty as no persistent state needs cleanup */
}

static void test_atomic_init_valid(void)
{
    atomic_size_t obj;

    atomic_init_size_t(&obj, 0U);
    TEST_ASSERT_EQUAL_UINT(0U, obj.value);

    atomic_init_size_t(&obj, 42U);
    TEST_ASSERT_EQUAL_UINT(42U, obj.value);

    atomic_init_size_t(&obj, SIZE_MAX);
    TEST_ASSERT_EQUAL_UINT(SIZE_MAX, obj.value);
}

static void test_atomic_init_null(void)
{
    /* Null pointer should be handled gracefully without crashing */
    atomic_init_size_t(NULL, 100U);
}

static void test_atomic_store_and_load(void)
{
    atomic_size_t obj;

    atomic_init_size_t(&obj, 0U);
    TEST_ASSERT_EQUAL_UINT(0U, atomic_load_acquire(&obj));

    atomic_store_release(&obj, 12345U);
    TEST_ASSERT_EQUAL_UINT(12345U, atomic_load_acquire(&obj));

    atomic_store_release(&obj, 0xABCDEF01U);
    TEST_ASSERT_EQUAL_UINT(0xABCDEF01U, atomic_load_acquire(&obj));

    atomic_store_release(&obj, SIZE_MAX);
    TEST_ASSERT_EQUAL_UINT(SIZE_MAX, atomic_load_acquire(&obj));
}

static void test_atomic_store_null(void)
{
    /* Null pointer should be handled gracefully without crashing */
    atomic_store_release(NULL, 555U);
}

static void test_atomic_load_null(void)
{
    /* Null pointer must return 0U */
    TEST_ASSERT_EQUAL_UINT(0U, atomic_load_acquire(NULL));
}

static void test_atomic_fences(void)
{
    /* Standalone fences should execute without exception */
    atomic_thread_fence_acquire();
    atomic_thread_fence_release();
    atomic_thread_fence_seq_cst();
}

static void test_atomic_relaxed(void)
{
    atomic_size_t obj;

    atomic_init_size_t(&obj, 0U);
    TEST_ASSERT_EQUAL_UINT(0U, atomic_load_relaxed(&obj));

    atomic_store_relaxed(&obj, 999U);
    TEST_ASSERT_EQUAL_UINT(999U, atomic_load_relaxed(&obj));

    /* Null pointer safety */
    atomic_store_relaxed(NULL, 123U);
    TEST_ASSERT_EQUAL_UINT(0U, atomic_load_relaxed(NULL));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_atomic_init_valid);
    RUN_TEST(test_atomic_init_null);
    RUN_TEST(test_atomic_store_and_load);
    RUN_TEST(test_atomic_store_null);
    RUN_TEST(test_atomic_load_null);
    RUN_TEST(test_atomic_relaxed);
    RUN_TEST(test_atomic_fences);

    return UNITY_END();
}
