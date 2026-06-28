#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

static int g_passed = 0;
static int g_failed = 0;

#define TEST(name, cond)                 \
    do                                   \
    {                                    \
        if (cond)                        \
        {                                \
            printf("[PASS] %s\n", name); \
            ++g_passed;                  \
        }                                \
        else                             \
        {                                \
            printf("[FAIL] %s\n", name); \
            ++g_failed;                  \
        }                                \
    } while (0)

/* ── ForEach helpers ─────────────────────────────────────────────── */

static int collect_action(void *_element, void *_context)
{
    int **arr = (int **)_context;
    **arr = *(int *)_element;
    (*arr)++;
    return 1;
}

static int stop_at_3_action(void *_element, void *_context)
{
    (void)_context;
    int val = *(int *)_element;
    return val != 3;
}

static void destroy_item(void *_element)
{
    free(_element);
}

/* ── Tests ───────────────────────────────────────────────────────── */

static void test_create(void)
{
    Queue *q = QueueCreate(0);
    TEST("create size 0 returns NULL", q == NULL);

    q = QueueCreate(5);
    TEST("create size 5 succeeds", q != NULL);
    TEST("new queue is empty", QueueIsEmpty(q));
    QueueDestroy(&q, NULL);
    TEST("destroy nulls pointer", q == NULL);
}

static void test_insert(void)
{
    int a = 1, b = 2;

    TEST("insert null queue", QueueInsert(NULL, &a) == QUEUE_UNINITIALIZED_ERROR);

    Queue *q = QueueCreate(2);
    TEST("insert null item", QueueInsert(q, NULL) == ITEM_IS_NULL);
    TEST("insert first item ok", QueueInsert(q, &a) == QUEUE_SUCCESS);
    TEST("not empty after insert", !QueueIsEmpty(q));
    TEST("insert second item ok", QueueInsert(q, &b) == QUEUE_SUCCESS);
    TEST("insert overflow", QueueInsert(q, &a) == QUEUE_OVERFLOW_ERROR);
    QueueDestroy(&q, NULL);
}

static void test_remove(void)
{
    int a = 10, b = 20;
    void *out;

    TEST("remove null queue", QueueRemove(NULL, &out) == QUEUE_UNINITIALIZED_ERROR);

    Queue *q = QueueCreate(3);
    TEST("remove from empty", QueueRemove(q, &out) == QUEUE_DATA_NOT_FOUND_ERROR);
    QueueInsert(q, &a);
    TEST("remove null out ptr", QueueRemove(q, NULL) == QUEUE_DATA_UNINITIALIZED_ERROR);
    QueueRemove(q, &out); /* drain the item we just inserted */

    QueueInsert(q, &a);
    QueueInsert(q, &b);

    TEST("remove first ok", QueueRemove(q, &out) == QUEUE_SUCCESS);
    TEST("remove preserves FIFO order (got a=10)", *(int *)out == 10);
    TEST("remove second ok", QueueRemove(q, &out) == QUEUE_SUCCESS);
    TEST("remove preserves FIFO order (got b=20)", *(int *)out == 20);
    TEST("empty after removing all", QueueIsEmpty(q));

    QueueDestroy(&q, NULL);
}

static void test_circular_wraparound(void)
{
    int vals[6] = {1, 2, 3, 4, 5, 6};
    void *out;

    Queue *q = QueueCreate(3);
    QueueInsert(q, &vals[0]);
    QueueInsert(q, &vals[1]);
    QueueInsert(q, &vals[2]);

    QueueRemove(q, &out); /* remove 1, head advances */
    QueueRemove(q, &out); /* remove 2 */

    /* tail has wrapped; inserting should reuse slots 0 and 1 */
    TEST("insert after wraparound ok", QueueInsert(q, &vals[3]) == QUEUE_SUCCESS);
    TEST("insert after wraparound ok 2", QueueInsert(q, &vals[4]) == QUEUE_SUCCESS);

    QueueRemove(q, &out);
    TEST("wraparound fifo order: 3", *(int *)out == 3);
    QueueRemove(q, &out);
    TEST("wraparound fifo order: 4", *(int *)out == 4);
    QueueRemove(q, &out);
    TEST("wraparound fifo order: 5", *(int *)out == 5);

    QueueDestroy(&q, NULL);
}

static void test_foreach(void)
{
    int vals[4] = {1, 2, 3, 4};
    int collected[4];
    int *ptr = collected;
    size_t count;

    TEST("foreach null queue returns 0", QueueForEach(NULL, collect_action, &ptr) == 0);

    Queue *q = QueueCreate(4);
    for (int i = 0; i < 4; i++)
        QueueInsert(q, &vals[i]);

    TEST("foreach null action returns 0", QueueForEach(q, NULL, NULL) == 0);

    count = QueueForEach(q, collect_action, &ptr);
    TEST("foreach visits all items", count == 4);
    TEST("foreach order correct [0]", collected[0] == 1);
    TEST("foreach order correct [3]", collected[3] == 4);

    /* early stop: stop_at_3_action returns 0 when element == 3 */
    count = QueueForEach(q, stop_at_3_action, NULL);
    TEST("foreach early stop count", count == 2); /* ran for 1, 2 then stopped at 3 */

    QueueDestroy(&q, NULL);
}

static void test_destroy_with_func(void)
{
    Queue *q = QueueCreate(3);
    for (int i = 0; i < 3; i++)
    {
        int *p = malloc(sizeof(int));
        *p = i;
        QueueInsert(q, p);
    }
    /* should call free on each item without crashing */
    QueueDestroy(&q, destroy_item);
    TEST("destroy with func nulls pointer", q == NULL);

    /* destroy on already-NULL pointer should not crash */
    QueueDestroy(&q, NULL);
    TEST("destroy on NULL is safe", q == NULL);
}

/* ── Main ────────────────────────────────────────────────────────── */

int main(void)
{
    test_create();
    test_insert();
    test_remove();
    test_circular_wraparound();
    test_foreach();
    test_destroy_with_func();

    printf("\n%d passed, %d failed\n", g_passed, g_failed);
    return g_failed ? 1 : 0;
}
