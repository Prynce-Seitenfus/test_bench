/**
 * @file test_sertos_stream_buffer.c
 * @brief Unit tests for SertOS Single-Producer Single-Consumer Stream Buffer Primitive.
 */

#include "unity.h"
#include "sertos_stream_buffer.h"
#include "sertos_scheduler.h"
#include "sertos_task.h"
#include "memory_pool.h"
#include <string.h>

#define SB_STORAGE_SIZE  (64U)

static SertosStreamBuffer s_stream_buf;
static SertosStreamBufferHandle s_sb_handle;
static uint8_t s_storage[SB_STORAGE_SIZE] __attribute__((aligned(8)));

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
    (void)memset(&s_stream_buf, 0, sizeof(s_stream_buf));
    (void)memset(s_storage, 0, sizeof(s_storage));
    s_sb_handle = NULL;

    cfg.name = "StreamWorker";
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

void test_stream_buffer_create_validation(void)
{
    SertosStatus status;
    SertosStreamBufferHandle dyn_sb = NULL;

    /* NULL parameter checks */
    status = sertos_stream_buffer_create_static(NULL, s_storage, 64U, 1U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    status = sertos_stream_buffer_create_static(&s_stream_buf, NULL, 64U, 1U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 64U, 1U, NULL);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_NULL_PTR, status);

    /* Storage size invalid (< 2 or not power of 2) */
    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 0U, 1U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);

    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 33U, 1U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);

    /* Trigger level greater than capacity */
    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 16U, 16U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);

    /* Trigger level 0 defaults to 1 */
    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 64U, 0U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, (uint32_t)s_stream_buf.trigger_level_bytes);

    /* Dynamic allocation */
    status = sertos_stream_buffer_create(32U, 1U, &dyn_sb);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_NOT_NULL(dyn_sb);
    TEST_ASSERT_FALSE(dyn_sb->is_statically_allocated);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, sertos_stream_buffer_delete(dyn_sb));
}

void test_stream_buffer_send_receive_fifo_ordering(void)
{
    SertosStatus status;
    const uint8_t tx_data[5] = { 0x11U, 0x22U, 0x33U, 0x44U, 0x55U };
    uint8_t rx_data[8] = { 0 };
    size_t transferred;

    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 64U, 1U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_TRUE(sertos_stream_buffer_is_empty(s_sb_handle));
    TEST_ASSERT_FALSE(sertos_stream_buffer_is_full(s_sb_handle));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_bytes_available(s_sb_handle));

    /* Send 5 bytes */
    transferred = sertos_stream_buffer_send(s_sb_handle, tx_data, 5U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(5U, (uint32_t)transferred);
    TEST_ASSERT_FALSE(sertos_stream_buffer_is_empty(s_sb_handle));
    TEST_ASSERT_EQUAL_UINT32(5U, (uint32_t)sertos_stream_buffer_bytes_available(s_sb_handle));

    /* Read first 3 bytes */
    transferred = sertos_stream_buffer_receive(s_sb_handle, rx_data, 3U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(3U, (uint32_t)transferred);
    TEST_ASSERT_EQUAL_HEX8(0x11U, rx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x22U, rx_data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x33U, rx_data[2]);
    TEST_ASSERT_EQUAL_UINT32(2U, (uint32_t)sertos_stream_buffer_bytes_available(s_sb_handle));

    /* Read remaining 2 bytes */
    transferred = sertos_stream_buffer_receive(s_sb_handle, rx_data, 5U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(2U, (uint32_t)transferred);
    TEST_ASSERT_EQUAL_HEX8(0x44U, rx_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x55U, rx_data[1]);
    TEST_ASSERT_TRUE(sertos_stream_buffer_is_empty(s_sb_handle));

    /* Receive on empty buffer with NO_WAIT returns 0 */
    transferred = sertos_stream_buffer_receive(s_sb_handle, rx_data, 5U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)transferred);
}

void test_stream_buffer_full_and_partial_transfer(void)
{
    SertosStatus status;
    uint8_t tx_chunk[16];
    uint8_t rx_chunk[16];
    size_t spaces;
    size_t transferred;
    uint32_t i;

    /* 16 bytes storage gives 15 bytes usable capacity */
    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 16U, 1U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    for (i = 0U; i < 16U; i++) {
        tx_chunk[i] = (uint8_t)(i + 1U);
    }

    spaces = sertos_stream_buffer_spaces_available(s_sb_handle);
    TEST_ASSERT_EQUAL_UINT32(15U, (uint32_t)spaces);

    /* Attempt to send 16 bytes: only 15 bytes fit */
    transferred = sertos_stream_buffer_send(s_sb_handle, tx_chunk, 16U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(15U, (uint32_t)transferred);
    TEST_ASSERT_TRUE(sertos_stream_buffer_is_full(s_sb_handle));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_spaces_available(s_sb_handle));

    /* Sending when full with NO_WAIT returns 0 */
    transferred = sertos_stream_buffer_send(s_sb_handle, tx_chunk, 4U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)transferred);

    /* Read 5 bytes to make room */
    transferred = sertos_stream_buffer_receive(s_sb_handle, rx_chunk, 5U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(5U, (uint32_t)transferred);
    TEST_ASSERT_FALSE(sertos_stream_buffer_is_full(s_sb_handle));
    TEST_ASSERT_EQUAL_UINT32(5U, (uint32_t)sertos_stream_buffer_spaces_available(s_sb_handle));

    /* Write 5 more bytes to test circular wrap-around */
    transferred = sertos_stream_buffer_send(s_sb_handle, tx_chunk, 5U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(5U, (uint32_t)transferred);
    TEST_ASSERT_TRUE(sertos_stream_buffer_is_full(s_sb_handle));
}

void test_stream_buffer_trigger_level_and_reset(void)
{
    SertosStatus status;
    const uint8_t pattern[8] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U };
    uint8_t rx_buf[8] = { 0 };
    size_t transferred;

    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 64U, 4U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* Send 8 bytes */
    transferred = sertos_stream_buffer_send(s_sb_handle, pattern, 8U, SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(8U, (uint32_t)transferred);

    /* Modify trigger level */
    status = sertos_stream_buffer_set_trigger_level(s_sb_handle, 8U);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* Setting trigger level > capacity fails */
    status = sertos_stream_buffer_set_trigger_level(s_sb_handle, 64U);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, status);

    /* Reset stream buffer */
    status = sertos_stream_buffer_reset(s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);
    TEST_ASSERT_TRUE(sertos_stream_buffer_is_empty(s_sb_handle));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_bytes_available(s_sb_handle));

    transferred = sertos_stream_buffer_receive(s_sb_handle, rx_buf, sizeof(rx_buf), SERTOS_NO_WAIT);
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)transferred);
}

void test_stream_buffer_isr_transfers(void)
{
    SertosStatus status;
    const uint8_t isr_tx[4] = { 0xAAU, 0xBBU, 0xCCU, 0xDDU };
    uint8_t isr_rx[4] = { 0 };
    bool higher_woken = false;
    size_t transferred;

    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 32U, 2U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* ISR write */
    transferred = sertos_stream_buffer_send_from_isr(s_sb_handle, isr_tx, 4U, &higher_woken);
    TEST_ASSERT_EQUAL_UINT32(4U, (uint32_t)transferred);
    TEST_ASSERT_FALSE(higher_woken);
    TEST_ASSERT_EQUAL_UINT32(4U, (uint32_t)sertos_stream_buffer_bytes_available(s_sb_handle));

    /* ISR read */
    transferred = sertos_stream_buffer_receive_from_isr(s_sb_handle, isr_rx, 4U, &higher_woken);
    TEST_ASSERT_EQUAL_UINT32(4U, (uint32_t)transferred);
    TEST_ASSERT_FALSE(higher_woken);
    TEST_ASSERT_EQUAL_HEX8(0xAAU, isr_rx[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBBU, isr_rx[1]);
    TEST_ASSERT_EQUAL_HEX8(0xCCU, isr_rx[2]);
    TEST_ASSERT_EQUAL_HEX8(0xDDU, isr_rx[3]);
    TEST_ASSERT_TRUE(sertos_stream_buffer_is_empty(s_sb_handle));
}

void test_stream_buffer_isr_higher_prio_woken(void)
{
    SertosStatus status;
    SertosTaskControlBlock high_tcb;
    SertosTaskHandle high_handle;
    uint8_t high_stack[512] __attribute__((aligned(8)));
    SertosTaskConfig cfg;
    bool higher_woken = false;
    const uint8_t data[2] = { 0x12U, 0x34U };
    uint8_t rx_buf[2] = { 0 };
    size_t transferred;

    status = sertos_stream_buffer_create_static(&s_stream_buf, s_storage, 32U, 2U, &s_sb_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* Create high priority task (priority 5, while worker is priority 3) */
    cfg.name = "HighPrio";
    cfg.entry_func = worker_entry;
    cfg.param = NULL;
    cfg.priority = 5U;
    cfg.stack_buffer = high_stack;
    cfg.stack_size = sizeof(high_stack);
    status = sertos_task_create_static(&cfg, &high_tcb, &high_handle);
    TEST_ASSERT_EQUAL(SERTOS_STATUS_OK, status);

    /* Simulate high_tcb blocked on wait_recv */
    high_tcb.state = SERTOS_TASK_STATE_BLOCKED;
    high_tcb.wait_list = &s_sb_handle->wait_recv;
    (void)linked_list_insert_tail(&s_sb_handle->wait_recv, &high_tcb.event_node);

    /* Send from ISR meeting trigger level (2 bytes) */
    transferred = sertos_stream_buffer_send_from_isr(s_sb_handle, data, 2U, &higher_woken);
    TEST_ASSERT_EQUAL_UINT32(2U, (uint32_t)transferred);
    TEST_ASSERT_TRUE(higher_woken);

    /* Fill buffer completely (31 bytes usable) */
    while (sertos_stream_buffer_spaces_available(s_sb_handle) > 0U) {
        (void)sertos_stream_buffer_send(s_sb_handle, data, 1U, SERTOS_NO_WAIT);
    }
    TEST_ASSERT_TRUE(sertos_stream_buffer_is_full(s_sb_handle));

    /* Simulate high_tcb blocked on wait_send */
    higher_woken = false;
    high_tcb.state = SERTOS_TASK_STATE_BLOCKED;
    high_tcb.wait_list = &s_sb_handle->wait_send;
    (void)linked_list_insert_tail(&s_sb_handle->wait_send, &high_tcb.event_node);

    /* Receive from ISR frees space and unblocks high priority sender */
    transferred = sertos_stream_buffer_receive_from_isr(s_sb_handle, rx_buf, 2U, &higher_woken);
    TEST_ASSERT_EQUAL_UINT32(2U, (uint32_t)transferred);
    TEST_ASSERT_TRUE(higher_woken);
}

void test_stream_buffer_null_and_error_defenses(void)
{
    uint8_t dummy[4] = { 0 };

    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_bytes_available(NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_spaces_available(NULL));
    TEST_ASSERT_TRUE(sertos_stream_buffer_is_empty(NULL));
    TEST_ASSERT_FALSE(sertos_stream_buffer_is_full(NULL));

    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_send(NULL, dummy, 4U, SERTOS_NO_WAIT));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_receive(NULL, dummy, 4U, SERTOS_NO_WAIT));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_send_from_isr(NULL, dummy, 4U, NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, (uint32_t)sertos_stream_buffer_receive_from_isr(NULL, dummy, 4U, NULL));

    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_stream_buffer_reset(NULL));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_stream_buffer_set_trigger_level(NULL, 1U));
    TEST_ASSERT_EQUAL(SERTOS_STATUS_ERROR_INVALID_PARAM, sertos_stream_buffer_delete(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_stream_buffer_create_validation);
    RUN_TEST(test_stream_buffer_send_receive_fifo_ordering);
    RUN_TEST(test_stream_buffer_full_and_partial_transfer);
    RUN_TEST(test_stream_buffer_trigger_level_and_reset);
    RUN_TEST(test_stream_buffer_isr_transfers);
    RUN_TEST(test_stream_buffer_isr_higher_prio_woken);
    RUN_TEST(test_stream_buffer_null_and_error_defenses);
    return UNITY_END();
}
