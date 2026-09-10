#include "unity.h"
#include "ring_buffer.h"
#include <stdint.h>
#include <stdbool.h>

void setUp(void)
{
    /* Global test setup, empty as each test case manages its own buffer instance */
}

void tearDown(void)
{
    /* Global test teardown, empty as no persistent state needs cleanup */
}

static void test_ring_buffer_init_success(void)
{
    RingBuffer rb;
    uint8_t buffer_16[16];
    uint8_t buffer_256[256];

    /* Test initialization with power-of-two capacity 16 */
    TEST_ASSERT_TRUE(ring_buffer_init(&rb, buffer_16, 16U));
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
    TEST_ASSERT_FALSE(ring_buffer_is_full(&rb));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_count(&rb));
    TEST_ASSERT_EQUAL_UINT(15U, ring_buffer_capacity(&rb));

    /* Test initialization with power-of-two capacity 256 */
    TEST_ASSERT_TRUE(ring_buffer_init(&rb, buffer_256, 256U));
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
    TEST_ASSERT_FALSE(ring_buffer_is_full(&rb));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_count(&rb));
    TEST_ASSERT_EQUAL_UINT(255U, ring_buffer_capacity(&rb));
}

static void test_ring_buffer_init_invalid_params(void)
{
    RingBuffer rb;
    uint8_t buffer[32];

    /* NULL pointer validation */
    TEST_ASSERT_FALSE(ring_buffer_init(NULL, buffer, 16U));
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, NULL, 16U));
    TEST_ASSERT_FALSE(ring_buffer_init(NULL, NULL, 16U));

    /* Non-power-of-two and boundary capacities */
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, buffer, 0U));
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, buffer, 1U));
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, buffer, 3U));
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, buffer, 7U));
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, buffer, 15U));
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, buffer, 30U));
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, buffer, 100U));
    TEST_ASSERT_FALSE(ring_buffer_init(&rb, buffer, 255U));
}

static void test_ring_buffer_core_fifo_logic(void)
{
    RingBuffer rb;
    uint8_t storage[16];
    uint8_t read_byte = 0U;

    TEST_ASSERT_TRUE(ring_buffer_init(&rb, storage, 16U));

    /* Single byte push and pop */
    TEST_ASSERT_TRUE(ring_buffer_push(&rb, 0xAAU));
    TEST_ASSERT_EQUAL_UINT(1U, ring_buffer_count(&rb));
    TEST_ASSERT_FALSE(ring_buffer_is_empty(&rb));

    TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &read_byte));
    TEST_ASSERT_EQUAL_HEX8(0xAAU, read_byte);
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_count(&rb));
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));

    /* Multi-byte sequence in strict FIFO order */
    const uint8_t test_sequence[5] = {0x10U, 0x20U, 0x30U, 0x40U, 0x50U};

    for (size_t i = 0U; i < 5U; ++i) {
        TEST_ASSERT_TRUE(ring_buffer_push(&rb, test_sequence[i]));
        TEST_ASSERT_EQUAL_UINT(i + 1U, ring_buffer_count(&rb));
    }

    for (size_t i = 0U; i < 5U; ++i) {
        read_byte = 0U;
        TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &read_byte));
        TEST_ASSERT_EQUAL_HEX8(test_sequence[i], read_byte);
        TEST_ASSERT_EQUAL_UINT(5U - 1U - i, ring_buffer_count(&rb));
    }

    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
}

static void test_ring_buffer_boundary_full(void)
{
    RingBuffer rb;
    uint8_t storage[16];
    uint8_t read_byte = 0U;

    TEST_ASSERT_TRUE(ring_buffer_init(&rb, storage, 16U));
    TEST_ASSERT_EQUAL_UINT(15U, ring_buffer_capacity(&rb));

    /* Fill buffer up to usable capacity (capacity - 1 = 15) */
    for (uint8_t i = 0U; i < 15U; ++i) {
        TEST_ASSERT_FALSE(ring_buffer_is_full(&rb));
        TEST_ASSERT_TRUE(ring_buffer_push(&rb, i));
        TEST_ASSERT_EQUAL_UINT((size_t)(i + 1U), ring_buffer_count(&rb));
    }

    /* Buffer must now report full */
    TEST_ASSERT_TRUE(ring_buffer_is_full(&rb));
    TEST_ASSERT_FALSE(ring_buffer_is_empty(&rb));
    TEST_ASSERT_EQUAL_UINT(15U, ring_buffer_count(&rb));

    /* Overflow write must gracefully fail */
    TEST_ASSERT_FALSE(ring_buffer_push(&rb, 0xFFU));
    TEST_ASSERT_TRUE(ring_buffer_is_full(&rb));
    TEST_ASSERT_EQUAL_UINT(15U, ring_buffer_count(&rb));

    /* Verify all existing data is intact after failed overflow */
    for (uint8_t i = 0U; i < 15U; ++i) {
        TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &read_byte));
        TEST_ASSERT_EQUAL_UINT8(i, read_byte);
    }

    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
    TEST_ASSERT_FALSE(ring_buffer_is_full(&rb));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_count(&rb));
}

static void test_ring_buffer_boundary_empty(void)
{
    RingBuffer rb;
    uint8_t storage[8];
    uint8_t read_byte = 0x55U;

    TEST_ASSERT_TRUE(ring_buffer_init(&rb, storage, 8U));

    /* Initial empty state */
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
    TEST_ASSERT_FALSE(ring_buffer_is_full(&rb));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_count(&rb));

    /* Underflow pop must gracefully fail and leave variable intact */
    TEST_ASSERT_FALSE(ring_buffer_pop(&rb, &read_byte));
    TEST_ASSERT_EQUAL_HEX8(0x55U, read_byte);
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));

    /* Transition from empty -> active -> empty */
    TEST_ASSERT_TRUE(ring_buffer_push(&rb, 0xAAU));
    TEST_ASSERT_FALSE(ring_buffer_is_empty(&rb));

    TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &read_byte));
    TEST_ASSERT_EQUAL_HEX8(0xAAU, read_byte);
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));

    /* Repeated underflow pop */
    read_byte = 0x55U;
    TEST_ASSERT_FALSE(ring_buffer_pop(&rb, &read_byte));
    TEST_ASSERT_EQUAL_HEX8(0x55U, read_byte);
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
}

static void test_ring_buffer_wraparound(void)
{
    RingBuffer rb;
    /* Small buffer of capacity 8 (usable = 7) to force frequent index wrap-around */
    uint8_t storage[8];
    uint8_t read_byte = 0U;

    TEST_ASSERT_TRUE(ring_buffer_init(&rb, storage, 8U));

    /*
     * Execute 500 iterations of pushing 3 elements and popping 3 elements.
     * Total elements pushed/popped: 1500 elements.
     * Head and tail will cross the physical 8-byte boundary approximately 187 times.
     */
    uint8_t sequence_counter = 0U;

    for (uint16_t iter = 0U; iter < 500U; ++iter) {
        uint8_t b1 = sequence_counter;
        sequence_counter++;
        uint8_t b2 = sequence_counter;
        sequence_counter++;
        uint8_t b3 = sequence_counter;
        sequence_counter++;

        TEST_ASSERT_TRUE(ring_buffer_push(&rb, b1));
        TEST_ASSERT_TRUE(ring_buffer_push(&rb, b2));
        TEST_ASSERT_TRUE(ring_buffer_push(&rb, b3));

        TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &read_byte));
        TEST_ASSERT_EQUAL_UINT8(b1, read_byte);

        TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &read_byte));
        TEST_ASSERT_EQUAL_UINT8(b2, read_byte);

        TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &read_byte));
        TEST_ASSERT_EQUAL_UINT8(b3, read_byte);

        TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
        TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_count(&rb));
    }

    /*
     * Asymmetric wrap-around test:
     * Push 4 bytes, pop 4 bytes across 50 cycles with continuous counter.
     */
    uint8_t expected_val = sequence_counter;

    for (uint8_t cycle = 0U; cycle < 50U; ++cycle) {
        for (uint8_t i = 0U; i < 4U; ++i) {
            TEST_ASSERT_TRUE(ring_buffer_push(&rb, sequence_counter));
            sequence_counter++;
        }

        for (uint8_t i = 0U; i < 4U; ++i) {
            TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &read_byte));
            TEST_ASSERT_EQUAL_UINT8(expected_val, read_byte);
            expected_val++;
        }

        TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
    }
}

static void test_ring_buffer_bulk_transfer(void)
{
    RingBuffer rb;
    uint8_t storage[16];
    uint8_t write_data[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    uint8_t read_data[16] = { 0 };

    TEST_ASSERT_TRUE(ring_buffer_init(&rb, storage, 16U));

    /* NULL and zero count checks */
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_write(NULL, write_data, 5U));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_write(&rb, NULL, 5U));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_write(&rb, write_data, 0U));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_read(NULL, read_data, 5U));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_read(&rb, NULL, 5U));
    TEST_ASSERT_EQUAL_UINT(0U, ring_buffer_read(&rb, read_data, 0U));

    /* Bulk write 10 bytes */
    TEST_ASSERT_EQUAL_UINT(10U, ring_buffer_write(&rb, write_data, 10U));
    TEST_ASSERT_EQUAL_UINT(10U, ring_buffer_count(&rb));

    /* Attempt to write more than available capacity (max 15 bytes, 5 left) */
    TEST_ASSERT_EQUAL_UINT(5U, ring_buffer_write(&rb, write_data, 10U));
    TEST_ASSERT_TRUE(ring_buffer_is_full(&rb));

    /* Read 8 bytes */
    TEST_ASSERT_EQUAL_UINT(8U, ring_buffer_read(&rb, read_data, 8U));
    for (size_t i = 0U; i < 8U; ++i) {
        TEST_ASSERT_EQUAL_UINT8(write_data[i], read_data[i]);
    }

    /* Wrap-around bulk write across boundary */
    TEST_ASSERT_EQUAL_UINT(6U, ring_buffer_write(&rb, write_data, 6U));

    /* Read remaining bytes */
    size_t remaining = ring_buffer_count(&rb);
    TEST_ASSERT_EQUAL_UINT(remaining, ring_buffer_read(&rb, read_data, remaining));
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ring_buffer_init_success);
    RUN_TEST(test_ring_buffer_init_invalid_params);
    RUN_TEST(test_ring_buffer_core_fifo_logic);
    RUN_TEST(test_ring_buffer_boundary_full);
    RUN_TEST(test_ring_buffer_boundary_empty);
    RUN_TEST(test_ring_buffer_wraparound);
    RUN_TEST(test_ring_buffer_bulk_transfer);

    return UNITY_END();
}
