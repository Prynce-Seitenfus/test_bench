#include "unity.h"
#include "fsm.h"
#include <stdint.h>
#include <stdbool.h>

void setUp(void)
{
    /* Global setup, empty as each test initializes its own FSM */
}

void tearDown(void)
{
    /* Global teardown, empty as no persistent state needs cleanup */
}

/* Test states and events */
typedef enum {
    STATE_IDLE = 10,
    STATE_ACTIVE = 20,
    STATE_PAUSED = 30
} TestState;

typedef enum {
    EVENT_START = 1,
    EVENT_PAUSE = 2,
    EVENT_RESUME = 3,
    EVENT_STOP = 4,
    EVENT_TICK = 5,
    EVENT_UNHANDLED = 99
} TestEvent;

typedef struct {
    uint32_t trace_count;
    uint32_t trace_log[16];
    bool allow_transition;
} TestContext;

#define TRACE_IDLE_EXIT    (0x01U)
#define TRACE_ACTIVE_ENTER (0x02U)
#define TRACE_ACTIVE_EXIT  (0x03U)
#define TRACE_ACTION_START (0x04U)
#define TRACE_ACTION_TICK  (0x05U)
#define TRACE_PAUSED_ENTER (0x06U)

static void record_trace(TestContext* ctx, uint32_t step)
{
    if ((ctx != NULL) && (ctx->trace_count < 16U)) {
        ctx->trace_log[ctx->trace_count] = step;
        ctx->trace_count++;
    }
}

static void on_idle_exit(void* context, FsmEventId event)
{
    (void)event;
    record_trace((TestContext*)context, TRACE_IDLE_EXIT);
}

static void on_active_enter(void* context, FsmEventId event)
{
    (void)event;
    record_trace((TestContext*)context, TRACE_ACTIVE_ENTER);
}

static void on_active_exit(void* context, FsmEventId event)
{
    (void)event;
    record_trace((TestContext*)context, TRACE_ACTIVE_EXIT);
}

static void on_paused_enter(void* context, FsmEventId event)
{
    (void)event;
    record_trace((TestContext*)context, TRACE_PAUSED_ENTER);
}

static void action_start(void* context, FsmEventId event)
{
    (void)event;
    record_trace((TestContext*)context, TRACE_ACTION_START);
}

static void action_tick(void* context, FsmEventId event)
{
    (void)event;
    record_trace((TestContext*)context, TRACE_ACTION_TICK);
}

static bool guard_allow(void* context, FsmEventId event)
{
    (void)event;
    TestContext* ctx = (TestContext*)context;
    return (ctx != NULL) ? ctx->allow_transition : true;
}

static bool guard_always_deny(void* context, FsmEventId event)
{
    (void)context;
    (void)event;
    return false;
}

static bool guard_always_allow(void* context, FsmEventId event)
{
    (void)context;
    (void)event;
    return true;
}

static void test_fsm_init_valid(void)
{
    Fsm fsm;
    static const FsmTransition transitions[] = {
        { STATE_IDLE, EVENT_START, NULL, NULL, STATE_ACTIVE }
    };

    FsmStatus status = fsm_init(&fsm, transitions, 1U, NULL, 0U, STATE_IDLE, NULL);
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_IDLE, fsm_get_state(&fsm));
    TEST_ASSERT_TRUE(fsm_is_state(&fsm, STATE_IDLE));
    TEST_ASSERT_FALSE(fsm_is_state(&fsm, STATE_ACTIVE));
}

static void test_fsm_init_invalid_params(void)
{
    Fsm fsm;
    static const FsmTransition transitions[] = {
        { STATE_IDLE, EVENT_START, NULL, NULL, STATE_ACTIVE }
    };

    TEST_ASSERT_EQUAL(FSM_STATUS_ERROR_NULL_PTR, fsm_init(NULL, transitions, 1U, NULL, 0U, STATE_IDLE, NULL));
    TEST_ASSERT_EQUAL(FSM_STATUS_ERROR_NULL_PTR, fsm_init(&fsm, NULL, 1U, NULL, 0U, STATE_IDLE, NULL));
    TEST_ASSERT_EQUAL(FSM_STATUS_ERROR_INVALID_PARAM, fsm_init(&fsm, transitions, 0U, NULL, 0U, STATE_IDLE, NULL));
    TEST_ASSERT_EQUAL(FSM_STATUS_ERROR_INVALID_PARAM, fsm_init(&fsm, transitions, 1U, NULL, 2U, STATE_IDLE, NULL));
}

static void test_fsm_null_queries(void)
{
    TEST_ASSERT_EQUAL_UINT32(0U, fsm_get_state(NULL));
    TEST_ASSERT_FALSE(fsm_is_state(NULL, STATE_IDLE));
    TEST_ASSERT_EQUAL(FSM_STATUS_ERROR_NULL_PTR, fsm_dispatch(NULL, EVENT_START));

    Fsm invalid_fsm;
    invalid_fsm.transitions = NULL;
    invalid_fsm.transition_count = 0U;
    TEST_ASSERT_EQUAL(FSM_STATUS_ERROR_INVALID_PARAM, fsm_dispatch(&invalid_fsm, EVENT_START));
}

static void test_fsm_transition_lifecycle_and_hooks(void)
{
    Fsm fsm;
    TestContext ctx = { 0U, { 0U }, true };

    static const FsmStateDesc states[] = {
        { STATE_IDLE,   NULL,            on_idle_exit },
        { STATE_ACTIVE, on_active_enter, on_active_exit },
        { STATE_PAUSED, on_paused_enter, NULL }
    };

    static const FsmTransition transitions[] = {
        { STATE_IDLE,   EVENT_START, guard_allow, action_start, STATE_ACTIVE },
        { STATE_ACTIVE, EVENT_TICK,  NULL,        action_tick,  STATE_ACTIVE },
        { STATE_ACTIVE, EVENT_PAUSE, NULL,        NULL,         STATE_PAUSED }
    };

    TEST_ASSERT_EQUAL(FSM_STATUS_OK, fsm_init(&fsm, transitions, 3U, states, 3U, STATE_IDLE, &ctx));

    /* 1. Transition STATE_IDLE -> STATE_ACTIVE */
    FsmStatus status = fsm_dispatch(&fsm, EVENT_START);
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_ACTIVE, fsm_get_state(&fsm));

    /* Check lifecycle order: idle_exit -> action_start -> active_enter */
    TEST_ASSERT_EQUAL_UINT32(3U, ctx.trace_count);
    TEST_ASSERT_EQUAL_UINT32(TRACE_IDLE_EXIT, ctx.trace_log[0]);
    TEST_ASSERT_EQUAL_UINT32(TRACE_ACTION_START, ctx.trace_log[1]);
    TEST_ASSERT_EQUAL_UINT32(TRACE_ACTIVE_ENTER, ctx.trace_log[2]);

    /* 2. Self-transition in STATE_ACTIVE (TICK) */
    ctx.trace_count = 0U;
    status = fsm_dispatch(&fsm, EVENT_TICK);
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_ACTIVE, fsm_get_state(&fsm));
    /* Only action_tick should be called; neither on_exit nor on_enter */
    TEST_ASSERT_EQUAL_UINT32(1U, ctx.trace_count);
    TEST_ASSERT_EQUAL_UINT32(TRACE_ACTION_TICK, ctx.trace_log[0]);

    /* 3. Transition STATE_ACTIVE -> STATE_PAUSED (no action hook, but exit/enter hooks) */
    ctx.trace_count = 0U;
    status = fsm_dispatch(&fsm, EVENT_PAUSE);
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_PAUSED, fsm_get_state(&fsm));
    TEST_ASSERT_EQUAL_UINT32(2U, ctx.trace_count);
    TEST_ASSERT_EQUAL_UINT32(TRACE_ACTIVE_EXIT, ctx.trace_log[0]);
    TEST_ASSERT_EQUAL_UINT32(TRACE_PAUSED_ENTER, ctx.trace_log[1]);
}

static void test_fsm_guard_rejection_and_multi_branch(void)
{
    Fsm fsm;
    TestContext ctx = { 0U, { 0U }, false };

    static const FsmTransition transitions[] = {
        /* Single guarded transition */
        { STATE_IDLE, EVENT_START, guard_allow, NULL, STATE_ACTIVE }
    };

    TEST_ASSERT_EQUAL(FSM_STATUS_OK, fsm_init(&fsm, transitions, 1U, NULL, 0U, STATE_IDLE, &ctx));

    /* Guard rejects because ctx.allow_transition == false */
    FsmStatus status = fsm_dispatch(&fsm, EVENT_START);
    TEST_ASSERT_EQUAL(FSM_STATUS_GUARD_REJECTED, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_IDLE, fsm_get_state(&fsm));

    /* Now allow it */
    ctx.allow_transition = true;
    status = fsm_dispatch(&fsm, EVENT_START);
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_ACTIVE, fsm_get_state(&fsm));
}

static void test_fsm_multi_guarded_branches(void)
{
    Fsm fsm;

    static const FsmTransition branch_transitions[] = {
        { STATE_IDLE, EVENT_START, guard_always_deny,  NULL, STATE_PAUSED },
        { STATE_IDLE, EVENT_START, guard_always_allow, NULL, STATE_ACTIVE }
    };

    TEST_ASSERT_EQUAL(FSM_STATUS_OK, fsm_init(&fsm, branch_transitions, 2U, NULL, 0U, STATE_IDLE, NULL));

    /* First rule fails guard, second rule succeeds -> advances to STATE_ACTIVE */
    FsmStatus status = fsm_dispatch(&fsm, EVENT_START);
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_ACTIVE, fsm_get_state(&fsm));
}

static void test_fsm_unhandled_event(void)
{
    Fsm fsm;
    static const FsmTransition transitions[] = {
        { STATE_IDLE, EVENT_START, NULL, NULL, STATE_ACTIVE }
    };

    TEST_ASSERT_EQUAL(FSM_STATUS_OK, fsm_init(&fsm, transitions, 1U, NULL, 0U, STATE_IDLE, NULL));

    FsmStatus status = fsm_dispatch(&fsm, EVENT_UNHANDLED);
    TEST_ASSERT_EQUAL(FSM_STATUS_NO_TRANSITION, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_IDLE, fsm_get_state(&fsm));
}

static void test_fsm_transition_without_state_descriptors(void)
{
    Fsm fsm;
    static const FsmTransition transitions[] = {
        { STATE_IDLE, EVENT_START, NULL, NULL, STATE_ACTIVE }
    };

    /* NULL state descriptors array */
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, fsm_init(&fsm, transitions, 1U, NULL, 0U, STATE_IDLE, NULL));

    FsmStatus status = fsm_dispatch(&fsm, EVENT_START);
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(STATE_ACTIVE, fsm_get_state(&fsm));
}

static void test_fsm_state_desc_not_found(void)
{
    Fsm fsm;
    static const FsmStateDesc partial_states[] = {
        { STATE_IDLE, NULL, NULL }
    };
    static const FsmTransition transitions[] = {
        { STATE_IDLE, EVENT_START, NULL, NULL, STATE_ACTIVE }
    };

    /* STATE_ACTIVE has no descriptor in partial_states */
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, fsm_init(&fsm, transitions, 1U, partial_states, 1U, STATE_IDLE, NULL));
    TEST_ASSERT_EQUAL(FSM_STATUS_OK, fsm_dispatch(&fsm, EVENT_START));
    TEST_ASSERT_EQUAL_UINT32(STATE_ACTIVE, fsm_get_state(&fsm));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_fsm_init_valid);
    RUN_TEST(test_fsm_init_invalid_params);
    RUN_TEST(test_fsm_null_queries);
    RUN_TEST(test_fsm_transition_lifecycle_and_hooks);
    RUN_TEST(test_fsm_guard_rejection_and_multi_branch);
    RUN_TEST(test_fsm_multi_guarded_branches);
    RUN_TEST(test_fsm_unhandled_event);
    RUN_TEST(test_fsm_transition_without_state_descriptors);
    RUN_TEST(test_fsm_state_desc_not_found);

    return UNITY_END();
}
