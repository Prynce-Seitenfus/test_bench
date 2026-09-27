/**
 * @file test_sertos_systick.c
 * @brief Unit tests for Cortex-M SysTick reload validation.
 */

#include "unity.h"
#include "cortex_m_systick.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_systick_reload_for_common_clock_rates(void)
{
    uint32_t reload_value = 0U;

    TEST_ASSERT_TRUE(cortex_m_systick(32000000U, 1000U, &reload_value));
    TEST_ASSERT_EQUAL_UINT32(31999U, reload_value);

    TEST_ASSERT_TRUE(cortex_m_systick(25000000U, 1000U, &reload_value));
    TEST_ASSERT_EQUAL_UINT32(24999U, reload_value);
}

void test_systick_reload_accepts_maximum_24_bit_count(void)
{
    uint32_t reload_value = 0U;

    TEST_ASSERT_TRUE(cortex_m_systick(0x01000000U, 1U, &reload_value));
    TEST_ASSERT_EQUAL_UINT32(0x00FFFFFFU, reload_value);
}

void test_systick_reload_rejects_invalid_clock_inputs(void)
{
    uint32_t reload_value = 123U;

    TEST_ASSERT_FALSE(cortex_m_systick(0U, 1000U, &reload_value));
    TEST_ASSERT_FALSE(cortex_m_systick(32000000U, 0U, &reload_value));
    TEST_ASSERT_FALSE(cortex_m_systick(999U, 1000U, &reload_value));
    TEST_ASSERT_FALSE(cortex_m_systick(0x01000001U, 1U, &reload_value));
    TEST_ASSERT_FALSE(cortex_m_systick(32000000U, 1000U, NULL));
    TEST_ASSERT_EQUAL_UINT32(123U, reload_value);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_systick_reload_for_common_clock_rates);
    RUN_TEST(test_systick_reload_accepts_maximum_24_bit_count);
    RUN_TEST(test_systick_reload_rejects_invalid_clock_inputs);
    return UNITY_END();
}
