#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

/* ── Simple test framework ────────────────────────────────────────── */

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(desc, expr)                                               \
    do {                                                                \
        if (expr) {                                                     \
            printf("  PASS  %s\n", desc);                              \
            g_passed++;                                                 \
        } else {                                                        \
            printf("  FAIL  %s  (line %d)\n", desc, __LINE__);        \
            g_failed++;                                                 \
        }                                                               \
    } while (0)

#define SECTION(name) printf("\n[%s]\n", name)

/* ── Helpers ──────────────────────────────────────────────────────── */

static int g_destroy_count = 0;

static void count_destroy(void *item)
{
    g_destroy_count++;
    free(item);
}

static int *make_int(int val)
{
    int *p = malloc(sizeof(int));
    *p = val;
    return p;
}

static int collect_action(void *elem, size_t index, void *ctx)
{
    (void)index;
    int **arr = (int **)ctx;
    arr[index] = (int *)elem;
    return 1;
}

static int stop_after_2(void *elem, size_t index, void *ctx)
{
    (void)elem;
    (void)ctx;
    return index < 2 ? 1 : 0;
}

/* ── StackCreate ──────────────────────────────────────────────────── */

static void test_create(void)
{
    SECTION("StackCreate");

    CHECK("both zero returns NULL", StackCreate(0, 0) == NULL);

    Stack *s = StackCreate(8, 4);
    CHECK("normal create non-NULL",    s != NULL);
    CHECK("initial size is 0",         StackSize(s) == 0);
    CHECK("initial capacity is 8",     StackCapacity(s) == 8);
    CHECK("new stack is empty",        StackIsEmpty(s) == 1);
    StackDestroy(&s, NULL);

    s = StackCreate(4, 0);
    CHECK("fixed-size create non-NULL", s != NULL);
    StackDestroy(&s, NULL);
}

/* ── StackDestroy ─────────────────────────────────────────────────── */

static void test_destroy(void)
{
    SECTION("StackDestroy");

    StackDestroy(NULL, NULL);
    CHECK("destroy(NULL,NULL) does not crash", 1);

    Stack *s = NULL;
    StackDestroy(&s, NULL);
    CHECK("destroy(&NULL,NULL) does not crash", 1);

    s = StackCreate(4, 2);
    StackDestroy(&s, NULL);
    CHECK("pointer is NULL after destroy", s == NULL);

    s = StackCreate(4, 2);
    int *a = make_int(1), *b = make_int(2), *c = make_int(3);
    StackPush(s, a);
    StackPush(s, b);
    StackPush(s, c);
    g_destroy_count = 0;
    StackDestroy(&s, count_destroy);
    CHECK("elementDestroy called for each item", g_destroy_count == 3);
    CHECK("pointer is NULL after destroy with callback", s == NULL);

    /* destroy with NULL callback must not leak — just no crash */
    s = StackCreate(4, 2);
    StackPush(s, make_int(99));
    StackDestroy(&s, NULL);
    CHECK("destroy NULL callback does not crash", s == NULL);
}

/* ── StackPush ────────────────────────────────────────────────────── */

static void test_push(void)
{
    SECTION("StackPush");

    CHECK("push to NULL → UNINITIALIZED",
          StackPush(NULL, (void *)1) == STACK_UNITIALIZED_ERROR);

    Stack *s = StackCreate(4, 2);

    CHECK("push NULL item → ITEM_UNINITIALIZED",
          StackPush(s, NULL) == STACK_ITEM_UNITIALIZED_ERROR);

    int vals[4] = {10, 20, 30, 40};
    CHECK("push[0] success", StackPush(s, &vals[0]) == STACK_SUCCESS);
    CHECK("push[1] success", StackPush(s, &vals[1]) == STACK_SUCCESS);
    CHECK("push[2] success", StackPush(s, &vals[2]) == STACK_SUCCESS);
    CHECK("push[3] success", StackPush(s, &vals[3]) == STACK_SUCCESS);
    CHECK("size is 4 after 4 pushes", StackSize(s) == 4);

    /* push beyond capacity triggers grow */
    int extra = 99;
    CHECK("push beyond capacity succeeds", StackPush(s, &extra) == STACK_SUCCESS);
    CHECK("size is 5 after grow-push",     StackSize(s) == 5);

    StackDestroy(&s, NULL);
}

/* ── StackPop ─────────────────────────────────────────────────────── */

static void test_pop(void)
{
    SECTION("StackPop");

    void *out = NULL;

    CHECK("pop from NULL → UNINITIALIZED",
          StackPop(NULL, &out) == STACK_UNITIALIZED_ERROR);

    Stack *s = StackCreate(4, 2);

    CHECK("pop NULL pValue → VALUE_IS_NULL",
          StackPop(s, NULL) == STACK_VALUE_IS_NULL);

    CHECK("pop empty → STACK_IS_EMPTY",
          StackPop(s, &out) == STACK_IS_EMPTY);

    int a = 1, b = 2, c = 3;
    StackPush(s, &a);
    StackPush(s, &b);
    StackPush(s, &c);

    /* LIFO order: last pushed comes out first */
    StackPop(s, &out);
    CHECK("pop returns top (last pushed)", *(int *)out == 3);
    CHECK("size decremented after pop",    StackSize(s) == 2);

    StackPop(s, &out);
    CHECK("second pop returns next item",  *(int *)out == 2);

    StackPop(s, &out);
    CHECK("third pop returns first item",  *(int *)out == 1);
    CHECK("size is 0 after all pops",      StackSize(s) == 0);

    CHECK("pop now-empty → STACK_IS_EMPTY",
          StackPop(s, &out) == STACK_IS_EMPTY);

    StackDestroy(&s, NULL);
}

/* ── StackTop ─────────────────────────────────────────────────────── */

static void test_top(void)
{
    SECTION("StackTop");

    void *out = NULL;

    CHECK("top on NULL → UNINITIALIZED",
          StackTop(NULL, &out) == STACK_UNITIALIZED_ERROR);

    Stack *s = StackCreate(4, 2);

    CHECK("top NULL pValue → VALUE_IS_NULL",
          StackTop(s, NULL) == STACK_VALUE_IS_NULL);

    CHECK("top empty → STACK_IS_EMPTY",
          StackTop(s, &out) == STACK_IS_EMPTY);

    int a = 1, b = 2, c = 3;
    StackPush(s, &a);
    StackPush(s, &b);
    StackPush(s, &c);

    StackTop(s, &out);
    CHECK("top returns last pushed item",    *(int *)out == 3);
    CHECK("size unchanged after top",        StackSize(s) == 3);

    /* top after pop reflects new top */
    StackPop(s, &out);
    StackTop(s, &out);
    CHECK("top after pop returns new top",   *(int *)out == 2);

    StackDestroy(&s, NULL);
}

/* ── StackSize ────────────────────────────────────────────────────── */

static void test_size(void)
{
    SECTION("StackSize");

    CHECK("size of NULL → UNINITIALIZED (1)", StackSize(NULL) == STACK_UNITIALIZED_ERROR);

    Stack *s = StackCreate(4, 2);
    CHECK("size of new stack is 0", StackSize(s) == 0);

    int vals[3] = {1, 2, 3};
    StackPush(s, &vals[0]);
    CHECK("size is 1 after 1 push", StackSize(s) == 1);
    StackPush(s, &vals[1]);
    CHECK("size is 2 after 2 pushes", StackSize(s) == 2);
    StackPush(s, &vals[2]);
    CHECK("BUG: size is 3 after 3 pushes (fails if wrapped through vectorToStackResult)",
          StackSize(s) == 3);

    void *out;
    StackPop(s, &out);
    CHECK("size is 2 after pop", StackSize(s) == 2);

    StackDestroy(&s, NULL);
}

/* ── StackCapacity ────────────────────────────────────────────────── */

static void test_capacity(void)
{
    SECTION("StackCapacity");

    CHECK("capacity of NULL returns 0", StackCapacity(NULL) == 0);

    Stack *s = StackCreate(6, 3);
    CHECK("initial capacity is 6", StackCapacity(s) == 6);

    int items[7];
    for (int i = 0; i < 7; i++) { items[i] = i; StackPush(s, &items[i]); }
    CHECK("capacity grew after exceeding initial", StackCapacity(s) == 9); /* 6+3 */

    StackDestroy(&s, NULL);
}

/* ── StackIsEmpty ─────────────────────────────────────────────────── */

static void test_is_empty(void)
{
    SECTION("StackIsEmpty");

    CHECK("NULL stack → empty (1)", StackIsEmpty(NULL) == 1);

    Stack *s = StackCreate(4, 2);
    CHECK("new stack is empty",     StackIsEmpty(s) == 1);

    int x = 5;
    StackPush(s, &x);
    CHECK("not empty after push",   StackIsEmpty(s) == 0);

    void *out;
    StackPop(s, &out);
    CHECK("empty again after pop",  StackIsEmpty(s) == 1);

    StackDestroy(&s, NULL);
}

/* ── StackPrint ───────────────────────────────────────────────────── */

static void test_print(void)
{
    SECTION("StackPrint");

    CHECK("print NULL stack → 0",   StackPrint(NULL, collect_action, NULL) == 0);
    Stack *s = StackCreate(4, 2);
    CHECK("print NULL action → 0",  StackPrint(s, NULL, NULL) == 0);
    CHECK("print empty stack → 0",  StackPrint(s, collect_action, NULL) == 0);

    int a = 1, b = 2, c = 3;
    StackPush(s, &a);
    StackPush(s, &b);
    StackPush(s, &c);

    size_t count = StackPrint(s, stop_after_2, NULL);
    CHECK("early-stop returns 2",   count == 2);

    CHECK("size unchanged after print", StackSize(s) == 3);

    StackDestroy(&s, NULL);
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== Stack Tests ===\n");

    test_create();
    test_destroy();
    test_push();
    test_pop();
    test_top();
    test_size();
    test_capacity();
    test_is_empty();
    test_print();

    printf("\n=== Results: %d passed, %d failed ===\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
