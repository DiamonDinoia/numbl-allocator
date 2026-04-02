#include "numbl_allocator.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_init_destroy(void) {
  assert(numbl_alloc_init(1024 * 1024) == 0);
  numbl_alloc_destroy();
  printf("  PASS: init/destroy\n");
}

static void test_alloc_free(void) {
  assert(numbl_alloc_init(1024 * 1024) == 0);

  size_t p1 = numbl_alloc(64);
  assert(p1 != 0);
  assert((p1 & 7) == 0); /* 8-byte aligned */

  size_t p2 = numbl_alloc(128);
  assert(p2 != 0);
  assert(p2 != p1);

  numbl_free(p1);
  numbl_free(p2);

  numbl_alloc_destroy();
  printf("  PASS: alloc/free\n");
}

static void test_stats(void) {
  assert(numbl_alloc_init(1024 * 1024) == 0);

  numbl_alloc_stats_t stats;
  numbl_alloc_stats(&stats);
  assert(stats.alloc_count == 0);
  assert(stats.free_count == 0);

  size_t p1 = numbl_alloc(256);
  numbl_alloc_stats(&stats);
  assert(stats.alloc_count == 1);
  assert(stats.used_bytes >= 256);

  numbl_free(p1);
  numbl_alloc_stats(&stats);
  assert(stats.free_count == 1);

  numbl_alloc_destroy();
  printf("  PASS: stats\n");
}

static void test_alignment(void) {
  assert(numbl_alloc_init(1024 * 1024) == 0);

  /* Float64Array requires 8-byte alignment */
  for (int i = 0; i < 100; i++) {
    size_t p = numbl_alloc(i * 8 + 8);
    assert(p != 0);
    assert((p & 7) == 0);
  }

  numbl_alloc_destroy();
  printf("  PASS: alignment\n");
}

static void test_zero_alloc(void) {
  assert(numbl_alloc_init(1024 * 1024) == 0);
  assert(numbl_alloc(0) == 0); /* zero-size returns null */
  numbl_alloc_destroy();
  printf("  PASS: zero alloc\n");
}

static void test_free_null(void) {
  assert(numbl_alloc_init(1024 * 1024) == 0);
  numbl_free(0); /* should not crash */
  numbl_alloc_destroy();
  printf("  PASS: free null\n");
}

static void test_reuse(void) {
  assert(numbl_alloc_init(1024 * 1024) == 0);

  size_t p1 = numbl_alloc(512);
  numbl_free(p1);
  size_t p2 = numbl_alloc(512);
  assert(p2 != 0);

  numbl_free(p2);
  numbl_alloc_destroy();
  printf("  PASS: reuse\n");
}

int main(void) {
  printf("numbl_allocator tests:\n");
  test_init_destroy();
  test_alloc_free();
  test_stats();
  test_alignment();
  test_zero_alloc();
  test_free_null();
  test_reuse();
  printf("All tests passed.\n");
  return 0;
}
