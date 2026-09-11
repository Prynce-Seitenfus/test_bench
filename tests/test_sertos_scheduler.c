/**
 * @file test_sertos_scheduler.c
 * @brief Unit tests for SertOS Preemptive Priority Scheduler Core.
 *
 * Validates O(1) bitmap priority selection, round-robin time slicing,
 * scheduler preemption lock nesting, monotonic ticks, and delay wakeups.
 */

#include "unity.h"
#include "sertos_scheduler.h"
#include "sertos_task.h"
#include "sertos_port.h"
#include "memory_pool.h"
#include <string.h>

#define STACK_SIZE  (512U)

static uint8_t s_stack_a[STACK_SIZE] __attribute__((aligned(8)));
static uint8_t s_stack_b[STACK_SIZE] __attribute__((aligned(8)));
static uint8_t s_stack_c[STACK_SIZE] __attribute__((aligned(8)));

static SertosTaskControlBlock s_tcb_a;
static SertosTaskControlBlock s_tcb_b;
static SertosTaskControlBlock s_tcb_c;

static SertosTaskHandle s_handle_a;
static SertosTaskHandle s_handle_b;
static SertosTaskHandle s_handle_c;

static void task_entry_dummy(void* param)
{
    (void)param;
}

void setUp(void)
{
    memory_pool_init();
    (void)sertos_scheduler_init();

    (void)memset(s_stack_a, 0, sizeof(s_stack_a));
    (void)memset(s_stack_b, 0, sizeof(s_stack_b));
    (void)memset(s_stack_c, 0, sizeof(s_stack_c));
}

void tearDown(void)
{
}

void test_scheduler_init_creates_idle_task(void)
{
    SertosTaskControlBlock* selected;

    TEST_ASSERT_FALSE(sertos_scheduler_is_running());
    TEST_ASSERT_FALSE(sertos_scheduler_is_locked());
    TEST_ASSERT_EQUAL_UINT32(0U, sertos_scheduler_get_tick_count());

    /* Scheduler init creates Idle Task at Priority 0 */
    selected = sertos_scheduler_select_next_task();
    TEST_ASSERT_NOT_NULL(selected);
    TEST_ASSERT_EQUAL_UINT8(0U, selected->priority);
    TEST_ASSERT_EQUAL_STRING("Idle", selected->name);
}

void test_scheduler_select_highest_priority(void)
{
    SertosTaskConfig cfg_a;
    SertosTaskConfig cfg_b;
    SertosTaskConfig cfg_c;
    SertosTaskControlBlock* selected;

    cfg_a.name = "Low";
    cfg_a.entry_func = task_entry_dummy;
    cfg_a.param = NULL;
    cfg_a.priority = 2U;
    cfg_a.stack_buffer = s_stack_a;
    cfg_a.stack_size = sizeof(s_stack_a);

    cfg_b.name = "Mid";
    cfg_b.entry_func = task_entry_dummy;
    cfg_b.param = NULL;
    cfg_b.priority = 8U;
    cfg_b.stack_buffer = s_stack_b;
    cfg_b.stack_size = sizeof(s_stack_b);

    cfg_c.name = "High";
    cfg_c.entry_func = task_entry_dummy;
    cfg_c.param = NULL;
    cfg_c.priority = 25U;
    cfg_c.stack_buffer = s_stack_c;
    cfg_c.stack_size = sizeof(s_stack_c);

    (void)sertos_task_create_static(&cfg_a, &s_tcb_a, &s_handle_a);
    (void)sertos_task_create_static(&cfg_b, &s_tcb_b, &s_handle_b);
    (void)sertos_task_create_static(&cfg_c, &s_tcb_c, &s_handle_c);

    /* O(1) CLZ lookup must select Task C (priority 25) */
    selected = sertos_scheduler_select_next_task();
    TEST_ASSERT_NOT_NULL(selected);
    TEST_ASSERT_EQUAL_PTR(&s_tcb_c, selected);
    TEST_ASSERT_EQUAL_UINT8(25U, selected->priority);

    /* Remove Task C; Task B (priority 8) must now be selected */
    (void)sertos_scheduler_remove_ready(&s_tcb_c);
    selected = sertos_scheduler_select_next_task();
    TEST_ASSERT_NOT_NULL(selected);
    TEST_ASSERT_EQUAL_PTR(&s_tcb_b, selected);

    /* Remove Task B; Task A (priority 2) must now be selected */
    (void)sertos_scheduler_remove_ready(&s_tcb_b);
    selected = sertos_scheduler_select_next_task();
    TEST_ASSERT_NOT_NULL(selected);
    TEST_ASSERT_EQUAL_PTR(&s_tcb_a, selected);
}

void test_scheduler_round_robin_rotation(void)
{
    SertosTaskConfig cfg_a;
    SertosTaskConfig cfg_b;
    SertosTaskControlBlock* selected;

    cfg_a.name = "WorkerA";
    cfg_a.entry_func = task_entry_dummy;
    cfg_a.param = NULL;
    cfg_a.priority = 5U;
    cfg_a.stack_buffer = s_stack_a;
    cfg_a.stack_size = sizeof(s_stack_a);

    cfg_b.name = "WorkerB";
    cfg_b.entry_func = task_entry_dummy;
    cfg_b.param = NULL;
    cfg_b.priority = 5U;
    cfg_b.stack_buffer = s_stack_b;
    cfg_b.stack_size = sizeof(s_stack_b);

    (void)sertos_task_create_static(&cfg_a, &s_tcb_a, &s_handle_a);
    (void)sertos_task_create_static(&cfg_b, &s_tcb_b, &s_handle_b);

    /* First in queue is WorkerA */
    selected = sertos_scheduler_select_next_task();
    TEST_ASSERT_EQUAL_PTR(&s_tcb_a, selected);

    /* Simulate context switch to WorkerA */
    sertos_scheduler_set_current_tcb(&s_tcb_a);
    s_tcb_a.state = SERTOS_TASK_STATE_RUNNING;

    /* Yield voluntarily triggers time slicing rotation */
    sertos_scheduler_yield();

    /* After rotation, WorkerB should be next */
    selected = sertos_scheduler_select_next_task();
    TEST_ASSERT_EQUAL_PTR(&s_tcb_b, selected);
}

void test_scheduler_lock_and_nesting(void)
{
    TEST_ASSERT_FALSE(sertos_scheduler_is_locked());

    sertos_scheduler_lock();
    TEST_ASSERT_TRUE(sertos_scheduler_is_locked());

    /* Nested lock */
    sertos_scheduler_lock();
    TEST_ASSERT_TRUE(sertos_scheduler_is_locked());

    /* First unlock */
    sertos_scheduler_unlock();
    TEST_ASSERT_TRUE(sertos_scheduler_is_locked());

    /* Outer unlock */
    sertos_scheduler_unlock();
    TEST_ASSERT_FALSE(sertos_scheduler_is_locked());
}

void test_scheduler_delay_and_tick_wakeup(void)
{
    SertosTaskConfig cfg_a;
    SertosTaskControlBlock* selected;

    cfg_a.name = "Sleeper";
    cfg_a.entry_func = task_entry_dummy;
    cfg_a.param = NULL;
    cfg_a.priority = 10U;
    cfg_a.stack_buffer = s_stack_a;
    cfg_a.stack_size = sizeof(s_stack_a);

    (void)sertos_task_create_static(&cfg_a, &s_tcb_a, &s_handle_a);

    /* Make Sleeper the running task */
    sertos_scheduler_set_current_tcb(&s_tcb_a);
    s_tcb_a.state = SERTOS_TASK_STATE_RUNNING;

    /* Start scheduler */
    sertos_scheduler_start();
    TEST_ASSERT_TRUE(sertos_scheduler_is_running());

    /* Delay task for 3 ticks */
    (void)sertos_scheduler_delay(3U);
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_BLOCKED, s_tcb_a.state);

    /* While delayed, Idle task should be selected (no other tasks) */
    selected = sertos_scheduler_select_next_task();
    TEST_ASSERT_EQUAL_UINT8(0U, selected->priority);

    /* Tick 1 */
    sertos_scheduler_tick();
    TEST_ASSERT_EQUAL_UINT32(1U, sertos_scheduler_get_tick_count());
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_BLOCKED, s_tcb_a.state);

    /* Tick 2 */
    sertos_scheduler_tick();
    TEST_ASSERT_EQUAL_UINT32(2U, sertos_scheduler_get_tick_count());
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_BLOCKED, s_tcb_a.state);

    /* Tick 3: Delay expires, Sleeper preempts Idle and transitions to RUNNING */
    sertos_scheduler_tick();
    TEST_ASSERT_EQUAL_UINT32(3U, sertos_scheduler_get_tick_count());
    TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_RUNNING, s_tcb_a.state);
    TEST_ASSERT_EQUAL_PTR(&s_tcb_a, sertos_scheduler_get_current_tcb());
}

void test_scheduler_parameter_validation_and_edge_cases(void)
{
    SertosTaskControlBlock invalid_tcb;
    (void)memset(&invalid_tcb, 0, sizeof(invalid_tcb));
    invalid_tcb.priority = SERTOS_CONFIG_MAX_PRIORITIES;

    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_scheduler_add_ready(NULL));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_scheduler_add_ready(&invalid_tcb));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_scheduler_remove_ready(NULL));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_scheduler_remove_ready(&invalid_tcb));

    /* Delay with 0 ticks yields */
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_scheduler_delay(0U));

    /* Reschedule when not running is safe no-op */
    sertos_scheduler_reschedule();
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_scheduler_init_creates_idle_task);
    RUN_TEST(test_scheduler_select_highest_priority);
    RUN_TEST(test_scheduler_round_robin_rotation);
    RUN_TEST(test_scheduler_lock_and_nesting);
    RUN_TEST(test_scheduler_delay_and_tick_wakeup);
    RUN_TEST(test_scheduler_parameter_validation_and_edge_cases);
    return UNITY_END();
}
