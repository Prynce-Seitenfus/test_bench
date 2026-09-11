/**
 * @file test_sertos_mutex.c
 * @brief Unit tests for SertOS Mutex and Priority Inheritance Protocol (PIP).
 */

#include "unity.h"
#include "sertos_mutex.h"
#include "sertos_scheduler.h"
#include "sertos_task.h"
#include "memory_pool.h"
#include <string.h>

static SertosMutex s_mutex;
static SertosMutexHandle s_mutex_handle;

static uint8_t s_stack_low[512] __attribute__((aligned(8)));
static uint8_t s_stack_high[512] __attribute__((aligned(8)));
static SertosTaskControlBlock s_tcb_low;
static SertosTaskControlBlock s_tcb_high;
static SertosTaskHandle s_handle_low;
static SertosTaskHandle s_handle_high;

static void worker_entry(void* param)
{
    (void)param;
}

void setUp(void)
{
    SertosTaskConfig cfg;

    memory_pool_init();
    (void)sertos_scheduler_init();
    (void)memset(&s_mutex, 0, sizeof(s_mutex));
    s_mutex_handle = NULL;

    cfg.name = "LowWorker";
    cfg.entry_func = worker_entry;
    cfg.param = NULL;
    cfg.priority = 2U;
    cfg.stack_buffer = s_stack_low;
    cfg.stack_size = sizeof(s_stack_low);
    (void)sertos_task_create_static(&cfg, &s_tcb_low, &s_handle_low);

    cfg.name = "HighWorker";
    cfg.entry_func = worker_entry;
    cfg.param = NULL;
    cfg.priority = 10U;
    cfg.stack_buffer = s_stack_high;
    cfg.stack_size = sizeof(s_stack_high);
    (void)sertos_task_create_static(&cfg, &s_tcb_high, &s_handle_high);
}

void tearDown(void)
{
}

void test_mutex_create_static_and_dynamic_validation(void)
{
    SertosStatus status;
    SertosMutexHandle dyn_mutex = NULL;

    status = sertos_mutex_create_static(NULL, &s_mutex_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    status = sertos_mutex_create_static(&s_mutex, NULL);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    status = sertos_mutex_create_static(&s_mutex, &s_mutex_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NULL(sertos_mutex_get_owner(s_mutex_handle));

    /* Dynamic allocation */
    status = sertos_mutex_create(&dyn_mutex);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NOT_NULL(dyn_mutex);
    status = sertos_mutex_delete(dyn_mutex);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
}

void test_mutex_lock_and_unlock_basic(void)
{
    SertosStatus status;

    (void)sertos_mutex_create_static(&s_mutex, &s_mutex_handle);
    sertos_scheduler_set_current_tcb(&s_tcb_low);
    s_tcb_low.state = SERTOS_TASK_STATE_RUNNING;

    /* Lock */
    status = sertos_mutex_lock(s_mutex_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_PTR(&s_tcb_low, sertos_mutex_get_owner(s_mutex_handle));

    /* Unlock */
    status = sertos_mutex_unlock(s_mutex_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NULL(sertos_mutex_get_owner(s_mutex_handle));
}

void test_mutex_recursive_locking(void)
{
    SertosStatus status;

    (void)sertos_mutex_create_static(&s_mutex, &s_mutex_handle);
    sertos_scheduler_set_current_tcb(&s_tcb_low);
    s_tcb_low.state = SERTOS_TASK_STATE_RUNNING;

    /* First lock */
    status = sertos_mutex_lock(s_mutex_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, s_mutex.lock_count);

    /* Second recursive lock */
    status = sertos_mutex_lock(s_mutex_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(2U, s_mutex.lock_count);

    /* First unlock: still held */
    status = sertos_mutex_unlock(s_mutex_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, s_mutex.lock_count);
    TEST_ASSERT_EQUAL_PTR(&s_tcb_low, sertos_mutex_get_owner(s_mutex_handle));

    /* Second unlock: released */
    status = sertos_mutex_unlock(s_mutex_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(0U, s_mutex.lock_count);
    TEST_ASSERT_NULL(sertos_mutex_get_owner(s_mutex_handle));
}

void test_mutex_unlock_by_non_owner_fails(void)
{
    SertosStatus status;

    (void)sertos_mutex_create_static(&s_mutex, &s_mutex_handle);
    sertos_scheduler_set_current_tcb(&s_tcb_low);
    s_tcb_low.state = SERTOS_TASK_STATE_RUNNING;
    (void)sertos_mutex_lock(s_mutex_handle, SERTOS_NO_WAIT);

    /* HighWorker tries to unlock mutex owned by LowWorker */
    sertos_scheduler_set_current_tcb(&s_tcb_high);
    s_tcb_high.state = SERTOS_TASK_STATE_RUNNING;

    status = sertos_mutex_unlock(s_mutex_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_PTR(&s_tcb_low, sertos_mutex_get_owner(s_mutex_handle));
}

void test_mutex_priority_inheritance_protocol(void)
{
    SertosStatus status;

    (void)sertos_mutex_create_static(&s_mutex, &s_mutex_handle);

    /* LowWorker (priority 2) acquires mutex */
    sertos_scheduler_set_current_tcb(&s_tcb_low);
    s_tcb_low.state = SERTOS_TASK_STATE_RUNNING;
    (void)sertos_mutex_lock(s_mutex_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT8(2U, s_tcb_low.priority);

    /* HighWorker (priority 10) attempts to lock with NO_WAIT */
    sertos_scheduler_set_current_tcb(&s_tcb_high);
    s_tcb_high.state = SERTOS_TASK_STATE_RUNNING;
    status = sertos_mutex_lock(s_mutex_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_TIMEOUT, status);

    /* Priority Inheritance: LowWorker priority must be boosted to 10 */
    TEST_ASSERT_EQUAL_UINT8(10U, s_tcb_low.priority);

    /* LowWorker runs and releases the lock */
    sertos_scheduler_set_current_tcb(&s_tcb_low);
    s_tcb_low.state = SERTOS_TASK_STATE_RUNNING;
    status = sertos_mutex_unlock(s_mutex_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* LowWorker priority must be restored back to base priority 2 */
    TEST_ASSERT_EQUAL_UINT8(2U, s_tcb_low.priority);
    TEST_ASSERT_NULL(sertos_mutex_get_owner(s_mutex_handle));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mutex_create_static_and_dynamic_validation);
    RUN_TEST(test_mutex_lock_and_unlock_basic);
    RUN_TEST(test_mutex_recursive_locking);
    RUN_TEST(test_mutex_unlock_by_non_owner_fails);
    RUN_TEST(test_mutex_priority_inheritance_protocol);
    return UNITY_END();
}
