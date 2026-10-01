/**
 * @file test_sertos_stats.c
 * @brief Unit tests for the SertOS runtime statistics (telemetry) subsystem.
 *
 * Validates per-task run-time accounting, context-switch counting, CPU-share
 * computation, idle-load complement, snapshot capacity truncation, counter
 * reset, and the runtime enable/disable configuration gate. A controllable fake
 * run-time clock overrides the weak port counter for deterministic assertions.
 */

#include "unity.h"
#include "sertos_scheduler.h"
#include "sertos_stats.h"
#include "sertos_task.h"
#include "sertos_port.h"
#include "memory_pool.h"
#include <string.h>

#define STACK_SIZE  (512U)

static uint8_t s_test_mem_pool[8192] __attribute__((aligned(8)));
static uint8_t s_stack_a[STACK_SIZE] __attribute__((aligned(8)));
static uint8_t s_stack_b[STACK_SIZE] __attribute__((aligned(8)));

static SertosTaskControlBlock s_tcb_a;
static SertosTaskControlBlock s_tcb_b;
static SertosTaskHandle s_handle_a;
static SertosTaskHandle s_handle_b;
static SertosTaskControlBlock* s_idle_tcb;

/**
 * @brief Controllable fake run-time counter value (strong port override).
 */
static uint32_t s_fake_clock = 0U;

/**
 * @brief Strong override of the weak port run-time counter for deterministic tests.
 *
 * @return Current fake clock value.
 */
uint32_t sertos_port_runtime_counter(void)
{
    return s_fake_clock;
}

static void task_entry_dummy(void* param)
{
    (void)param;
}

static void init_with_stats(bool enabled)
{
    SertosConfig config;

    (void)memset(&config, 0, sizeof(config));
    config.tick_rate_hz = 1000U;
    config.enable_time_slicing = true;
    config.enable_runtime_stats = enabled;

    (void)memory_pool_init(s_test_mem_pool, sizeof(s_test_mem_pool));
    (void)sertos_scheduler_init_with_config(&config);
    s_idle_tcb = sertos_scheduler_get_idle_tcb();
}

static void create_two_tasks(void)
{
    SertosTaskConfig cfg_a;
    SertosTaskConfig cfg_b;

    (void)memset(s_stack_a, 0, sizeof(s_stack_a));
    (void)memset(s_stack_b, 0, sizeof(s_stack_b));

    cfg_a.name = "TaskA";
    cfg_a.entry_func = task_entry_dummy;
    cfg_a.param = NULL;
    cfg_a.priority = 2U;
    cfg_a.stack_buffer = s_stack_a;
    cfg_a.stack_size = sizeof(s_stack_a);

    cfg_b.name = "TaskB";
    cfg_b.entry_func = task_entry_dummy;
    cfg_b.param = NULL;
    cfg_b.priority = 1U;
    cfg_b.stack_buffer = s_stack_b;
    cfg_b.stack_size = sizeof(s_stack_b);

    (void)sertos_task_create_static(&cfg_a, &s_tcb_a, &s_handle_a);
    (void)sertos_task_create_static(&cfg_b, &s_tcb_b, &s_handle_b);
}

void setUp(void)
{
    s_fake_clock = 0U;
    s_handle_a = NULL;
    s_handle_b = NULL;
    init_with_stats(true);
    create_two_tasks();
}

void tearDown(void)
{
    if (sertos_scheduler_is_running()) {
        sertos_scheduler_stop();
    }
    if ((s_handle_a != NULL) && (s_handle_a->state != SERTOS_TASK_STATE_TERMINATED)) {
        (void)sertos_task_delete(s_handle_a);
    }
    if ((s_handle_b != NULL) && (s_handle_b->state != SERTOS_TASK_STATE_TERMINATED)) {
        (void)sertos_task_delete(s_handle_b);
    }
    if ((s_idle_tcb != NULL) && (s_idle_tcb->state != SERTOS_TASK_STATE_TERMINATED)) {
        (void)sertos_task_delete(s_idle_tcb);
    }
}

/* Accounting correctness: run-time deltas accumulate exactly per task. */
void test_stats_accounting_accumulates_run_time(void)
{
    s_fake_clock = 100U;
    sertos_stats_reset();

    s_fake_clock = 150U;
    sertos_stats_on_switch(&s_tcb_a, &s_tcb_b);   /* A ran 50 units */
    s_fake_clock = 200U;
    sertos_stats_on_switch(&s_tcb_b, &s_tcb_a);   /* B ran 50 units */
    s_fake_clock = 260U;
    sertos_stats_on_switch(&s_tcb_a, &s_tcb_b);   /* A ran 60 units */

    TEST_ASSERT_EQUAL_UINT64(110U, s_tcb_a.run_time_total);
    TEST_ASSERT_EQUAL_UINT64(50U, s_tcb_b.run_time_total);
}

/* Switch counts: per-task switch-in and global totals. */
void test_stats_switch_counts(void)
{
    SertosSystemStats sys;

    s_fake_clock = 0U;
    sertos_stats_reset();

    sertos_stats_on_switch(&s_tcb_a, &s_tcb_b);
    sertos_stats_on_switch(&s_tcb_b, &s_tcb_a);
    sertos_stats_on_switch(&s_tcb_a, &s_tcb_b);

    TEST_ASSERT_EQUAL_UINT32(1U, s_tcb_a.switch_in_count);
    TEST_ASSERT_EQUAL_UINT32(2U, s_tcb_b.switch_in_count);

    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stats_get_system(&sys));
    TEST_ASSERT_EQUAL_UINT32(3U, sys.total_switches);
}

/* A switch to the same task accounts run-time but not a new switch-in. */
void test_stats_same_task_switch_is_not_counted(void)
{
    s_fake_clock = 0U;
    sertos_stats_reset();

    s_fake_clock = 40U;
    sertos_stats_on_switch(&s_tcb_a, &s_tcb_a);

    TEST_ASSERT_EQUAL_UINT64(40U, s_tcb_a.run_time_total);
    TEST_ASSERT_EQUAL_UINT32(0U, s_tcb_a.switch_in_count);
}

/* CPU-percent: two tasks with a known 110:50 run ratio. */
void test_stats_cpu_percent_shares(void)
{
    SertosTaskStats stats_a;
    SertosTaskStats stats_b;

    s_fake_clock = 0U;
    sertos_stats_reset();

    s_tcb_a.run_time_total = 110U;
    s_tcb_b.run_time_total = 50U;
    /* Total across idle(0)+A+B = 160. */

    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stats_get_task(s_handle_a, &stats_a));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stats_get_task(s_handle_b, &stats_b));

    TEST_ASSERT_EQUAL_UINT32(6875U, stats_a.cpu_percent_x100);
    TEST_ASSERT_EQUAL_UINT32(3125U, stats_b.cpu_percent_x100);
}

/* Idle load and CPU load are complementary to 100%. */
void test_stats_idle_load_complement(void)
{
    SertosSystemStats sys;
    uint32_t cpu_load;

    s_fake_clock = 0U;
    sertos_stats_reset();

    s_idle_tcb->run_time_total = 75U;
    s_tcb_a.run_time_total = 25U;
    /* Total = 100; idle share = 75%. */

    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stats_get_system(&sys));
    TEST_ASSERT_EQUAL_UINT32(7500U, sys.idle_percent_x100);

    cpu_load = sertos_stats_get_cpu_load_x100();
    TEST_ASSERT_EQUAL_UINT32(10000U - sys.idle_percent_x100, cpu_load);
    TEST_ASSERT_EQUAL_UINT32(2500U, cpu_load);
}

/* Snapshot capacity smaller than task count truncates safely. */
void test_stats_snapshot_capacity_truncates(void)
{
    SertosTaskStats one[1];
    SertosSystemStats sys;
    size_t written;

    s_fake_clock = 0U;
    sertos_stats_reset();

    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stats_get_system(&sys));
    TEST_ASSERT_EQUAL_UINT32(3U, sys.task_count);   /* idle + A + B */

    written = sertos_stats_get_tasks(one, 1U);
    TEST_ASSERT_EQUAL_UINT32(1U, written);
}

/* Reset returns all counters to zero. */
void test_stats_reset_clears_counters(void)
{
    SertosSystemStats sys;
    SertosTaskStats stats_a;

    s_fake_clock = 0U;
    sertos_stats_reset();
    sertos_stats_on_switch(&s_tcb_a, &s_tcb_b);
    s_fake_clock = 500U;
    sertos_stats_on_switch(&s_tcb_b, &s_tcb_a);

    s_fake_clock = 1000U;
    sertos_stats_reset();

    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stats_get_system(&sys));
    TEST_ASSERT_EQUAL_UINT32(0U, sys.total_switches);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stats_get_task(s_handle_a, &stats_a));
    TEST_ASSERT_EQUAL_UINT64(0U, stats_a.run_time);
    TEST_ASSERT_EQUAL_UINT32(0U, stats_a.switch_in_count);
}

/* Disabled at init: accounting is inert and snapshots report NOT_INITIALIZED. */
void test_stats_runtime_disable(void)
{
    SertosSystemStats sys;

    tearDown();
    s_fake_clock = 0U;
    init_with_stats(false);
    create_two_tasks();

    TEST_ASSERT_FALSE(sertos_stats_is_enabled());

    s_fake_clock = 100U;
    sertos_stats_on_switch(&s_tcb_a, &s_tcb_b);
    s_fake_clock = 200U;
    sertos_stats_on_switch(&s_tcb_b, &s_tcb_a);

    TEST_ASSERT_EQUAL_UINT64(0U, s_tcb_a.run_time_total);
    TEST_ASSERT_EQUAL_UINT64(0U, s_tcb_b.run_time_total);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NOT_INITIALIZED, sertos_stats_get_system(&sys));
}

/* Enabled at init: accounting is active and reflected in snapshots. */
void test_stats_runtime_enable_at_init(void)
{
    SertosSystemStats sys;

    TEST_ASSERT_TRUE(sertos_stats_is_enabled());

    s_fake_clock = 0U;
    sertos_stats_reset();
    s_fake_clock = 30U;
    sertos_stats_on_switch(&s_tcb_a, &s_tcb_b);

    TEST_ASSERT_EQUAL_UINT64(30U, s_tcb_a.run_time_total);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stats_get_system(&sys));
    TEST_ASSERT_EQUAL_UINT32(1U, sys.total_switches);
}

/* NULL-argument validation for the snapshot APIs. */
void test_stats_null_argument_validation(void)
{
    SertosTaskStats rec;

    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, sertos_stats_get_system(NULL));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, sertos_stats_get_task(NULL, &rec));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, sertos_stats_get_task(s_handle_a, NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, sertos_stats_get_tasks(NULL, 4U));
}

/* Integration: the scheduler switch path drives accounting. */
void test_stats_perform_switch_integration(void)
{
    s_fake_clock = 0U;
    sertos_stats_reset();

    s_fake_clock = 10U;
    (void)sertos_scheduler_perform_switch();   /* selects highest priority: TaskA */

    TEST_ASSERT_EQUAL(&s_tcb_a, sertos_scheduler_get_current_tcb());
    TEST_ASSERT_EQUAL_UINT32(1U, s_tcb_a.switch_in_count);
}

/* Registry: a task blocked forever (detached state_node) still appears. */
void test_stats_forever_blocked_task_is_visible(void)
{
    SertosTaskStats snapshot[SERTOS_CONFIG_STATS_MAX_TASKS];
    size_t written;
    size_t i;
    bool found_a = false;

    /* Simulate a WAIT_FOREVER block: removed from ready queues with a
       detached state_node, mirroring sertos_scheduler_wait_list_block(). */
    (void)sertos_scheduler_remove_ready(&s_tcb_a);
    s_tcb_a.state = SERTOS_TASK_STATE_BLOCKED;
    s_tcb_a.state_node.next = NULL;
    s_tcb_a.state_node.prev = NULL;

    written = sertos_stats_get_tasks(snapshot, SERTOS_CONFIG_STATS_MAX_TASKS);
    TEST_ASSERT_EQUAL_UINT32(3U, written);   /* idle + A (blocked) + B */

    for (i = 0U; i < written; i++) {
        if (snapshot[i].name == s_tcb_a.name) {
            found_a = true;
            TEST_ASSERT_EQUAL(SERTOS_TASK_STATE_BLOCKED, snapshot[i].state);
        }
    }
    TEST_ASSERT_TRUE(found_a);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_stats_accounting_accumulates_run_time);
    RUN_TEST(test_stats_switch_counts);
    RUN_TEST(test_stats_same_task_switch_is_not_counted);
    RUN_TEST(test_stats_cpu_percent_shares);
    RUN_TEST(test_stats_idle_load_complement);
    RUN_TEST(test_stats_snapshot_capacity_truncates);
    RUN_TEST(test_stats_reset_clears_counters);
    RUN_TEST(test_stats_runtime_disable);
    RUN_TEST(test_stats_runtime_enable_at_init);
    RUN_TEST(test_stats_null_argument_validation);
    RUN_TEST(test_stats_perform_switch_integration);
    RUN_TEST(test_stats_forever_blocked_task_is_visible);
    return UNITY_END();
}
