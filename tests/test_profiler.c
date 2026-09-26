#include "unity.h"
#include "profiler.h"
#include "bitmap.h"
#include <stdint.h>
#include <stdbool.h>

/* Test the compiler instrumentation hooks directly without exposing them publicly. */
extern void __cyg_profile_func_enter(void* this_fn, void* call_site);
extern void __cyg_profile_func_exit(void* this_fn, void* call_site);

#define TEST_CAPACITY (16U)
static profiler_event_t s_test_buffer[TEST_CAPACITY];

/* External declarations of instrumented functions */
extern uint32_t profiler_test_target_add(uint32_t a, uint32_t b);
extern uint32_t profiler_test_target_sub(uint32_t a, uint32_t b);

void setUp(void)
{
    profiler_config_t config = {
        .frequency = 1000000U,
        .buffer    = s_test_buffer,
        .capacity  = TEST_CAPACITY
    };
    profiler_init(&config);
}

void tearDown(void)
{
    profiler_stop();
}

static void test_profiler_init_validation(void)
{
    profiler_config_t cfg;

    /* NULL config */
    profiler_init(NULL);
    TEST_ASSERT_EQUAL_UINT32(0U, profiler_capacity());
    TEST_ASSERT_EQUAL_UINT32(0U, profiler_frequency());
    TEST_ASSERT_FALSE(profiler_enabled());

    /* NULL buffer */
    cfg.frequency = 1000U;
    cfg.buffer = NULL;
    cfg.capacity = 16U;
    profiler_init(&cfg);
    TEST_ASSERT_EQUAL_UINT32(0U, profiler_capacity());

    /* Invalid capacities: 0, 1, non-power of two */
    cfg.buffer = s_test_buffer;
    cfg.capacity = 0U;
    profiler_init(&cfg);
    TEST_ASSERT_EQUAL_UINT32(0U, profiler_capacity());

    cfg.capacity = 1U;
    profiler_init(&cfg);
    TEST_ASSERT_EQUAL_UINT32(0U, profiler_capacity());

    cfg.capacity = 15U;
    profiler_init(&cfg);
    TEST_ASSERT_EQUAL_UINT32(0U, profiler_capacity());

    cfg.capacity = 100U;
    profiler_init(&cfg);
    TEST_ASSERT_EQUAL_UINT32(0U, profiler_capacity());

    /* Valid power-of-two */
    cfg.capacity = TEST_CAPACITY;
    profiler_init(&cfg);
    TEST_ASSERT_EQUAL_UINT32(TEST_CAPACITY, profiler_capacity());
    TEST_ASSERT_EQUAL_UINT32(1000U, profiler_frequency());
}

static void test_profiler_start_stop_gate(void)
{
    TEST_ASSERT_FALSE(profiler_enabled());

    /* Calling hook while disabled must not record anything */
    __cyg_profile_func_enter((void*)0x1234, (void*)0x5678);
    __cyg_profile_func_exit((void*)0x1234, (void*)0x5678);

    profiler_event_t ev;
    TEST_ASSERT_FALSE(profiler_read_event(0U, &ev));

    /* Enable and record */
    profiler_start();
    TEST_ASSERT_TRUE(profiler_enabled());

    __cyg_profile_func_enter((void*)0x1111, (void*)0x2222);
    TEST_ASSERT_TRUE(profiler_read_event(0U, &ev));
    TEST_ASSERT_EQUAL_PTR((void*)0x1111, ev.this);
    TEST_ASSERT_EQUAL_PTR((void*)0x2222, ev.call);
    TEST_ASSERT_EQUAL_UINT8(PROFILER_EVENT_ENTER, ev.event);

    /* Stop */
    profiler_stop();
    TEST_ASSERT_FALSE(profiler_enabled());

    __cyg_profile_func_enter((void*)0x3333, (void*)0x4444);
    TEST_ASSERT_FALSE(profiler_read_event(1U, &ev));
}

static void test_profiler_overflow_and_chronological_read(void)
{
    profiler_start();

    /* Write 16 events (fill exact capacity) */
    for (uint32_t i = 0U; i < TEST_CAPACITY; ++i) {
        __cyg_profile_func_enter((void*)(uintptr_t)(0x1000U + i), (void*)0x0);
    }
    TEST_ASSERT_FALSE(profiler_overflowed());

    profiler_event_t ev;
    TEST_ASSERT_TRUE(profiler_read_event(0U, &ev));
    TEST_ASSERT_EQUAL_PTR((void*)0x1000U, ev.this);

    /* 17th event causes overflow / wrap */
    __cyg_profile_func_enter((void*)0x9999U, (void*)0x0);
    TEST_ASSERT_TRUE(profiler_overflowed());

    /* Oldest retained event should now be the 2nd event written (0x1001) */
    TEST_ASSERT_TRUE(profiler_read_event(0U, &ev));
    TEST_ASSERT_EQUAL_PTR((void*)0x1001U, ev.this);

    /* Most recent event at index capacity - 1 should be 0x9999 */
    TEST_ASSERT_TRUE(profiler_read_event(TEST_CAPACITY - 1U, &ev));
    TEST_ASSERT_EQUAL_PTR((void*)0x9999U, ev.this);

    /* Index past capacity must return false */
    TEST_ASSERT_FALSE(profiler_read_event(TEST_CAPACITY, &ev));

    /* Reset */
    profiler_reset();
    TEST_ASSERT_FALSE(profiler_overflowed());
    TEST_ASSERT_FALSE(profiler_read_event(0U, &ev));
    TEST_ASSERT_TRUE(profiler_enabled());
}

static void test_profiler_bitmap_filtering(void)
{
    Bitmap filter;
    BitmapWord words[BITMAP_BITS_TO_WORDS(64U)];
    TEST_ASSERT_TRUE(bitmap_init(&filter, words, 64U));

    void* allowed_fn = (void*)0x100U;
    void* blocked_fn = (void*)0x108U;

    size_t allowed_hash = ((size_t)allowed_fn >> 2U) % 64U;
    bitmap_set_bit(&filter, allowed_hash);

    profiler_set_filter_bitmap(&filter);
    profiler_start();

    /* Call blocked function */
    __cyg_profile_func_enter(blocked_fn, (void*)0x0);
    profiler_event_t ev;
    TEST_ASSERT_FALSE(profiler_read_event(0U, &ev));

    /* Call allowed function */
    __cyg_profile_func_enter(allowed_fn, (void*)0x0);
    TEST_ASSERT_TRUE(profiler_read_event(0U, &ev));
    TEST_ASSERT_EQUAL_PTR(allowed_fn, ev.this);

    /* Disable filter */
    profiler_set_filter_bitmap(NULL);
    __cyg_profile_func_enter(blocked_fn, (void*)0x0);
    TEST_ASSERT_TRUE(profiler_read_event(1U, &ev));
    TEST_ASSERT_EQUAL_PTR(blocked_fn, ev.this);
}

static void test_profiler_instrumented_targets(void)
{
    profiler_start();

    uint32_t sum = profiler_test_target_add(10U, 20U);
    TEST_ASSERT_EQUAL_UINT32(30U, sum);

    uint32_t diff = profiler_test_target_sub(50U, 15U);
    TEST_ASSERT_EQUAL_UINT32(35U, diff);

    profiler_stop();

    /* We expect at least 4 events: enter/exit for add, enter/exit for sub */
    profiler_event_t ev0, ev1, ev2, ev3;
    TEST_ASSERT_TRUE(profiler_read_event(0U, &ev0));
    TEST_ASSERT_TRUE(profiler_read_event(1U, &ev1));
    TEST_ASSERT_TRUE(profiler_read_event(2U, &ev2));
    TEST_ASSERT_TRUE(profiler_read_event(3U, &ev3));

    TEST_ASSERT_EQUAL_UINT8(PROFILER_EVENT_ENTER, ev0.event);
    TEST_ASSERT_EQUAL_UINT8(PROFILER_EVENT_EXIT, ev1.event);
    TEST_ASSERT_EQUAL_PTR(ev0.this, ev1.this);

    TEST_ASSERT_EQUAL_UINT8(PROFILER_EVENT_ENTER, ev2.event);
    TEST_ASSERT_EQUAL_UINT8(PROFILER_EVENT_EXIT, ev3.event);
    TEST_ASSERT_EQUAL_PTR(ev2.this, ev3.this);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_profiler_init_validation);
    RUN_TEST(test_profiler_start_stop_gate);
    RUN_TEST(test_profiler_overflow_and_chronological_read);
    RUN_TEST(test_profiler_bitmap_filtering);
    RUN_TEST(test_profiler_instrumented_targets);
    return UNITY_END();
}
