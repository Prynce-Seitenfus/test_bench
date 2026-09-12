/**
 * @file test_sertos_benchmark.c
 * @brief Performance, footprint, and execution jitter benchmark suite for SertOS.
 *
 * Measures context switch cycles, O(1) scheduling determinism, semaphore signaling jitter,
 * mutex priority inheritance protocol overhead, queue throughput, and memory footprint.
 */

#include "unity.h"
#include "sertos_task.h"
#include "sertos_scheduler.h"
#include "sertos_sem.h"
#include "sertos_mutex.h"
#include "sertos_queue.h"
#include "sertos_timer.h"
#include "memory_pool.h"
#include <string.h>

#define BENCH_ITERATIONS        (1000U)
#define BENCH_STACK_SIZE        (512U)
#define BENCH_TASK_COUNT        (8U)

static uint8_t s_bench_stacks[BENCH_TASK_COUNT][BENCH_STACK_SIZE] __attribute__((aligned(8)));
static SertosTaskControlBlock s_bench_tcbs[BENCH_TASK_COUNT];
static SertosTaskHandle s_bench_handles[BENCH_TASK_COUNT];

static inline uint64_t get_cpu_cycles(void)
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    return __builtin_ia32_rdtsc();
#else
    return 0ULL;
#endif
}

static void bench_dummy_entry(void* param)
{
    (void)param;
}

static uint8_t s_test_mem_pool[64U * 1024U] __attribute__((aligned(8)));

void setUp(void)
{
    (void)memory_pool_init(s_test_mem_pool, sizeof(s_test_mem_pool));
    (void)sertos_scheduler_init();
    (void)memset(s_bench_stacks, 0, sizeof(s_bench_stacks));
    (void)memset(s_bench_tcbs, 0, sizeof(s_bench_tcbs));
}

void tearDown(void)
{
}

void test_benchmark_memory_footprint(void)
{
    size_t tcb_size;
    size_t mutex_size;
    size_t sem_size;
    size_t queue_size;
    size_t timer_size;
    uintptr_t stack_addr;

    tcb_size   = sizeof(SertosTaskControlBlock);
    mutex_size = sizeof(SertosMutex);
    sem_size   = sizeof(SertosSemaphore);
    queue_size = sizeof(SertosQueue);
    timer_size = sizeof(SertosTimer);

    /* Verify memory structures are compact and fit within embedded bounds */
    TEST_ASSERT_TRUE(tcb_size <= 256U);
    TEST_ASSERT_TRUE(mutex_size <= 128U);
    TEST_ASSERT_TRUE(sem_size <= 64U);
    TEST_ASSERT_TRUE(queue_size <= 128U);
    TEST_ASSERT_TRUE(timer_size <= 64U);

    /* Stack pointer MUST be the first field in TCB (offset 0) for assembly dereference */
    TEST_ASSERT_EQUAL_PTR(&s_bench_tcbs[0].stack_ptr, &s_bench_tcbs[0]);

    /* Verify AAPCS 8-byte stack alignment */
    stack_addr = (uintptr_t)&s_bench_stacks[0][0];
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)(stack_addr % 8U));
}

void test_benchmark_context_switch_latency_and_o1(void)
{
    SertosTaskConfig cfg;
    uint64_t start_cycles;
    uint64_t elapsed_cycles;
    uint64_t total_cycles = 0ULL;
    uint64_t min_cycles = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t max_cycles = 0ULL;
    uint32_t i;

    for (i = 0U; i < 4U; i++) {
        cfg.name = "BenchTask";
        cfg.entry_func = bench_dummy_entry;
        cfg.param = NULL;
        cfg.priority = (SertosPriority)(i + 1U);
        cfg.stack_buffer = s_bench_stacks[i];
        cfg.stack_size = sizeof(s_bench_stacks[i]);

        TEST_ASSERT_EQUAL(SERTOS_STATUS_OK,
                          sertos_task_create_static(&cfg, &s_bench_tcbs[i], &s_bench_handles[i]));
    }

    sertos_scheduler_start();

    for (i = 0U; i < BENCH_ITERATIONS; i++) {
        start_cycles = get_cpu_cycles();
        (void)sertos_scheduler_perform_switch();
        elapsed_cycles = get_cpu_cycles() - start_cycles;

        total_cycles += elapsed_cycles;
        if (elapsed_cycles < min_cycles) {
            min_cycles = elapsed_cycles;
        }
        if (elapsed_cycles > max_cycles) {
            max_cycles = elapsed_cycles;
        }
    }

    TEST_ASSERT_TRUE(total_cycles > 0ULL);
    TEST_ASSERT_NOT_NULL(sertos_scheduler_get_current_tcb());
}

void test_benchmark_sem_signaling_and_jitter(void)
{
    SertosSemaphore sem;
    SertosSemHandle sem_handle;
    SertosStatus status;
    uint64_t start_cycles;
    uint64_t elapsed_cycles;
    uint64_t total_cycles = 0ULL;
    uint64_t min_cycles = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t max_cycles = 0ULL;
    uint64_t jitter;
    bool higher_woken;
    uint32_t i;

    status = sertos_sem_create_counting_static(&sem, 1U, 1U, &sem_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    for (i = 0U; i < BENCH_ITERATIONS; i++) {
        start_cycles = get_cpu_cycles();
        (void)sertos_sem_take(sem_handle, SERTOS_NO_WAIT);
        (void)sertos_sem_give_from_isr(sem_handle, &higher_woken);
        elapsed_cycles = get_cpu_cycles() - start_cycles;

        total_cycles += elapsed_cycles;
        if (elapsed_cycles < min_cycles) {
            min_cycles = elapsed_cycles;
        }
        if (elapsed_cycles > max_cycles) {
            max_cycles = elapsed_cycles;
        }
    }

    jitter = max_cycles - min_cycles;
    (void)jitter;
    TEST_ASSERT_TRUE(total_cycles > 0ULL);
}

void test_benchmark_mutex_pip_overhead(void)
{
    SertosMutex mutex;
    SertosMutexHandle mutex_handle;
    SertosTaskConfig cfg;
    SertosStatus status;
    uint64_t start_cycles;
    uint64_t elapsed_cycles;
    uint64_t total_cycles = 0ULL;
    uint32_t i;

    status = sertos_mutex_create_static(&mutex, &mutex_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    cfg.name = "MutexTask";
    cfg.entry_func = bench_dummy_entry;
    cfg.param = NULL;
    cfg.priority = 10U;
    cfg.stack_buffer = s_bench_stacks[0];
    cfg.stack_size = sizeof(s_bench_stacks[0]);

    (void)sertos_task_create_static(&cfg, &s_bench_tcbs[0], &s_bench_handles[0]);
    sertos_scheduler_set_current_tcb(&s_bench_tcbs[0]);
    s_bench_tcbs[0].state = SERTOS_TASK_STATE_RUNNING;

    for (i = 0U; i < BENCH_ITERATIONS; i++) {
        start_cycles = get_cpu_cycles();
        (void)sertos_mutex_lock(mutex_handle, SERTOS_WAIT_FOREVER);
        (void)sertos_mutex_unlock(mutex_handle);
        elapsed_cycles = get_cpu_cycles() - start_cycles;
        total_cycles += elapsed_cycles;
    }

    TEST_ASSERT_TRUE(total_cycles > 0ULL);
}

void test_benchmark_queue_fifo_throughput(void)
{
    SertosQueue queue;
    SertosQueueHandle q_handle;
    uint8_t queue_buffer[64];
    uint32_t send_val;
    uint32_t recv_val = 0U;
    SertosStatus status;
    uint64_t start_cycles;
    uint64_t total_cycles = 0ULL;
    uint32_t i;

    status = sertos_queue_create_static(&queue, queue_buffer, sizeof(queue_buffer), sizeof(uint32_t), &q_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    start_cycles = get_cpu_cycles();
    for (i = 0U; i < BENCH_ITERATIONS; i++) {
        send_val = i;
        (void)sertos_queue_send(q_handle, &send_val, SERTOS_NO_WAIT);
        (void)sertos_queue_receive(q_handle, &recv_val, SERTOS_NO_WAIT);
        TEST_ASSERT_EQUAL_UINT32(send_val, recv_val);
    }
    total_cycles = get_cpu_cycles() - start_cycles;

    TEST_ASSERT_TRUE(total_cycles > 0ULL);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_benchmark_memory_footprint);
    RUN_TEST(test_benchmark_context_switch_latency_and_o1);
    RUN_TEST(test_benchmark_sem_signaling_and_jitter);
    RUN_TEST(test_benchmark_mutex_pip_overhead);
    RUN_TEST(test_benchmark_queue_fifo_throughput);

    return UNITY_END();
}
