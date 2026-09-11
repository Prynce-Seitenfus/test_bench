/**
 * @file test_sertos_task.c
 * @brief Unit tests for SertOS Task Management and TCB lifecycle.
 *
 * Validates static and dynamic task creation, parameter checking,
 * stack painting, high-water mark detection, and state transitions.
 */

#include "unity.h"
#include "sertos_task.h"
#include "sertos_scheduler.h"
#include "memory_pool.h"
#include <string.h>

#define TEST_TASK_STACK_SIZE   (512U)

static uint8_t s_task_stack[TEST_TASK_STACK_SIZE] __attribute__((aligned(8)));
static SertosTaskControlBlock s_test_tcb;

static void dummy_task_entry(void* param)
{
    (void)param;
}

void setUp(void)
{
    memory_pool_init();
    (void)sertos_scheduler_init();
    (void)memset(s_task_stack, 0, sizeof(s_task_stack));
    (void)memset(&s_test_tcb, 0, sizeof(s_test_tcb));
}

void tearDown(void)
{
}

void test_task_create_static_null_validation(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle;
    SertosStatus status;

    cfg.name = "TestNull";
    cfg.entry_func = dummy_task_entry;
    cfg.param = NULL;
    cfg.priority = 1U;
    cfg.stack_buffer = s_task_stack;
    cfg.stack_size = sizeof(s_task_stack);

    /* NULL tcb */
    status = sertos_task_create_static(&cfg, NULL, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    /* NULL out_handle */
    status = sertos_task_create_static(&cfg, &s_test_tcb, NULL);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    /* NULL config */
    status = sertos_task_create_static(NULL, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
}

void test_task_create_static_invalid_parameters(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle;
    SertosStatus status;

    cfg.name = "TestInvalid";
    cfg.entry_func = dummy_task_entry;
    cfg.param = NULL;
    cfg.priority = 1U;
    cfg.stack_buffer = s_task_stack;
    cfg.stack_size = sizeof(s_task_stack);

    /* NULL entry function */
    cfg.entry_func = NULL;
    status = sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
    cfg.entry_func = dummy_task_entry;

    /* Invalid priority (>= SERTOS_CONFIG_MAX_PRIORITIES) */
    cfg.priority = SERTOS_CONFIG_MAX_PRIORITIES;
    status = sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
    cfg.priority = 1U;

    /* Stack size too small */
    cfg.stack_size = SERTOS_CONFIG_MINIMAL_STACK_SIZE - 1U;
    status = sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
    cfg.stack_size = sizeof(s_task_stack);

    /* NULL stack buffer */
    cfg.stack_buffer = NULL;
    status = sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
    cfg.stack_buffer = s_task_stack;

    /* Unaligned stack buffer */
    cfg.stack_buffer = (void*)((uintptr_t)s_task_stack + 1U);
    status = sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
}

void test_task_create_static_success_and_metadata(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle = NULL;
    SertosStatus status;

    cfg.name = "Worker1";
    cfg.entry_func = dummy_task_entry;
    cfg.param = (void*)0x1234U;
    cfg.priority = 3U;
    cfg.stack_buffer = s_task_stack;
    cfg.stack_size = sizeof(s_task_stack);

    status = sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_EQUAL_PTR(&s_test_tcb, handle);

    /* Verify state and metadata */
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_READY, sertos_task_get_state(handle));
    TEST_ASSERT_EQUAL_STRING("Worker1", sertos_task_get_name(handle));
    TEST_ASSERT_EQUAL_UINT8(3U, sertos_task_get_priority(handle));
    TEST_ASSERT_EQUAL_HEX32(SERTOS_TASK_MAGIC_WORD, s_test_tcb.magic);
    TEST_ASSERT_TRUE(s_test_tcb.is_statically_allocated);
}

void test_task_stack_painting_and_high_water_mark(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle = NULL;
    SertosStatus status;
    size_t high_water_mark;

    cfg.name = "StackCheck";
    cfg.entry_func = dummy_task_entry;
    cfg.param = NULL;
    cfg.priority = 2U;
    cfg.stack_buffer = s_task_stack;
    cfg.stack_size = sizeof(s_task_stack);

    status = sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* High water mark immediately after creation */
    high_water_mark = sertos_task_get_stack_high_water_mark(handle);
    TEST_ASSERT_TRUE(high_water_mark > 0U);
    TEST_ASSERT_TRUE(high_water_mark <= sizeof(s_task_stack));

    /* Simulate stack usage by overwriting bottom of stack with non-fill byte */
    s_task_stack[0] = 0xFFU;
    s_task_stack[1] = 0xFFU;
    high_water_mark = sertos_task_get_stack_high_water_mark(handle);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)high_water_mark);
}

void test_task_create_dynamic_from_memory_pool(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle = NULL;
    SertosStatus status;

    cfg.name = "DynTask";
    cfg.entry_func = dummy_task_entry;
    cfg.param = NULL;
    cfg.priority = 4U;
    cfg.stack_buffer = NULL; /* Requests memory_pool allocation */
    cfg.stack_size = 512U;

    status = sertos_task_create(&cfg, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NOT_NULL(handle);
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_READY, sertos_task_get_state(handle));
    TEST_ASSERT_FALSE(handle->is_statically_allocated);

    /* Delete dynamic task and verify cleanup */
    status = sertos_task_delete(handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
}

void test_task_priority_modification(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle = NULL;
    SertosStatus status;

    cfg.name = "PrioTask";
    cfg.entry_func = dummy_task_entry;
    cfg.param = NULL;
    cfg.priority = 2U;
    cfg.stack_buffer = s_task_stack;
    cfg.stack_size = sizeof(s_task_stack);

    (void)sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL_UINT8(2U, sertos_task_get_priority(handle));

    /* Change priority */
    status = sertos_task_set_priority(handle, 7U);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT8(7U, sertos_task_get_priority(handle));

    /* Reject invalid priority */
    status = sertos_task_set_priority(handle, SERTOS_CONFIG_MAX_PRIORITIES);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
}

void test_task_suspend_and_resume_lifecycle(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle = NULL;
    SertosStatus status;

    cfg.name = "SuspendTask";
    cfg.entry_func = dummy_task_entry;
    cfg.param = NULL;
    cfg.priority = 3U;
    cfg.stack_buffer = s_task_stack;
    cfg.stack_size = sizeof(s_task_stack);

    (void)sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_READY, sertos_task_get_state(handle));

    /* Suspend task */
    status = sertos_task_suspend(handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_SUSPENDED, sertos_task_get_state(handle));

    /* Redundant suspend fails */
    status = sertos_task_suspend(handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);

    /* Resume task */
    status = sertos_task_resume(handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_READY, sertos_task_get_state(handle));

    /* Redundant resume fails */
    status = sertos_task_resume(handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);
}

void test_task_invalid_handle_and_corruption(void)
{
    SertosTaskControlBlock corrupted_tcb;
    (void)memset(&corrupted_tcb, 0, sizeof(corrupted_tcb));

    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_TERMINATED, sertos_task_get_state(NULL));
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_TERMINATED, sertos_task_get_state(&corrupted_tcb));

    TEST_ASSERT_EQUAL_STRING("Unknown", sertos_task_get_name(NULL));
    TEST_ASSERT_EQUAL_STRING("Unknown", sertos_task_get_name(&corrupted_tcb));

    TEST_ASSERT_EQUAL_UINT8(0U, sertos_task_get_priority(NULL));
    TEST_ASSERT_EQUAL_UINT8(0U, sertos_task_get_priority(&corrupted_tcb));

    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_task_get_stack_high_water_mark(NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_task_get_stack_high_water_mark(&corrupted_tcb));

    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_task_delete(&corrupted_tcb));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_task_suspend(&corrupted_tcb));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_task_resume(NULL));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_task_resume(&corrupted_tcb));
}

void test_task_create_dynamic_exhaustion(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle = NULL;
    SertosStatus status;

    cfg.name = "HugeTask";
    cfg.entry_func = dummy_task_entry;
    cfg.param = NULL;
    cfg.priority = 1U;
    cfg.stack_buffer = NULL;
    cfg.stack_size = 0xFFFFFFFFU;

    status = sertos_task_create(&cfg, &handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NO_MEMORY, status);
    TEST_ASSERT_NULL(handle);

    status = sertos_task_create(&cfg, NULL);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);
}

void test_task_delete_running_task(void)
{
    SertosTaskConfig cfg;
    SertosTaskHandle handle = NULL;
    SertosStatus status;

    cfg.name = "RunningDel";
    cfg.entry_func = dummy_task_entry;
    cfg.param = NULL;
    cfg.priority = 2U;
    cfg.stack_buffer = s_task_stack;
    cfg.stack_size = sizeof(s_task_stack);

    (void)sertos_task_create_static(&cfg, &s_test_tcb, &handle);
    sertos_scheduler_set_current_tcb(&s_test_tcb);
    s_test_tcb.state = SERTOS_TASK_STATE_RUNNING;

    status = sertos_task_delete(NULL);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_TERMINATED, s_test_tcb.state);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_task_create_static_null_validation);
    RUN_TEST(test_task_create_static_invalid_parameters);
    RUN_TEST(test_task_create_static_success_and_metadata);
    RUN_TEST(test_task_stack_painting_and_high_water_mark);
    RUN_TEST(test_task_create_dynamic_from_memory_pool);
    RUN_TEST(test_task_priority_modification);
    RUN_TEST(test_task_suspend_and_resume_lifecycle);
    RUN_TEST(test_task_invalid_handle_and_corruption);
    RUN_TEST(test_task_create_dynamic_exhaustion);
    RUN_TEST(test_task_delete_running_task);
    return UNITY_END();
}
