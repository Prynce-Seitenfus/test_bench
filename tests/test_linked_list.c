#include "unity.h"
#include "linked_list.h"
#include <stdint.h>
#include <stdbool.h>

void setUp(void)
{
    /* Global setup, empty as each test initializes its own list instance */
}

void tearDown(void)
{
    /* Global teardown, empty as no persistent state needs cleanup */
}

static void test_linked_list_init_success(void)
{
    LinkedList list;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_is_empty(&list));
    TEST_ASSERT_EQUAL_UINT(0U, linked_list_count(&list));
    TEST_ASSERT_NULL(linked_list_peek_head(&list));
    TEST_ASSERT_NULL(linked_list_peek_tail(&list));
}

static void test_linked_list_init_null(void)
{
    TEST_ASSERT_FALSE(linked_list_init(NULL));
}

static void test_linked_list_node_init_and_state(void)
{
    LinkedListNode node;

    TEST_ASSERT_TRUE(linked_list_node_init(&node));
    TEST_ASSERT_FALSE(linked_list_node_is_linked(&node));

    /* NULL validations */
    TEST_ASSERT_FALSE(linked_list_node_init(NULL));
    TEST_ASSERT_FALSE(linked_list_node_is_linked(NULL));
}

static void test_linked_list_insert_head(void)
{
    LinkedList list;
    LinkedListNode node1;
    LinkedListNode node2;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&node1));
    TEST_ASSERT_TRUE(linked_list_node_init(&node2));

    /* NULL checks */
    TEST_ASSERT_FALSE(linked_list_insert_head(NULL, &node1));
    TEST_ASSERT_FALSE(linked_list_insert_head(&list, NULL));

    /* Insert first node */
    TEST_ASSERT_TRUE(linked_list_insert_head(&list, &node1));
    TEST_ASSERT_FALSE(linked_list_is_empty(&list));
    TEST_ASSERT_EQUAL_UINT(1U, linked_list_count(&list));
    TEST_ASSERT_TRUE(linked_list_node_is_linked(&node1));
    TEST_ASSERT_EQUAL_PTR(&node1, linked_list_peek_head(&list));
    TEST_ASSERT_EQUAL_PTR(&node1, linked_list_peek_tail(&list));

    /* Prevent double insertion of the same node */
    TEST_ASSERT_FALSE(linked_list_insert_head(&list, &node1));

    /* Insert second node at head */
    TEST_ASSERT_TRUE(linked_list_insert_head(&list, &node2));
    TEST_ASSERT_EQUAL_UINT(2U, linked_list_count(&list));
    TEST_ASSERT_EQUAL_PTR(&node2, linked_list_peek_head(&list));
    TEST_ASSERT_EQUAL_PTR(&node1, linked_list_peek_tail(&list));
}

static void test_linked_list_insert_tail(void)
{
    LinkedList list;
    LinkedListNode node1;
    LinkedListNode node2;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&node1));
    TEST_ASSERT_TRUE(linked_list_node_init(&node2));

    /* NULL checks */
    TEST_ASSERT_FALSE(linked_list_insert_tail(NULL, &node1));
    TEST_ASSERT_FALSE(linked_list_insert_tail(&list, NULL));

    /* Insert node1 then node2 at tail */
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node1));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node2));

    TEST_ASSERT_EQUAL_UINT(2U, linked_list_count(&list));
    TEST_ASSERT_EQUAL_PTR(&node1, linked_list_peek_head(&list));
    TEST_ASSERT_EQUAL_PTR(&node2, linked_list_peek_tail(&list));

    /* Prevent duplicate insertion */
    TEST_ASSERT_FALSE(linked_list_insert_tail(&list, &node2));
}

static void test_linked_list_insert_before_and_after(void)
{
    LinkedList list;
    LinkedListNode node_a;
    LinkedListNode node_b;
    LinkedListNode node_c;
    LinkedListNode unlinked_node;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&node_a));
    TEST_ASSERT_TRUE(linked_list_node_init(&node_b));
    TEST_ASSERT_TRUE(linked_list_node_init(&node_c));
    TEST_ASSERT_TRUE(linked_list_node_init(&unlinked_node));

    /* Add node_a */
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node_a));

    /* Invalid insertions */
    TEST_ASSERT_FALSE(linked_list_insert_before(NULL, &node_a, &node_b));
    TEST_ASSERT_FALSE(linked_list_insert_before(&list, NULL, &node_b));
    TEST_ASSERT_FALSE(linked_list_insert_before(&list, &node_a, NULL));
    TEST_ASSERT_FALSE(linked_list_insert_before(&list, &unlinked_node, &node_b));

    TEST_ASSERT_FALSE(linked_list_insert_after(NULL, &node_a, &node_c));
    TEST_ASSERT_FALSE(linked_list_insert_after(&list, NULL, &node_c));
    TEST_ASSERT_FALSE(linked_list_insert_after(&list, &node_a, NULL));
    TEST_ASSERT_FALSE(linked_list_insert_after(&list, &unlinked_node, &node_c));

    /* Insert node_b before node_a */
    TEST_ASSERT_TRUE(linked_list_insert_before(&list, &node_a, &node_b));

    /* Insert node_c after node_a */
    TEST_ASSERT_TRUE(linked_list_insert_after(&list, &node_a, &node_c));

    /* List should now be: node_b -> node_a -> node_c */
    TEST_ASSERT_EQUAL_UINT(3U, linked_list_count(&list));
    TEST_ASSERT_EQUAL_PTR(&node_b, linked_list_peek_head(&list));
    TEST_ASSERT_EQUAL_PTR(&node_c, linked_list_peek_tail(&list));

    TEST_ASSERT_EQUAL_PTR(&node_a, linked_list_next(&list, &node_b));
    TEST_ASSERT_EQUAL_PTR(&node_c, linked_list_next(&list, &node_a));
    TEST_ASSERT_NULL(linked_list_next(&list, &node_c));
}

static void test_linked_list_remove(void)
{
    LinkedList list;
    LinkedListNode node1;
    LinkedListNode node2;
    LinkedListNode node3;
    LinkedListNode external_node;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&node1));
    TEST_ASSERT_TRUE(linked_list_node_init(&node2));
    TEST_ASSERT_TRUE(linked_list_node_init(&node3));
    TEST_ASSERT_TRUE(linked_list_node_init(&external_node));

    /* Remove from empty list */
    TEST_ASSERT_FALSE(linked_list_remove(&list, &node1));

    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node1));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node2));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node3));

    /* Invalid parameters */
    TEST_ASSERT_FALSE(linked_list_remove(NULL, &node2));
    TEST_ASSERT_FALSE(linked_list_remove(&list, NULL));
    TEST_ASSERT_FALSE(linked_list_remove(&list, &external_node));

    /* Remove middle node (node2) */
    TEST_ASSERT_TRUE(linked_list_remove(&list, &node2));
    TEST_ASSERT_EQUAL_UINT(2U, linked_list_count(&list));
    TEST_ASSERT_FALSE(linked_list_node_is_linked(&node2));

    /* Verify node1 -> node3 */
    TEST_ASSERT_EQUAL_PTR(&node3, linked_list_next(&list, &node1));
    TEST_ASSERT_EQUAL_PTR(&node1, linked_list_prev(&list, &node3));

    /* Remove head (node1) */
    TEST_ASSERT_TRUE(linked_list_remove(&list, &node1));
    TEST_ASSERT_EQUAL_UINT(1U, linked_list_count(&list));
    TEST_ASSERT_EQUAL_PTR(&node3, linked_list_peek_head(&list));

    /* Remove remaining node (node3) */
    TEST_ASSERT_TRUE(linked_list_remove(&list, &node3));
    TEST_ASSERT_EQUAL_UINT(0U, linked_list_count(&list));
    TEST_ASSERT_TRUE(linked_list_is_empty(&list));
}

static void test_linked_list_pop_head_and_tail(void)
{
    LinkedList list;
    LinkedListNode node1;
    LinkedListNode node2;
    LinkedListNode node3;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&node1));
    TEST_ASSERT_TRUE(linked_list_node_init(&node2));
    TEST_ASSERT_TRUE(linked_list_node_init(&node3));

    /* Pop on empty list */
    TEST_ASSERT_NULL(linked_list_pop_head(&list));
    TEST_ASSERT_NULL(linked_list_pop_tail(&list));
    TEST_ASSERT_NULL(linked_list_pop_head(NULL));
    TEST_ASSERT_NULL(linked_list_pop_tail(NULL));

    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node1));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node2));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node3));

    /* Pop head -> returns node1 */
    TEST_ASSERT_EQUAL_PTR(&node1, linked_list_pop_head(&list));
    TEST_ASSERT_FALSE(linked_list_node_is_linked(&node1));
    TEST_ASSERT_EQUAL_UINT(2U, linked_list_count(&list));

    /* Pop tail -> returns node3 */
    TEST_ASSERT_EQUAL_PTR(&node3, linked_list_pop_tail(&list));
    TEST_ASSERT_FALSE(linked_list_node_is_linked(&node3));
    TEST_ASSERT_EQUAL_UINT(1U, linked_list_count(&list));

    /* Pop head -> returns node2 */
    TEST_ASSERT_EQUAL_PTR(&node2, linked_list_pop_head(&list));
    TEST_ASSERT_TRUE(linked_list_is_empty(&list));
}

static void test_linked_list_peek_and_traversal(void)
{
    LinkedList list;
    LinkedListNode node1;
    LinkedListNode node2;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&node1));
    TEST_ASSERT_TRUE(linked_list_node_init(&node2));

    /* Peek on NULL */
    TEST_ASSERT_NULL(linked_list_peek_head(NULL));
    TEST_ASSERT_NULL(linked_list_peek_tail(NULL));

    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node1));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node2));

    /* Traversal validation */
    TEST_ASSERT_NULL(linked_list_next(NULL, &node1));
    TEST_ASSERT_NULL(linked_list_next(&list, NULL));
    TEST_ASSERT_EQUAL_PTR(&node2, linked_list_next(&list, &node1));
    TEST_ASSERT_NULL(linked_list_next(&list, &node2));

    TEST_ASSERT_NULL(linked_list_prev(NULL, &node2));
    TEST_ASSERT_NULL(linked_list_prev(&list, NULL));
    TEST_ASSERT_EQUAL_PTR(&node1, linked_list_prev(&list, &node2));
    TEST_ASSERT_NULL(linked_list_prev(&list, &node1));
}

static void test_linked_list_clear(void)
{
    LinkedList list;
    LinkedListNode node1;
    LinkedListNode node2;
    LinkedListNode node3;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&node1));
    TEST_ASSERT_TRUE(linked_list_node_init(&node2));
    TEST_ASSERT_TRUE(linked_list_node_init(&node3));

    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node1));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node2));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &node3));

    /* Clear NULL list should safely do nothing */
    linked_list_clear(NULL);

    /* Clear populated list */
    linked_list_clear(&list);
    TEST_ASSERT_TRUE(linked_list_is_empty(&list));
    TEST_ASSERT_EQUAL_UINT(0U, linked_list_count(&list));

    /* Verify all nodes were marked unlinked */
    TEST_ASSERT_FALSE(linked_list_node_is_linked(&node1));
    TEST_ASSERT_FALSE(linked_list_node_is_linked(&node2));
    TEST_ASSERT_FALSE(linked_list_node_is_linked(&node3));
}

typedef struct TestTask {
    uint32_t task_id;
    uint8_t priority;
    LinkedListNode node;
} TestTask;

static void test_linked_list_container_of(void)
{
    LinkedList list;
    TestTask task1 = { .task_id = 101U, .priority = 2U };
    TestTask task2 = { .task_id = 102U, .priority = 5U };

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&task1.node));
    TEST_ASSERT_TRUE(linked_list_node_init(&task2.node));

    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &task1.node));
    TEST_ASSERT_TRUE(linked_list_insert_tail(&list, &task2.node));

    LinkedListNode* popped_node = linked_list_pop_head(&list);
    TEST_ASSERT_NOT_NULL(popped_node);

    TestTask* recovered_task = LINKED_LIST_CONTAINER_OF(popped_node, TestTask, node);
    TEST_ASSERT_EQUAL_UINT32(101U, recovered_task->task_id);
    TEST_ASSERT_EQUAL_UINT8(2U, recovered_task->priority);
}

static void test_linked_list_direct_operations(void)
{
    LinkedList list;
    LinkedListNode node1;
    LinkedListNode node2;
    LinkedListNode node3;

    TEST_ASSERT_TRUE(linked_list_init(&list));
    TEST_ASSERT_TRUE(linked_list_node_init(&node1));
    TEST_ASSERT_TRUE(linked_list_node_init(&node2));
    TEST_ASSERT_TRUE(linked_list_node_init(&node3));

    /* Direct insert tail */
    linked_list_insert_tail_direct(&list, &node1);
    TEST_ASSERT_EQUAL_UINT(1U, list.count);
    TEST_ASSERT_EQUAL_PTR(&node1, list.root.next);

    /* Direct insert head */
    linked_list_insert_head_direct(&list, &node2);
    TEST_ASSERT_EQUAL_UINT(2U, list.count);
    TEST_ASSERT_EQUAL_PTR(&node2, list.root.next);
    TEST_ASSERT_EQUAL_PTR(&node1, list.root.prev);

    /* Direct insert tail */
    linked_list_insert_tail_direct(&list, &node3);
    TEST_ASSERT_EQUAL_UINT(3U, list.count);

    /* Direct remove middle (node1) */
    linked_list_remove_direct(&list, &node1);
    TEST_ASSERT_EQUAL_UINT(2U, list.count);
    TEST_ASSERT_FALSE(linked_list_node_is_linked(&node1));

    /* Test peek_head and peek_tail on 2 elements */
    TEST_ASSERT_EQUAL_PTR(&node2, linked_list_peek_head(&list));
    TEST_ASSERT_EQUAL_PTR(&node3, linked_list_peek_tail(&list));
    TEST_ASSERT_FALSE(linked_list_is_empty(&list));

    /* Pop tail (node3) */
    LinkedListNode* popped = linked_list_pop_tail(&list);
    TEST_ASSERT_EQUAL_PTR(&node3, popped);
    TEST_ASSERT_EQUAL_UINT(1U, list.count);

    /* Pop head remaining (node2) */
    popped = linked_list_pop_head(&list);
    TEST_ASSERT_EQUAL_PTR(&node2, popped);
    TEST_ASSERT_EQUAL_UINT(0U, list.count);
    TEST_ASSERT_TRUE(linked_list_is_empty(&list));

    /* Pop empty returns NULL */
    TEST_ASSERT_NULL(linked_list_pop_head(&list));
    TEST_ASSERT_NULL(linked_list_pop_tail(&list));
    TEST_ASSERT_NULL(linked_list_peek_head(&list));
    TEST_ASSERT_NULL(linked_list_peek_tail(&list));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_linked_list_init_success);
    RUN_TEST(test_linked_list_init_null);
    RUN_TEST(test_linked_list_node_init_and_state);
    RUN_TEST(test_linked_list_insert_head);
    RUN_TEST(test_linked_list_insert_tail);
    RUN_TEST(test_linked_list_insert_before_and_after);
    RUN_TEST(test_linked_list_remove);
    RUN_TEST(test_linked_list_pop_head_and_tail);
    RUN_TEST(test_linked_list_peek_and_traversal);
    RUN_TEST(test_linked_list_clear);
    RUN_TEST(test_linked_list_container_of);
    RUN_TEST(test_linked_list_direct_operations);

    return UNITY_END();
}
