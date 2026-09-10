#include "unity.h"
#include "bitmap.h"
#include <stdint.h>
#include <stdbool.h>

void setUp(void)
{
    /* Global setup, empty as each test initializes its own bitmap */
}

void tearDown(void)
{
    /* Global teardown, empty as no persistent state needs cleanup */
}

static void test_bitmap_init_success(void)
{
    Bitmap bm;
    BitmapWord storage[BITMAP_BITS_TO_WORDS(64U)];

    TEST_ASSERT_TRUE(bitmap_init(&bm, storage, 64U));
    TEST_ASSERT_EQUAL_UINT(64U, bitmap_bit_count(&bm));
    TEST_ASSERT_TRUE(bitmap_is_empty(&bm));
    TEST_ASSERT_FALSE(bitmap_is_full(&bm));
    TEST_ASSERT_EQUAL_UINT(0U, bitmap_count_set(&bm));
}

static void test_bitmap_init_invalid_params(void)
{
    Bitmap bm;
    BitmapWord storage[1];

    TEST_ASSERT_FALSE(bitmap_init(NULL, storage, 32U));
    TEST_ASSERT_FALSE(bitmap_init(&bm, NULL, 32U));
    TEST_ASSERT_FALSE(bitmap_init(&bm, storage, 0U));
}

static void test_bitmap_set_clear_test_bit(void)
{
    Bitmap bm;
    BitmapWord storage[BITMAP_BITS_TO_WORDS(64U)];

    TEST_ASSERT_TRUE(bitmap_init(&bm, storage, 64U));

    /* NULL and boundary checks */
    TEST_ASSERT_FALSE(bitmap_set_bit(NULL, 0U));
    TEST_ASSERT_FALSE(bitmap_set_bit(&bm, 64U)); /* Out of bounds */
    TEST_ASSERT_FALSE(bitmap_clear_bit(NULL, 0U));
    TEST_ASSERT_FALSE(bitmap_clear_bit(&bm, 64U));
    TEST_ASSERT_FALSE(bitmap_test_bit(NULL, 0U));
    TEST_ASSERT_FALSE(bitmap_test_bit(&bm, 64U));

    /* Set bits across word boundary */
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 0U));
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 31U));
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 32U));
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 63U));

    TEST_ASSERT_TRUE(bitmap_test_bit(&bm, 0U));
    TEST_ASSERT_TRUE(bitmap_test_bit(&bm, 31U));
    TEST_ASSERT_TRUE(bitmap_test_bit(&bm, 32U));
    TEST_ASSERT_TRUE(bitmap_test_bit(&bm, 63U));

    /* Unset bits must be false */
    TEST_ASSERT_FALSE(bitmap_test_bit(&bm, 1U));
    TEST_ASSERT_FALSE(bitmap_test_bit(&bm, 30U));
    TEST_ASSERT_FALSE(bitmap_test_bit(&bm, 33U));

    TEST_ASSERT_EQUAL_UINT(4U, bitmap_count_set(&bm));

    /* Clear bits */
    TEST_ASSERT_TRUE(bitmap_clear_bit(&bm, 0U));
    TEST_ASSERT_FALSE(bitmap_test_bit(&bm, 0U));
    TEST_ASSERT_EQUAL_UINT(3U, bitmap_count_set(&bm));
}

static void test_bitmap_toggle_bit(void)
{
    Bitmap bm;
    BitmapWord storage[1];

    TEST_ASSERT_TRUE(bitmap_init(&bm, storage, 32U));

    /* Toggle 0 -> 1 */
    TEST_ASSERT_TRUE(bitmap_toggle_bit(&bm, 15U));
    TEST_ASSERT_TRUE(bitmap_test_bit(&bm, 15U));

    /* Toggle 1 -> 0 */
    TEST_ASSERT_TRUE(bitmap_toggle_bit(&bm, 15U));
    TEST_ASSERT_FALSE(bitmap_test_bit(&bm, 15U));

    /* Invalid index */
    TEST_ASSERT_FALSE(bitmap_toggle_bit(&bm, 32U));
    TEST_ASSERT_FALSE(bitmap_toggle_bit(NULL, 0U));
}

static void test_bitmap_set_all_and_clear_all(void)
{
    Bitmap bm;
    /* 50 bits: Word 0 (32 bits), Word 1 (18 bits) */
    BitmapWord storage[BITMAP_BITS_TO_WORDS(50U)];

    TEST_ASSERT_TRUE(bitmap_init(&bm, storage, 50U));

    bitmap_set_all(&bm);
    TEST_ASSERT_TRUE(bitmap_is_full(&bm));
    TEST_ASSERT_FALSE(bitmap_is_empty(&bm));
    TEST_ASSERT_EQUAL_UINT(50U, bitmap_count_set(&bm));

    /* Verify trailing ghost bits in word 1 remain 0 */
    BitmapWord expected_mask = ((BitmapWord)1U << 18U) - 1U;
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, storage[0]);
    TEST_ASSERT_EQUAL_HEX32(expected_mask, storage[1]);

    bitmap_clear_all(&bm);
    TEST_ASSERT_TRUE(bitmap_is_empty(&bm));
    TEST_ASSERT_FALSE(bitmap_is_full(&bm));
    TEST_ASSERT_EQUAL_UINT(0U, bitmap_count_set(&bm));
}

static void test_bitmap_find_first_set(void)
{
    Bitmap bm;
    BitmapWord storage[BITMAP_BITS_TO_WORDS(64U)];
    size_t found_bit = 0U;

    TEST_ASSERT_TRUE(bitmap_init(&bm, storage, 64U));

    /* Empty bitmap */
    TEST_ASSERT_FALSE(bitmap_find_first_set(&bm, &found_bit));

    /* Set bit 20 */
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 20U));
    TEST_ASSERT_TRUE(bitmap_find_first_set(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(20U, found_bit);

    /* Set bit 5 (lower index) */
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 5U));
    TEST_ASSERT_TRUE(bitmap_find_first_set(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(5U, found_bit);

    /* Set bit 0 */
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 0U));
    TEST_ASSERT_TRUE(bitmap_find_first_set(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(0U, found_bit);

    /* NULL check */
    TEST_ASSERT_FALSE(bitmap_find_first_set(NULL, &found_bit));
    TEST_ASSERT_FALSE(bitmap_find_first_set(&bm, NULL));
}

static void test_bitmap_find_last_set(void)
{
    Bitmap bm;
    /* 50 bits */
    BitmapWord storage[BITMAP_BITS_TO_WORDS(50U)];
    size_t found_bit = 0U;

    TEST_ASSERT_TRUE(bitmap_init(&bm, storage, 50U));

    /* Empty bitmap */
    TEST_ASSERT_FALSE(bitmap_find_last_set(&bm, &found_bit));

    /* Set bit 5 */
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 5U));
    TEST_ASSERT_TRUE(bitmap_find_last_set(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(5U, found_bit);

    /* Set bit 31 (end of word 0) */
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 31U));
    TEST_ASSERT_TRUE(bitmap_find_last_set(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(31U, found_bit);

    /* Set bit 49 (last valid bit of 50-bit bitmap in word 1) */
    TEST_ASSERT_TRUE(bitmap_set_bit(&bm, 49U));
    TEST_ASSERT_TRUE(bitmap_find_last_set(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(49U, found_bit);

    /* NULL check */
    TEST_ASSERT_FALSE(bitmap_find_last_set(NULL, &found_bit));
    TEST_ASSERT_FALSE(bitmap_find_last_set(&bm, NULL));
}

static void test_bitmap_find_first_and_last_zero(void)
{
    Bitmap bm;
    BitmapWord storage[BITMAP_BITS_TO_WORDS(40U)];
    size_t found_bit = 0U;

    TEST_ASSERT_TRUE(bitmap_init(&bm, storage, 40U));

    /* In empty bitmap, first zero is bit 0, last zero is bit 39 */
    TEST_ASSERT_TRUE(bitmap_find_first_zero(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(0U, found_bit);

    TEST_ASSERT_TRUE(bitmap_find_last_zero(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(39U, found_bit);

    /* Fill all bits */
    bitmap_set_all(&bm);
    TEST_ASSERT_FALSE(bitmap_find_first_zero(&bm, &found_bit));
    TEST_ASSERT_FALSE(bitmap_find_last_zero(&bm, &found_bit));

    /* Clear bits 10 and 35 */
    TEST_ASSERT_TRUE(bitmap_clear_bit(&bm, 10U));
    TEST_ASSERT_TRUE(bitmap_clear_bit(&bm, 35U));

    TEST_ASSERT_TRUE(bitmap_find_first_zero(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(10U, found_bit);

    TEST_ASSERT_TRUE(bitmap_find_last_zero(&bm, &found_bit));
    TEST_ASSERT_EQUAL_UINT(35U, found_bit);

    /* NULL checks */
    TEST_ASSERT_FALSE(bitmap_find_first_zero(NULL, &found_bit));
    TEST_ASSERT_FALSE(bitmap_find_first_zero(&bm, NULL));
    TEST_ASSERT_FALSE(bitmap_find_last_zero(NULL, &found_bit));
    TEST_ASSERT_FALSE(bitmap_find_last_zero(&bm, NULL));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_bitmap_init_success);
    RUN_TEST(test_bitmap_init_invalid_params);
    RUN_TEST(test_bitmap_set_clear_test_bit);
    RUN_TEST(test_bitmap_toggle_bit);
    RUN_TEST(test_bitmap_set_all_and_clear_all);
    RUN_TEST(test_bitmap_find_first_set);
    RUN_TEST(test_bitmap_find_last_set);
    RUN_TEST(test_bitmap_find_first_and_last_zero);

    return UNITY_END();
}
