/*
 * numbl_allocator — wrapper around mimalloc.
 *
 * mimalloc handles memory management internally via emscripten's sbrk,
 * which translates to wasm memory.grow(). With SharedArrayBuffer-backed
 * imported memory, existing views survive growth.
 */

#include "numbl_allocator.h"
#include <mimalloc.h>
#include <string.h>

static int s_initialized = 0;
static size_t s_alloc_count = 0;
static size_t s_free_count = 0;
static size_t s_used_bytes = 0;

#define MIN_ALIGN 8

NUMBL_EXPORT int numbl_alloc_init(size_t initial_bytes) {
  (void)initial_bytes;
  if (s_initialized) return -1;
  s_initialized = 1;
  s_alloc_count = 0;
  s_free_count = 0;
  s_used_bytes = 0;
  return 0;
}

NUMBL_EXPORT size_t numbl_alloc(size_t bytes) {
  return numbl_alloc_aligned(bytes, MIN_ALIGN);
}

NUMBL_EXPORT size_t numbl_alloc_aligned(size_t bytes, size_t align) {
  if (!s_initialized || bytes == 0) return 0;
  if (align < MIN_ALIGN) align = MIN_ALIGN;

  void *ptr = mi_malloc_aligned(bytes, align);
  if (!ptr) return 0;

  s_alloc_count++;
  s_used_bytes += mi_usable_size(ptr);
  return (size_t)ptr;
}

NUMBL_EXPORT void numbl_free(size_t offset) {
  if (!s_initialized || offset == 0) return;

  void *ptr = (void *)offset;
  s_used_bytes -= mi_usable_size(ptr);
  s_free_count++;
  mi_free(ptr);
}

/* No-op: mimalloc handles growth internally via sbrk/memory.grow. */
NUMBL_EXPORT int numbl_alloc_add_pool(size_t pool_start, size_t pool_bytes) {
  (void)pool_start;
  (void)pool_bytes;
  return 0;
}

NUMBL_EXPORT void numbl_alloc_stats(numbl_alloc_stats_t *out) {
  if (!out) return;
  out->total_bytes = 0; /* mimalloc manages this internally */
  out->used_bytes = s_used_bytes;
  out->free_bytes = 0;
  out->alloc_count = s_alloc_count;
  out->free_count = s_free_count;
}

NUMBL_EXPORT void numbl_alloc_destroy(void) {
  if (!s_initialized) return;
  mi_collect(1);
  s_initialized = 0;
  s_alloc_count = 0;
  s_free_count = 0;
  s_used_bytes = 0;
}

NUMBL_EXPORT size_t numbl_alloc_pool_base(void) {
  return 0; /* not applicable with mimalloc */
}
