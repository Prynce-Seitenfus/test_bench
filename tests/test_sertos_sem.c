/**
 * @file test_sertos_sem.c
 * @brief Unit tests for SertOS Counting and Binary Semaphore Synchronization Primitives.
 */

#include "unity.h"
#include "sertos_sem.h"
#include "sertos_scheduler.h"
#include "sertos_task.h"
#include "memory_pool.h"
#include <string.h>

static SertosSemaphore s_sem;
static SertosSemHandle s_sem_handle;
static uint8_t s_dummy_stack[512] __attribute__((aligned(8)));
static SertosTaskControlBlock s_dummy_tcb;
static SertosTaskHandle s_dummy_handle;

static void dummy_entry(void* param)
{
    (void)param;
}

void setUp(void)
{
    memory_pool_init();
    (void)sertos_scheduler_init();
    (void)memset(&s_sem, 0, sizeof(s_sem));
    s_sem_handle = NULL;

    SertosTaskConfig cfg;
    cfg.name = "SemWorker";
    cfg.entry_func = dummy_entry;
    cfg.param = NULL;
    cfg.priority = 4U;
    cfg.stack_buffer = s_dummy_stack;
    cfg.stack_size = sizeof(s_dummy_stack);
    (void)sertos_task_create_static(&cfg, &s_dummy_tcb, &s_dummy_handle);
    sertos_scheduler_set_current_tcb(&s_dummy_tcb);
    s_dummy_tcb.state = SERTOS_TASK_STATE_RUNNING;
}

void tearDown(void)
{
}

void test_sem_create_counting_static_validation(void)
{
    SertosStatus status;

    /* NULL sem */
    status = sertos_sem_create_counting_static(NULL, 1U, 5U, &s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    /* NULL out_handle */
    status = sertos_sem_create_counting_static(&s_sem, 1U, 5U, NULL);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    /* max_count == 0 */
    status = sertos_sem_create_counting_static(&s_sem, 0U, 0U, &s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);

    /* initial > max */
    status = sertos_sem_create_counting_static(&s_sem, 6U, 5U, &s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
}

void test_sem_counting_take_give_lifecycle(void)
{
    SertosStatus status;

    status = sertos_sem_create_counting_static(&s_sem, 2U, 3U, &s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(2U, (uint32_t)sertos_sem_get_count(s_sem_handle));

    /* Take 1 */
    status = sertos_sem_take(s_sem_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)sertos_sem_get_count(s_sem_handle));

    /* Take 2 */
    status = sertos_sem_take(s_sem_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_sem_get_count(s_sem_handle));

    /* Take on empty with NO_WAIT returns TIMEOUT */
    status = sertos_sem_take(s_sem_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_TIMEOUT, status);

    /* Give 1, 2, 3 */
    status = sertos_sem_give(s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    status = sertos_sem_give(s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    status = sertos_sem_give(s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(3U, (uint32_t)sertos_sem_get_count(s_sem_handle));

    /* Give beyond max_count returns RESOURCE_BUSY */
    status = sertos_sem_give(s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_RESOURCE_BUSY, status);
}

void test_sem_binary_static_behavior(void)
{
    SertosStatus status;

    /* Initially available */
    status = sertos_sem_create_binary_static(&s_sem, true, &s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)sertos_sem_get_count(s_sem_handle));

    /* Take */
    status = sertos_sem_take(s_sem_handle, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_sem_get_count(s_sem_handle));

    /* Give back */
    status = sertos_sem_give(s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)sertos_sem_get_count(s_sem_handle));

    /* Exceeding binary capacity fails */
    status = sertos_sem_give(s_sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_RESOURCE_BUSY, status);
}

void test_sem_dynamic_allocation(void)
{
    SertosSemHandle dyn_sem = NULL;
    SertosStatus status;

    status = sertos_sem_create_counting(1U, 5U, &dyn_sem);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NOT_NULL(dyn_sem);
    TEST_ASSERT_FALSE(dyn_sem->is_statically_allocated);

    status = sertos_sem_delete(dyn_sem);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* Binary dynamic */
    status = sertos_sem_create_binary(false, &dyn_sem);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_sem_get_count(dyn_sem));
    (void)sertos_sem_delete(dyn_sem);
}

void test_sem_isr_signaling(void)
{
    SertosStatus status;
    bool higher_woken = false;

    (void)sertos_sem_create_binary_static(&s_sem, false, &s_sem_handle);

    status = sertos_sem_give_from_isr(s_sem_handle, &higher_woken);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)sertos_sem_get_count(s_sem_handle));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sem_create_counting_static_validation);
    RUN_TEST(test_sem_counting_take_give_lifecycle);
    RUN_TEST(test_sem_binary_static_behavior);
    RUN_TEST(test_sem_dynamic_allocation);
    RUN_TEST(test_sem_isr_signaling);
    return UNITY_END();
}
