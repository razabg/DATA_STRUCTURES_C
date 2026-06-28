#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vector.h"

/* ── Simple test framework ────────────────────────────────────────── */

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(desc, expr)                                      \
    do                                                         \
    {                                                          \
        if (expr)                                              \
        {                                                      \
            printf("  PASS  %s\n", desc);                      \
            g_passed++;                                        \
        }                                                      \
        else                                                   \
        {                                                      \
            printf("  FAIL  %s  (line %d)\n", desc, __LINE__); \
            g_failed++;                                        \
        }                                                      \
    } while (0)

#define SECTION(name) printf("\n[%s]\n", name)

/* ── Helper: elementDestroy that frees heap-allocated ints ────────── */

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

/* ── ForEach helpers ──────────────────────────────────────────────── */

static int sum_action(void *elem, size_t index, void *ctx)
{
    (void)index;
    *(int *)ctx += *(int *)elem;
    return 1; /* continue */
}

static int stop_at_3_action(void *elem, size_t index, void *ctx)
{
    (void)elem;
    (void)ctx;
    return (index < 3) ? 1 : 0; /* stop when index == 3 */
}

/* ── VectorCreate ─────────────────────────────────────────────────── */

static void test_create(void)
{
    SECTION("VectorCreate");

    /* both zero → NULL */
    Vector *v = VectorCreate(0, 0);
    CHECK("both zero returns NULL", v == NULL);

    /* normal creation */
    v = VectorCreate(10, 5);
    CHECK("normal create non-NULL", v != NULL);
    CHECK("initial size is 0", VectorSize(v) == 0);
    CHECK("initial capacity is 10", VectorCapacity(v) == 10);
    VectorDestroy(&v, NULL);

    /* zero initialCapacity, nonzero blockSize: growable empty vector.
       malloc(0) behaviour is implementation-defined; some allocators
       return a valid pointer, others NULL.  Either way the function
       must not crash. */
    v = VectorCreate(0, 4);
    /* just check we didn't crash; NULL is acceptable here */
    CHECK("zero cap + nonzero block does not crash", 1);
    if (v)
        VectorDestroy(&v, NULL);

    /* nonzero capacity, zero blockSize → header says "fixed size" */
    v = VectorCreate(5, 0);
    CHECK("fixed-size (blockSize=0) create non-NULL", v != NULL);
    CHECK("fixed-size initial capacity is 5", VectorCapacity(v) == 5);
    VectorDestroy(&v, NULL);
}

/* ── VectorDestroy ────────────────────────────────────────────────── */

static void test_destroy(void)
{
    SECTION("VectorDestroy");

    /* NULL double-pointer: must not crash */
    VectorDestroy(NULL, NULL);
    CHECK("destroy(NULL, NULL) does not crash", 1);

    /* *ptr == NULL: must not crash */
    Vector *v = NULL;
    VectorDestroy(&v, NULL);
    CHECK("destroy(&NULL, NULL) does not crash", 1);

    /* pointer is nulled after destroy */
    v = VectorCreate(4, 2);
    VectorDestroy(&v, NULL);
    CHECK("pointer is NULL after destroy", v == NULL);

    /* elementDestroy is called for every element */
    v = VectorCreate(4, 2);
    int *a = make_int(1), *b = make_int(2), *c = make_int(3);
    VectorAppend(v, a);
    VectorAppend(v, b);
    VectorAppend(v, c);
    g_destroy_count = 0;
    VectorDestroy(&v, count_destroy);
    CHECK("destroy calls elementDestroy for each item", g_destroy_count == 3);
    CHECK("pointer is NULL after destroy with callback", v == NULL);

    /* destroy with NULL callback does not call any destructor (no crash) */
    v = VectorCreate(4, 2);
    VectorAppend(v, make_int(42)); /* intentional leak — no destructor */
    VectorDestroy(&v, NULL);
    CHECK("destroy with NULL callback does not crash", v == NULL);
}

/* ── VectorAppend ─────────────────────────────────────────────────── */

static void test_append(void)
{
    SECTION("VectorAppend");

    /* NULL vector */
    CHECK("append to NULL → UNINITIALIZED",
          VectorAppend(NULL, (void *)1) == VECTOR_UNITIALIZED_ERROR);

    /* NULL item */
    Vector *v = VectorCreate(4, 2);
    CHECK("append NULL item → ITEM_UNINITIALIZED",
          VectorAppend(v, NULL) == ITEM_UNITIALIZED_ERROR);

    /* normal appends within initial capacity */
    int vals[4] = {10, 20, 30, 40};
    CHECK("append[0] success", VectorAppend(v, &vals[0]) == VECTOR_SUCCESS);
    CHECK("append[1] success", VectorAppend(v, &vals[1]) == VECTOR_SUCCESS);
    CHECK("append[2] success", VectorAppend(v, &vals[2]) == VECTOR_SUCCESS);
    CHECK("append[3] success", VectorAppend(v, &vals[3]) == VECTOR_SUCCESS);
    CHECK("size is 4 after 4 appends", VectorSize(v) == 4);

    /* append beyond initial capacity triggers grow */
    int extra = 99;
    VectorResult r = VectorAppend(v, &extra);
    CHECK("append beyond capacity succeeds (grow)", r == VECTOR_SUCCESS);
    CHECK("size is 5 after grow-append", VectorSize(v) == 5);
    CHECK("capacity grew by blockSize", VectorCapacity(v) == 6); /* 4 + blockSize(2) */

    VectorDestroy(&v, NULL);

    /* append to a vector that was created with blockSize=0 (fixed size).
       Per the header this should fail — expose the bug if present. */
    v = VectorCreate(2, 0);
    VectorAppend(v, &vals[0]);
    VectorAppend(v, &vals[1]);
    /* third append to a "fixed" vector: VECTOR_ALLOCATION_ERROR expected.
       NOTE: current implementation silently changes blockSize to 1, so
       it will grow instead.  This CHECK documents the specified behaviour. */
    r = VectorAppend(v, &vals[2]);
    CHECK("BUG#3: fixed-size append beyond capacity → ALLOCATION_ERROR",
          r == VECTOR_ALLOCATION_ERROR);
    VectorDestroy(&v, NULL);
}

/* ── VectorRemove ─────────────────────────────────────────────────── */

static void test_remove(void)
{
    SECTION("VectorRemove");

    void *out = NULL;

    /* NULL vector */
    CHECK("remove from NULL → UNINITIALIZED",
          VectorRemove(NULL, &out) == VECTOR_UNITIALIZED_ERROR);

    /* empty vector */
    Vector *v = VectorCreate(4, 2);
    CHECK("remove from empty → VECTOR_IS_EMPTY",
          VectorRemove(v, &out) == VECTOR_IS_EMPTY);

    /* NULL pValue */
    int x = 7;
    VectorAppend(v, &x);
    CHECK("remove with NULL pValue → VALUE_IS_NULL",
          VectorRemove(v, NULL) == VALUE_IS_NULL);

    /* normal remove: last element comes back */
    int a = 1, b = 2, c = 3;
    VectorAppend(v, &a);
    VectorAppend(v, &b);
    VectorAppend(v, &c);
    /* v now has: x(7), a(1), b(2), c(3) — 4 elements */
    VectorResult r = VectorRemove(v, &out);
    CHECK("remove success", r == VECTOR_SUCCESS);
    CHECK("removed value is last item", *(int *)out == 3);
    CHECK("size decremented after remove", VectorSize(v) == 3);

    /* remove until empty, then one more → VECTOR_IS_EMPTY */
    VectorRemove(v, &out);
    VectorRemove(v, &out);
    VectorRemove(v, &out);
    CHECK("remove last item leaves size 0", VectorSize(v) == 0);
    CHECK("remove from now-empty → VECTOR_IS_EMPTY",
          VectorRemove(v, &out) == VECTOR_IS_EMPTY);

    VectorDestroy(&v, NULL);

    /* shrink: fill past initial capacity then drain, check capacity shrinks
       but never below originalSize */
    v = VectorCreate(4, 2);
    int items[10];
    for (int i = 0; i < 10; i++)
    {
        items[i] = i;
        VectorAppend(v, &items[i]);
    }
    size_t cap_full = VectorCapacity(v);
    /* drain 8 items — capacity should shrink */
    for (int i = 0; i < 8; i++)
        VectorRemove(v, &out);
    size_t cap_drained = VectorCapacity(v);
    CHECK("capacity shrinks after mass remove", cap_drained < cap_full);
    CHECK("capacity never below originalSize(4)", cap_drained >= 4);

    VectorDestroy(&v, NULL);
}

/* ── VectorGet ────────────────────────────────────────────────────── */

static void test_get(void)
{
    SECTION("VectorGet");

    void *out = NULL;

    /* NULL vector */
    CHECK("get from NULL → UNINITIALIZED",
          VectorGet(NULL, 0, &out) == VECTOR_UNITIALIZED_ERROR);

    Vector *v = VectorCreate(4, 2);
    int a = 10, b = 20, c = 30;
    VectorAppend(v, &a);
    VectorAppend(v, &b);
    VectorAppend(v, &c);

    /* NULL pValue */
    CHECK("get with NULL pValue → VALUE_IS_NULL",
          VectorGet(v, 0, NULL) == VALUE_IS_NULL);

    /* out-of-bounds: index == nItems */
    CHECK("get at nItems → OUT_OF_BOUNDS",
          VectorGet(v, 3, &out) == VECTOR_INDEX_OUT_OF_BOUNDS_ERROR);

    /* out-of-bounds: large index */
    CHECK("get at SIZE_MAX → OUT_OF_BOUNDS",
          VectorGet(v, (size_t)-1, &out) == VECTOR_INDEX_OUT_OF_BOUNDS_ERROR);

    /* valid gets */
    VectorGet(v, 0, &out);
    CHECK("get index 0 → first item", *(int *)out == 10);
    VectorGet(v, 2, &out);
    CHECK("get last index (2) → last item", *(int *)out == 30);

    VectorDestroy(&v, NULL);
}

/* ── VectorSet ────────────────────────────────────────────────────── */

static void test_set(void)
{
    SECTION("VectorSet");

    /* NULL vector */
    int x = 99;
    CHECK("set on NULL → UNINITIALIZED",
          VectorSet(NULL, 0, &x) == VECTOR_UNITIALIZED_ERROR);

    Vector *v = VectorCreate(4, 2);
    int a = 1, b = 2, c = 3;
    VectorAppend(v, &a);
    VectorAppend(v, &b);
    VectorAppend(v, &c);

    /* out-of-bounds */
    CHECK("set at nItems → OUT_OF_BOUNDS",
          VectorSet(v, 3, &x) == VECTOR_INDEX_OUT_OF_BOUNDS_ERROR);
    CHECK("set at large index → OUT_OF_BOUNDS",
          VectorSet(v, (size_t)-1, &x) == VECTOR_INDEX_OUT_OF_BOUNDS_ERROR);

    /* normal set */
    VectorResult r = VectorSet(v, 1, &x);
    CHECK("set success", r == VECTOR_SUCCESS);

    void *out = NULL;
    VectorGet(v, 1, &out);
    CHECK("get after set returns new value", *(int *)out == 99);

    /* first and last index */
    int first = 111, last = 222;
    VectorSet(v, 0, &first);
    VectorSet(v, 2, &last);
    VectorGet(v, 0, &out);
    CHECK("set/get index 0", *(int *)out == 111);
    VectorGet(v, 2, &out);
    CHECK("set/get last index", *(int *)out == 222);

    VectorDestroy(&v, NULL);
}

/* ── VectorSize / VectorCapacity ──────────────────────────────────── */

static void test_size_capacity(void)
{
    SECTION("VectorSize / VectorCapacity");

    CHECK("size of NULL → UNINITIALIZED (1)", VectorSize(NULL) == VECTOR_UNITIALIZED_ERROR);
    CHECK("capacity of NULL → UNINITIALIZED (1)", VectorCapacity(NULL) == VECTOR_UNITIALIZED_ERROR);

    Vector *v = VectorCreate(8, 4);
    CHECK("size of new vector is 0", VectorSize(v) == 0);
    CHECK("capacity of new vector is 8", VectorCapacity(v) == 8);

    int items[12];
    for (int i = 0; i < 8; i++)
    {
        items[i] = i;
        VectorAppend(v, &items[i]);
    }
    CHECK("size after 8 appends is 8", VectorSize(v) == 8);
    CHECK("capacity still 8 (not grown)", VectorCapacity(v) == 8);

    /* one more triggers grow */
    items[8] = 8;
    VectorAppend(v, &items[8]);
    CHECK("size after 9th append is 9", VectorSize(v) == 9);
    CHECK("capacity after grow is 12", VectorCapacity(v) == 12); /* 8 + blockSize(4) */

    void *out;
    VectorRemove(v, &out);
    CHECK("size after remove is 8", VectorSize(v) == 8);

    VectorDestroy(&v, NULL);
}

/* ── VectorForEach ────────────────────────────────────────────────── */

static void test_foreach(void)
{
    SECTION("VectorForEach");

    /* NULL vector */
    CHECK("forEach NULL vector → 0",
          VectorForEach(NULL, sum_action, NULL) == 0);

    /* NULL action */
    Vector *v = VectorCreate(4, 2);
    CHECK("forEach NULL action → 0",
          VectorForEach(v, NULL, NULL) == 0);

    /* empty vector */
    CHECK("forEach empty vector → 0",
          VectorForEach(v, sum_action, NULL) == 0);

    /* full iteration */
    int items[5] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
        VectorAppend(v, &items[i]);

    int sum = 0;
    size_t count = VectorForEach(v, sum_action, &sum);
    CHECK("forEach visits all elements", count == 5);
    CHECK("forEach sum is correct", sum == 15);

    /* early stop: stop_at_3_action returns 0 when index == 3 */
    count = VectorForEach(v, stop_at_3_action, NULL);
    CHECK("forEach early-stop returns index where stopped (3)", count == 3);

    VectorDestroy(&v, NULL);
}

/* ── Realloc bug: growing to wrong capacity (BUG #2) ─────────────── */

static void test_realloc_bug(void)
{
    SECTION("BUG#2: VectorAppend realloc target size");

    /* With initialCapacity=4 and blockSize=3, after filling to capacity
       and appending one more, the new capacity should be 4+3=7.
       The buggy code computes blockSize+blockSize = 6, not 7.  */
    Vector *v = VectorCreate(4, 3);
    int items[5] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++)
        VectorAppend(v, &items[i]);

    CHECK("BUG#2: capacity after grow should be initialCap+blockSize (7), not 2*blockSize (6)",
          VectorCapacity(v) == 7);

    VectorDestroy(&v, NULL);
}

/* ── main ─────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== Dynamic Vector Tests ===\n");

    test_create();
    test_destroy();
    test_append();
    test_remove();
    test_get();
    test_set();
    test_size_capacity();
    test_foreach();
    test_realloc_bug();

    printf("\n=== Results: %d passed, %d failed ===\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
