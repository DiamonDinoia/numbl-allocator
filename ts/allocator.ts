/**
 * Browser-side wasm allocator backed by mimalloc.
 *
 * Loads the emscripten-generated module (.mjs + .wasm) and exposes
 * a minimal allocator interface. Memory growth is handled by emscripten
 * internally — after growth, the buffer reference changes, so callers
 * must use allocator.buffer (not a cached reference) for new views.
 */

const ALIGNMENT = 16; // wasm SIMD v128

export interface WasmAllocator {
  /** Current memory buffer. Re-read after each alloc (may change on growth). */
  readonly buffer: ArrayBuffer;
  /** Allocate `bytes` with the given alignment. Returns byte offset (0 = failure). */
  alloc(bytes: number, align?: number): number;
  /** Free a previously allocated byte offset. */
  free(ptr: number): void;
}

/**
 * Load the allocator module. Pass the URL prefix where both
 * numbl_allocator.mjs and numbl_allocator.wasm are served.
 *
 * In a Vite app, files in public/ are served at the root,
 * so urlPrefix would be "/wasm-kernels/".
 */
export async function createAllocator(
  moduleUrl: string,
): Promise<WasmAllocator | null> {
  try {
    const { default: createModule } = await import(/* @vite-ignore */ moduleUrl);
    const mod = await createModule();

    if ((mod._numbl_alloc_init(0) as number) !== 0) return null;

    const wasmMemory: WebAssembly.Memory = mod.wasmMemory;

    return {
      get buffer() {
        return wasmMemory.buffer;
      },
      alloc(bytes: number, align: number = ALIGNMENT): number {
        return mod._numbl_alloc_aligned(bytes, align) as number;
      },
      free(ptr: number): void {
        if (ptr !== 0) mod._numbl_free(ptr);
      },
    };
  } catch {
    return null;
  }
}
