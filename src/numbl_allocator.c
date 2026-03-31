/*
 * numbl_allocator — thin wrapper around TLSF.
 *
 * To switch to a different allocator backend:
 *   1. Change FetchContent_Declare in CMakeLists.txt
 *   2. Rewrite this file to call the new backend's API
 *   3. Keep the same numbl_allocator.h interface
 */

#include "numbl_allocator.h"
#include "tlsf.h"

#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
extern unsigned char __heap_base;
#else
#include <stdlib.h>
#endif

static tlsf_t s_tlsf = NULL;
static size_t s_alloc_count = 0;
static size_t s_free_count = 0;
static size_t s_used_bytes = 0;
static size_t s_total_bytes = 0;
static size_t s_pool_base_offset = 0;

/* Alignment for all allocations (Float64Array requires 8-byte alignment). */
#define MIN_ALIGN 8

NUMBL_EXPORT int numbl_alloc_init(size_t initial_bytes) {
  if (s_tlsf) return -1; /* already initialized */

#ifdef __EMSCRIPTEN__
  /* In WASM, the heap starts at __heap_base. Use everything from there
   * to the current memory size as the initial pool. */
  size_t heap_start = (size_t)&__heap_base;
  /* Align up to MIN_ALIGN */
  heap_start = (heap_start + MIN_ALIGN - 1) & ~(MIN_ALIGN - 1);

  size_t mem_size = __builtin_wasm_memory_size(0) * 65536;
  size_t available = mem_size - heap_start;

  if (initial_bytes > 0 && initial_bytes < available) {
    available = initial_bytes;
  }

  s_pool_base_offset = heap_start;
  s_tlsf = tlsf_create_with_pool((void *)heap_start, available);
  s_total_bytes = available;
#else
  size_t pool_size = initial_bytes > 0 ? initial_bytes : (64 * 1024 * 1024);
  void *pool = malloc(pool_size);
  if (!pool) return -1;

  s_pool_base_offset = (size_t)pool;
  s_tlsf = tlsf_create_with_pool(pool, pool_size);
  s_total_bytes = pool_size;
#endif

  if (!s_tlsf) return -1;
  s_alloc_count = 0;
  s_free_count = 0;
  s_used_bytes = 0;
  return 0;
}

NUMBL_EXPORT size_t numbl_alloc(size_t bytes) {
  return numbl_alloc_aligned(bytes, MIN_ALIGN);
}

NUMBL_EXPORT size_t numbl_alloc_aligned(size_t bytes, size_t align) {
  if (!s_tlsf || bytes == 0) return 0;
  if (align < MIN_ALIGN) align = MIN_ALIGN;

  void *ptr = tlsf_memalign(s_tlsf, align, bytes);
  if (!ptr) return 0;

  s_alloc_count++;
  s_used_bytes += tlsf_block_size(ptr);
  return (size_t)ptr;
}

NUMBL_EXPORT void numbl_free(size_t offset) {
  if (!s_tlsf || offset == 0) return;

  void *ptr = (void *)offset;
  s_used_bytes -= tlsf_block_size(ptr);
  s_free_count++;
  tlsf_free(s_tlsf, ptr);
}

NUMBL_EXPORT int numbl_alloc_add_pool(size_t pool_start, size_t pool_bytes) {
  if (!s_tlsf) return -1;
  pool_t pool = tlsf_add_pool(s_tlsf, (void *)pool_start, pool_bytes);
  if (!pool) return -1;
  s_total_bytes += pool_bytes;
  return 0;
}

NUMBL_EXPORT void numbl_alloc_stats(numbl_alloc_stats_t *out) {
  if (!out) return;
  out->total_bytes = s_total_bytes;
  out->used_bytes = s_used_bytes;
  out->free_bytes = s_total_bytes - s_used_bytes;
  out->alloc_count = s_alloc_count;
  out->free_count = s_free_count;
}

NUMBL_EXPORT void numbl_alloc_destroy(void) {
  if (!s_tlsf) return;
  tlsf_destroy(s_tlsf);
  s_tlsf = NULL;
  s_alloc_count = 0;
  s_free_count = 0;
  s_used_bytes = 0;
  s_total_bytes = 0;
}

NUMBL_EXPORT size_t numbl_alloc_pool_base(void) {
  return s_pool_base_offset;
}
