cmake_minimum_required(VERSION 3.15)

include(FetchContent)
include(extern.cmake)

Extern_MakeAvailable (fmt "https://github.com/fmtlib/fmt.git" 7.1.3)
Extern_MakeAvailable (lyra "https://github.com/bfgroup/Lyra.git" 1.5.1)
Extern_MakeAvailable (nlohmann_json "https://github.com/nlohmann/json.git" v3.9.1 BUILD_TESTING_VARIABLE JSON_BuildTests)
Extern_MakeAvailable (re2 "https://github.com/google/re2.git" 2020-11-01 BUILD_TESTING_VARIABLE RE2_BUILD_TESTING SOURCE_DIR_VARIABLE RE2_SOURCE_DIR)

target_compile_options (re2 PRIVATE $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:CLANG>>:-Wno-error=unused-parameter -Wno-error=missing-field-initializers>)
target_include_directories (re2 INTERFACE $<BUILD_INTERFACE:${RE2_SOURCE_DIR}>)

if (BUILD_TESTING)
  Extern_MakeAvailable (catch2 "https://github.com/catchorg/Catch2.git" v2.13.4)
  Extern_MakeAvailable (benchmark "https://github.com/google/benchmark.git" v1.5.2 BUILD_TESTING_VARIABLE BENCHMARK_ENABLE_TESTING)
endif()

find_package (Threads REQUIRED)
