#ifndef NUMBL_ALLOCATOR_H
#define NUMBL_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __EMSCRIPTEN__
#define NUMBL_EXPORT __attribute__((used, visibility("default")))
#else
#define NUMBL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  size_t total_bytes;
  size_t used_bytes;
  size_t free_bytes;
  size_t alloc_count;
  size_t free_count;
} numbl_alloc_stats_t;

/* Initialize the allocator. For WASM, sets up a pool over the available heap.
 * initial_bytes: requested initial pool size (0 = use all available heap). */
NUMBL_EXPORT int numbl_alloc_init(size_t initial_bytes);

/* Allocate `bytes` from the pool. Returns byte offset from WASM memory base.
 * Returns 0 on failure (offset 0 is reserved as null sentinel).
 * Returned pointer is 8-byte aligned (suitable for Float64Array). */
NUMBL_EXPORT size_t numbl_alloc(size_t bytes);

/* Allocate `bytes` with explicit alignment. */
NUMBL_EXPORT size_t numbl_alloc_aligned(size_t bytes, size_t align);

/* Free a previously allocated block. */
NUMBL_EXPORT void numbl_free(size_t offset);

/* Register additional memory pages after WebAssembly.Memory.grow().
 * pool_start: byte offset where the new pool begins.
 * pool_bytes: size of the new pool in bytes. */
NUMBL_EXPORT int numbl_alloc_add_pool(size_t pool_start, size_t pool_bytes);

/* Query usage statistics. */
NUMBL_EXPORT void numbl_alloc_stats(numbl_alloc_stats_t *out);

/* Destroy the allocator and release all resources. */
NUMBL_EXPORT void numbl_alloc_destroy(void);

/* Returns the byte offset of the pool base (start of allocatable region).
 * Useful for computing WASM memory layout. */
NUMBL_EXPORT size_t numbl_alloc_pool_base(void);

#ifdef __cplusplus
}
#endif

#endif /* NUMBL_ALLOCATOR_H */
