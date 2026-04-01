cmake_minimum_required(VERSION 3.20)
project(numbl-allocator C)

include(FetchContent)
FetchContent_Declare(tlsf
  GIT_REPOSITORY https://github.com/mattconte/tlsf.git
  GIT_TAG master
)
FetchContent_MakeAvailable(tlsf)

if(EMSCRIPTEN)
  # For WASM, build as an executable (produces standalone .wasm)
  add_executable(numbl_allocator src/numbl_allocator.c ${tlsf_SOURCE_DIR}/tlsf.c)
else()
  add_library(numbl_allocator src/numbl_allocator.c ${tlsf_SOURCE_DIR}/tlsf.c)
endif()
target_include_directories(numbl_allocator PUBLIC src/ PRIVATE ${tlsf_SOURCE_DIR})

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
  )
  list(JOIN WASM_EXPORTED_FUNCTIONS "," WASM_EXPORTS_STR)

  target_compile_options(numbl_allocator PRIVATE -O3)
  target_link_options(numbl_allocator PRIVATE
    "SHELL:-s STANDALONE_WASM"
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "SHELL:-s IMPORTED_MEMORY=1"
    "SHELL:-s FILESYSTEM=0"
    "SHELL:-s ERROR_ON_UNDEFINED_SYMBOLS=1"
    "SHELL:-s EXPORTED_FUNCTIONS=[${WASM_EXPORTS_STR}]"
    --no-entry
  )
  set_target_properties(numbl_allocator PROPERTIES SUFFIX ".wasm")
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
