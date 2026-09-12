/**
 * @file test_sertos_timer.c
 * @brief Unit tests for SertOS Monotonic Software Timers.
 */

#include "unity.h"
#include "sertos_timer.h"
#include "sertos_scheduler.h"
#include "memory_pool.h"
#include <string.h>

static SertosTimer s_timer;
static SertosTimerHandle s_timer_handle;
static volatile uint32_t s_callback_count = 0U;

static void test_timer_cb(SertosTimerHandle handle, void* param)
{
    (void)handle;
    (void)param;
    s_callback_count++;
}

static uint8_t s_test_mem_pool[64U * 1024U] __attribute__((aligned(8)));

void setUp(void)
{
    (void)memory_pool_init(s_test_mem_pool, sizeof(s_test_mem_pool));
    sertos_timer_init();
    (void)memset(&s_timer, 0, sizeof(s_timer));
    s_timer_handle = NULL;
    s_callback_count = 0U;
}

void tearDown(void)
{
}

void test_timer_create_validation(void)
{
    SertosTimerConfig cfg;
    SertosStatus status;
    SertosTimerHandle dyn_timer = NULL;

    cfg.name = "ValTimer";
    cfg.period = 5U;
    cfg.is_periodic = false;
    cfg.callback = test_timer_cb;
    cfg.param = NULL;

    /* NULL config */
    status = sertos_timer_create_static(NULL, &s_timer, &s_timer_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    /* Period == 0 */
    cfg.period = 0U;
    status = sertos_timer_create_static(&cfg, &s_timer, &s_timer_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
    cfg.period = 5U;

    /* NULL callback */
    cfg.callback = NULL;
    status = sertos_timer_create_static(&cfg, &s_timer, &s_timer_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
    cfg.callback = test_timer_cb;

    /* Dynamic creation */
    status = sertos_timer_create(&cfg, &dyn_timer);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NOT_NULL(dyn_timer);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_timer_delete(dyn_timer));
}

void test_timer_one_shot_execution(void)
{
    SertosTimerConfig cfg;
    SertosStatus status;

    cfg.name = "OneShot";
    cfg.period = 3U;
    cfg.is_periodic = false;
    cfg.callback = test_timer_cb;
    cfg.param = NULL;

    status = sertos_timer_create_static(&cfg, &s_timer, &s_timer_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_FALSE(sertos_timer_is_active(s_timer_handle));

    /* Start timer */
    status = sertos_timer_start(s_timer_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_TRUE(sertos_timer_is_active(s_timer_handle));

    /* Tick 1 */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(0U, s_callback_count);
    TEST_ASSERT_TRUE(sertos_timer_is_active(s_timer_handle));

    /* Tick 2 */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(0U, s_callback_count);

    /* Tick 3: Expiration */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(1U, s_callback_count);
    TEST_ASSERT_FALSE(sertos_timer_is_active(s_timer_handle));

    /* Subsequent ticks: no further calls */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(1U, s_callback_count);
}

void test_timer_periodic_execution(void)
{
    SertosTimerConfig cfg;
    SertosStatus status;

    cfg.name = "Periodic";
    cfg.period = 2U;
    cfg.is_periodic = true;
    cfg.callback = test_timer_cb;
    cfg.param = NULL;

    status = sertos_timer_create_static(&cfg, &s_timer, &s_timer_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    (void)sertos_timer_start(s_timer_handle);

    /* Tick 1 */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(0U, s_callback_count);

    /* Tick 2: Trigger 1 */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(1U, s_callback_count);
    TEST_ASSERT_TRUE(sertos_timer_is_active(s_timer_handle));

    /* Tick 3 */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(1U, s_callback_count);

    /* Tick 4: Trigger 2 */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(2U, s_callback_count);

    /* Stop timer */
    status = sertos_timer_stop(s_timer_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_FALSE(sertos_timer_is_active(s_timer_handle));

    /* Tick 5: Disarmed, no call */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(2U, s_callback_count);
}

void test_timer_change_period_and_reset(void)
{
    SertosTimerConfig cfg;
    SertosStatus status;

    cfg.name = "ChangePeriod";
    cfg.period = 10U;
    cfg.is_periodic = false;
    cfg.callback = test_timer_cb;
    cfg.param = NULL;

    (void)sertos_timer_create_static(&cfg, &s_timer, &s_timer_handle);
    (void)sertos_timer_start(s_timer_handle);

    /* Shorten period to 1 tick */
    status = sertos_timer_change_period(s_timer_handle, 1U);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* Tick 1 triggers immediately */
    sertos_timer_tick();
    TEST_ASSERT_EQUAL_UINT32(1U, s_callback_count);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_timer_create_validation);
    RUN_TEST(test_timer_one_shot_execution);
    RUN_TEST(test_timer_periodic_execution);
    RUN_TEST(test_timer_change_period_and_reset);
    return UNITY_END();
}
