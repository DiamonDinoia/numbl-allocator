/**
 * Browser-side wasm allocator backed by mimalloc.
 *
 * The emscripten output is plain JS (MODULARIZE=1, no EXPORT_ES6), so it
 * attaches a `Module` factory to `self` (or a provided global) rather than
 * using ES module syntax.  This lets us load it with:
 *
 *   - `importScripts(url)` inside a Web Worker (no COEP issues, synchronous)
 *   - `fetch` + `new Function(src)` on the main thread (CSP-permitting)
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

function isWorker(): boolean {
  return (
    typeof WorkerGlobalScope !== "undefined" &&
    self instanceof WorkerGlobalScope
  );
}

async function loadViaImportScripts(jsUrl: string, wasmUrl: string): Promise<Record<string, unknown>> {
  // importScripts is synchronous and available in all Worker types.
  // The plain-JS emscripten glue sets `self.Module` as the factory.
  (self as unknown as Record<string, unknown>).Module = undefined;
  importScripts(jsUrl);
  const factory = (self as unknown as Record<string, unknown>).Module as
    | ((opts: Record<string, unknown>) => Promise<Record<string, unknown>>)
    | undefined;
  if (typeof factory !== "function") throw new Error("Module factory not found after importScripts");
  const wasmResp = await fetch(wasmUrl);
  if (!wasmResp.ok) throw new Error(`wasm fetch failed: ${wasmResp.status}`);
  const wasmBinary = await wasmResp.arrayBuffer();
  return factory({ wasmBinary });
}

async function loadViaEval(jsUrl: string, wasmUrl: string): Promise<Record<string, unknown>> {
  const [jsResp, wasmResp] = await Promise.all([fetch(jsUrl), fetch(wasmUrl)]);
  if (!jsResp.ok || !wasmResp.ok) throw new Error("fetch failed");
  const [src, wasmBinary] = await Promise.all([jsResp.text(), wasmResp.arrayBuffer()]);

  // The emscripten plain-JS output is `var Module = (() => { return factory; })();`
  // Wrap in a function and return Module to capture the factory.
  // eslint-disable-next-line no-new-func
  const factory = new Function(src + "\nreturn Module;")() as (
    opts: Record<string, unknown>
  ) => Promise<Record<string, unknown>>;
  if (typeof factory !== "function") throw new Error("Module factory not found");
  return factory({ wasmBinary });
}

/**
 * Load the allocator module.
 *
 * @param jsUrl   URL to `numbl_allocator.js`
 * @param wasmUrl URL to `numbl_allocator.wasm`
 */
export async function createAllocator(
  jsUrl: string,
  wasmUrl: string,
): Promise<WasmAllocator | null> {
  try {
    const mod = isWorker()
      ? await loadViaImportScripts(jsUrl, wasmUrl)
      : await loadViaEval(jsUrl, wasmUrl);

    if ((mod._numbl_alloc_init as (n: number) => number)(0) !== 0) return null;

    const wasmMemory = mod.wasmMemory as WebAssembly.Memory;

    return {
      get buffer() {
        return wasmMemory.buffer;
      },
      alloc(bytes: number, align: number = ALIGNMENT): number {
        return (mod._numbl_alloc_aligned as (b: number, a: number) => number)(bytes, align);
      },
      free(ptr: number): void {
        if (ptr !== 0) (mod._numbl_free as (p: number) => void)(ptr);
      },
    };
  } catch {
    return null;
  }
}
