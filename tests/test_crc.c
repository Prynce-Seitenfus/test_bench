#include "unity.h"
#include "crc.h"
#include <stdint.h>
#include <stddef.h>

void setUp(void)
{
    /* Global setup, empty as no persistent state needs cleanup */
}

void tearDown(void)
{
    /* Global teardown, empty as no persistent state needs cleanup */
}

static const uint8_t check_vector[] = {
    (uint8_t)'1', (uint8_t)'2', (uint8_t)'3', (uint8_t)'4', (uint8_t)'5',
    (uint8_t)'6', (uint8_t)'7', (uint8_t)'8', (uint8_t)'9'
};

static void test_crc8_standard_vector(void)
{
    uint8_t result = crc_8_calculate(check_vector, sizeof(check_vector));
    TEST_ASSERT_EQUAL_HEX8(0xF4U, result);
}

static void test_crc8_streaming_accumulation(void)
{
    uint8_t seed = CRC8_INITIAL_SEED;

    /* Update in chunks: 4 bytes, then 5 bytes */
    seed = crc_8_update(seed, &check_vector[0], 4U);
    seed = crc_8_update(seed, &check_vector[4], 5U);

    TEST_ASSERT_EQUAL_HEX8(0xF4U, seed);
}

static void test_crc8_null_and_empty(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x12U, crc_8_update(0x12U, NULL, 5U));
    TEST_ASSERT_EQUAL_HEX8(0x34U, crc_8_update(0x34U, check_vector, 0U));
    TEST_ASSERT_EQUAL_HEX8(CRC8_INITIAL_SEED, crc_8_calculate(NULL, 10U));
}

static void test_crc8_error_detection(void)
{
    uint8_t corrupted[sizeof(check_vector)];
    for (size_t i = 0U; i < sizeof(check_vector); ++i) {
        corrupted[i] = check_vector[i];
    }

    uint8_t original_crc = crc_8_calculate(check_vector, sizeof(check_vector));

    /* Flip a single bit */
    corrupted[4] ^= 0x01U;
    uint8_t corrupted_crc = crc_8_calculate(corrupted, sizeof(corrupted));

    TEST_ASSERT_NOT_EQUAL(original_crc, corrupted_crc);
}

static void test_crc16_standard_vector(void)
{
    uint16_t result = crc_16_calculate(check_vector, sizeof(check_vector));
    TEST_ASSERT_EQUAL_HEX16(0x29B1U, result);
}

static void test_crc16_streaming_accumulation(void)
{
    uint16_t seed = CRC16_INITIAL_SEED;

    /* Process byte-by-byte */
    for (size_t i = 0U; i < sizeof(check_vector); ++i) {
        seed = crc_16_update(seed, &check_vector[i], 1U);
    }

    TEST_ASSERT_EQUAL_HEX16(0x29B1U, seed);
}

static void test_crc16_null_and_empty(void)
{
    TEST_ASSERT_EQUAL_HEX16(0xABCDU, crc_16_update(0xABCDU, NULL, 5U));
    TEST_ASSERT_EQUAL_HEX16(0x1234U, crc_16_update(0x1234U, check_vector, 0U));
    TEST_ASSERT_EQUAL_HEX16(CRC16_INITIAL_SEED, crc_16_calculate(NULL, 10U));
}

static void test_crc16_error_detection(void)
{
    uint8_t corrupted[sizeof(check_vector)];
    for (size_t i = 0U; i < sizeof(check_vector); ++i) {
        corrupted[i] = check_vector[i];
    }

    uint16_t original_crc = crc_16_calculate(check_vector, sizeof(check_vector));

    corrupted[0] ^= 0x80U;
    uint16_t corrupted_crc = crc_16_calculate(corrupted, sizeof(corrupted));

    TEST_ASSERT_NOT_EQUAL(original_crc, corrupted_crc);
}

static void test_crc32_standard_vector(void)
{
    uint32_t result = crc_32_calculate(check_vector, sizeof(check_vector));
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926U, result);
}

static void test_crc32_streaming_accumulation(void)
{
    uint32_t seed = CRC32_INITIAL_SEED;

    /* Accumulate in two slices */
    seed = crc_32_update(seed, &check_vector[0], 3U);
    seed = crc_32_update(seed, &check_vector[3], 6U);

    uint32_t result = seed ^ 0xFFFFFFFFU;
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926U, result);
}

static void test_crc32_null_and_empty(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEFU, crc_32_update(0xDEADBEEFU, NULL, 5U));
    TEST_ASSERT_EQUAL_HEX32(0xCAFEBABEU, crc_32_update(0xCAFEBABEU, check_vector, 0U));
    TEST_ASSERT_EQUAL_HEX32(0x00000000U, crc_32_calculate(NULL, 10U));
}

static void test_crc32_error_detection(void)
{
    uint8_t corrupted[sizeof(check_vector)];
    for (size_t i = 0U; i < sizeof(check_vector); ++i) {
        corrupted[i] = check_vector[i];
    }

    uint32_t original_crc = crc_32_calculate(check_vector, sizeof(check_vector));

    corrupted[8] ^= 0x02U;
    uint32_t corrupted_crc = crc_32_calculate(corrupted, sizeof(corrupted));

    TEST_ASSERT_NOT_EQUAL(original_crc, corrupted_crc);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_crc8_standard_vector);
    RUN_TEST(test_crc8_streaming_accumulation);
    RUN_TEST(test_crc8_null_and_empty);
    RUN_TEST(test_crc8_error_detection);

    RUN_TEST(test_crc16_standard_vector);
    RUN_TEST(test_crc16_streaming_accumulation);
    RUN_TEST(test_crc16_null_and_empty);
    RUN_TEST(test_crc16_error_detection);

    RUN_TEST(test_crc32_standard_vector);
    RUN_TEST(test_crc32_streaming_accumulation);
    RUN_TEST(test_crc32_null_and_empty);
    RUN_TEST(test_crc32_error_detection);

    return UNITY_END();
}
