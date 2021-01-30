cmake_minimum_required(VERSION 3.15)

include(GeneratorPrivate)

_Generator_MakeAvailable (fmt GIT_REPOSITORY "https://github.com/fmtlib/fmt.git" GIT_TAG 7.1.3 GIT_SHALLOW ON)
_Generator_MakeAvailable (lyra GIT_REPOSITORY "https://github.com/bfgroup/Lyra.git" GIT_TAG 1.5.1 GIT_SHALLOW ON)

set (JSON_BuildTests OFF CACHE BOOL "")
_Generator_MakeAvailable (nlohmann_json GIT_REPOSITORY "https://github.com/nlohmann/json.git" GIT_TAG v3.9.1 GIT_SHALLOW ON)

set (RE2_BUILD_TESTING OFF CACHE BOOL "")
_Generator_MakeAvailable (re2 GIT_REPOSITORY "https://github.com/google/re2.git" GIT_TAG 2020-11-01 GIT_SHALLOW ON SOURCE_DIR_VARIABLE RE2_SOURCE_DIR)

target_compile_options (re2 PRIVATE $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:CLANG>>:-Wno-error=unused-parameter -Wno-error=missing-field-initializers>)
target_include_directories (re2 INTERFACE $<BUILD_INTERFACE:${RE2_SOURCE_DIR}>)

if (BUILD_TESTING AND GENERATOR_BUILD_TESTS)
  _Generator_MakeAvailable (catch2 GIT_REPOSITORY "https://github.com/catchorg/Catch2.git" GIT_TAG v2.13.4 GIT_SHALLOW ON)

  set (BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
  _Generator_MakeAvailable (benchmark GIT_REPOSITORY "https://github.com/google/benchmark.git" GIT_TAG v1.5.2 GIT_SHALLOW ON)
endif()
