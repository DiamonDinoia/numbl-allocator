cmake_minimum_required(VERSION 3.20)
project(numbl-allocator C)

include(FetchContent)
FetchContent_Declare(mimalloc
  GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
  GIT_TAG v2.2.2
)
set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(mimalloc)

if(EMSCRIPTEN)
  add_executable(numbl_allocator src/numbl_allocator.c)
else()
  add_library(numbl_allocator src/numbl_allocator.c)
endif()
target_link_libraries(numbl_allocator PRIVATE mimalloc-static)
target_include_directories(numbl_allocator PUBLIC src/)


if(EMSCRIPTEN)
  set(WASM_EXPORTED_FUNCTIONS
    "_numbl_alloc_init"
    "_numbl_alloc"
    "_numbl_alloc_aligned"
    "_numbl_free"
    "_numbl_alloc_add_pool"
    "_numbl_alloc_stats"
    "_numbl_alloc_destroy"
    "_numbl_alloc_pool_base"
    "_malloc"
    "_free"
  )
  list(JOIN WASM_EXPORTED_FUNCTIONS "," WASM_EXPORTS_STR)

  target_compile_options(numbl_allocator PRIVATE -O3)
  target_link_options(numbl_allocator PRIVATE
    "SHELL:-s MODULARIZE=1"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s MALLOC=emmalloc"
    "SHELL:-s FILESYSTEM=0"
    "SHELL:-s EXPORTED_FUNCTIONS=[${WASM_EXPORTS_STR}]"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=[wasmMemory]"
    "SHELL:-s ENVIRONMENT=web,worker"
    --no-entry
  )
  set_target_properties(numbl_allocator PROPERTIES
    SUFFIX ".js"
    PREFIX ""
  )
else()
  target_compile_options(numbl_allocator PRIVATE -O3 -Wall -Wextra)
endif()

# Tests (not built for WASM)
if(NOT EMSCRIPTEN)
  enable_testing()
  add_executable(test_allocator tests/test_allocator.c)
  target_link_libraries(test_allocator PRIVATE numbl_allocator)
  add_test(NAME test_allocator COMMAND test_allocator)
endif()
