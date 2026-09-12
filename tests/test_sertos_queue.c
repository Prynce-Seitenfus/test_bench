/**
 * @file test_sertos_queue.c
 * @brief Unit tests for SertOS Thread-Safe Message Queue IPC Primitives.
 */

#include "unity.h"
#include "sertos_queue.h"
#include "sertos_scheduler.h"
#include "sertos_task.h"
#include "memory_pool.h"
#include <string.h>

#define QUEUE_STORAGE_SIZE  (64U)

static SertosQueue s_queue;
static SertosQueueHandle s_q_handle;
static uint8_t s_storage[QUEUE_STORAGE_SIZE];

static uint8_t s_worker_stack[512] __attribute__((aligned(8)));
static SertosTaskControlBlock s_worker_tcb;
static SertosTaskHandle s_worker_handle;

static void worker_entry(void* param)
{
    (void)param;
}

static uint8_t s_test_mem_pool[64U * 1024U] __attribute__((aligned(8)));

void setUp(void)
{
    SertosTaskConfig cfg;

    (void)memory_pool_init(s_test_mem_pool, sizeof(s_test_mem_pool));
    (void)sertos_scheduler_init();
    (void)memset(&s_queue, 0, sizeof(s_queue));
    (void)memset(s_storage, 0, sizeof(s_storage));
    s_q_handle = NULL;

    cfg.name = "QueueWorker";
    cfg.entry_func = worker_entry;
    cfg.param = NULL;
    cfg.priority = 3U;
    cfg.stack_buffer = s_worker_stack;
    cfg.stack_size = sizeof(s_worker_stack);
    (void)sertos_task_create_static(&cfg, &s_worker_tcb, &s_worker_handle);
    sertos_scheduler_set_current_tcb(&s_worker_tcb);
    s_worker_tcb.state = SERTOS_TASK_STATE_RUNNING;
}

void tearDown(void)
{
}

void test_queue_create_validation(void)
{
    SertosStatus status;
    SertosQueueHandle dyn_q = NULL;

    /* NULL params */
    status = sertos_queue_create_static(NULL, s_storage, 64U, sizeof(uint32_t), &s_q_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    status = sertos_queue_create_static(&s_queue, NULL, 64U, sizeof(uint32_t), &s_q_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    /* Invalid item size */
    status = sertos_queue_create_static(&s_queue, s_storage, 64U, 0U, &s_q_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);

    /* Dynamic allocation */
    status = sertos_queue_create(sizeof(uint32_t), 4U, &dyn_q);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NOT_NULL(dyn_q);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_queue_delete(dyn_q));
}

void test_queue_send_receive_fifo_ordering(void)
{
    SertosStatus status;
    uint32_t send_val;
    uint32_t recv_val = 0U;

    status = sertos_queue_create_static(&s_queue, s_storage, 64U, sizeof(uint32_t), &s_q_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_queue_get_count(s_q_handle));

    /* Send 10, 20, 30 */
    send_val = 10U;
    status = sertos_queue_send(s_q_handle, &send_val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    send_val = 20U;
    status = sertos_queue_send(s_q_handle, &send_val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    send_val = 30U;
    status = sertos_queue_send(s_q_handle, &send_val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    TEST_ASSERT_EQUAL_UINT32(3U, (uint32_t)sertos_queue_get_count(s_q_handle));

    /* Peek first item */
    status = sertos_queue_peek(s_q_handle, &recv_val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(10U, recv_val);
    TEST_ASSERT_EQUAL_UINT32(3U, (uint32_t)sertos_queue_get_count(s_q_handle));

    /* Receive in FIFO order */
    status = sertos_queue_receive(s_q_handle, &recv_val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(10U, recv_val);

    status = sertos_queue_receive(s_q_handle, &recv_val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(20U, recv_val);

    status = sertos_queue_receive(s_q_handle, &recv_val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(30U, recv_val);

    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_queue_get_count(s_q_handle));

    /* Empty receive with NO_WAIT returns TIMEOUT */
    status = sertos_queue_receive(s_q_handle, &recv_val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_TIMEOUT, status);
}

void test_queue_full_and_reset(void)
{
    SertosStatus status;
    uint32_t val = 42U;
    size_t spaces;

    /* Create 16-byte storage with 4-byte items (effective capacity 15 bytes = 3 items) */
    status = sertos_queue_create_static(&s_queue, s_storage, 16U, sizeof(uint32_t), &s_q_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    spaces = sertos_queue_get_spaces_available(s_q_handle);
    TEST_ASSERT_TRUE(spaces > 0U);

    while (sertos_queue_get_spaces_available(s_q_handle) > 0U) {
        status = sertos_queue_send(s_q_handle, &val, SERTOS_NO_WAIT);
        TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    }

    /* Send on full queue with NO_WAIT returns TIMEOUT */
    status = sertos_queue_send(s_q_handle, &val, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_TIMEOUT, status);

    /* Reset queue */
    status = sertos_queue_reset(s_q_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_queue_get_count(s_q_handle));
}

void test_queue_isr_send_receive(void)
{
    SertosStatus status;
    uint32_t send_val = 99U;
    uint32_t recv_val = 0U;
    bool higher_woken = false;

    (void)sertos_queue_create_static(&s_queue, s_storage, 64U, sizeof(uint32_t), &s_q_handle);

    status = sertos_queue_send_from_isr(s_q_handle, &send_val, &higher_woken);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)sertos_queue_get_count(s_q_handle));

    status = sertos_queue_receive_from_isr(s_q_handle, &recv_val, &higher_woken);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(99U, recv_val);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_queue_create_validation);
    RUN_TEST(test_queue_send_receive_fifo_ordering);
    RUN_TEST(test_queue_full_and_reset);
    RUN_TEST(test_queue_isr_send_receive);
    return UNITY_END();
}
