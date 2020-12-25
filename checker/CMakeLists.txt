cmake_minimum_required(VERSION 3.15)

project (CodingGuidelinesChecker)
find_package (Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(
  lyra_content
  GIT_REPOSITORY "https://github.com/bfgroup/Lyra.git"
  GIT_TAG 1.2
  )
FetchContent_Declare(
  json_content
  GIT_REPOSITORY "https://github.com/nlohmann/json.git"
  GIT_TAG v3.7.3
  )
FetchContent_Declare(
  fmt_content
  GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
  GIT_TAG 7.1.3
  )
FetchContent_Declare(
  re2_content
  GIT_REPOSITORY "https://github.com/google/re2.git"
  GIT_TAG 2019-12-01
  )

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(RE2_BUILD_TESTING OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(lyra_content)
FetchContent_MakeAvailable(json_content)
FetchContent_MakeAvailable(fmt_content)
FetchContent_MakeAvailable(re2_content)

target_compile_options (re2 PRIVATE $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:CLANG>>:-Wno-error=unused-parameter -Wno-error=missing-field-initializers>)
target_include_directories (re2 INTERFACE $<BUILD_INTERFACE:${re2_content_SOURCE_DIR}>)

add_library (core.${PROJECT_NAME} STATIC
  "src/parameter.cpp"
  "src/regex.cpp"
  "src/text.cpp"
  "include/cxx20/syncstream"
  "include/feedback/async.h"
  "include/feedback/container.h"
  "include/feedback/format.h"
  "include/feedback/io.h"
  "include/feedback/parameter.h"
  "include/feedback/regex.h"
  "include/feedback/text.h"
  )
target_link_libraries (core.${PROJECT_NAME}
  PUBLIC ${CMAKE_THREAD_LIBS_INIT}
  PUBLIC BFG::Lyra
  PUBLIC fmt::fmt-header-only
  PRIVATE re2::re2
  )
target_include_directories (core.${PROJECT_NAME}
  PUBLIC include
  )
target_compile_options (core.${PROJECT_NAME}
  PUBLIC $<$<CXX_COMPILER_ID:MSVC>:-permissive->
  )

add_executable (${PROJECT_NAME}
  "src/main.cpp"
  )
target_link_libraries (${PROJECT_NAME}
  PRIVATE core.${PROJECT_NAME}
  PRIVATE nlohmann_json::nlohmann_json
  )

set_target_properties (core.${PROJECT_NAME} ${PROJECT_NAME}
  PROPERTIES CXX_STANDARD 17
  )

option (CODING_GUIDELINES_BUILD_TESTING "enable tests for this package")
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR CODING_GUIDELINES_BUILD_TESTING)
  include(CTest)

  if (BUILD_TESTING)
    FetchContent_Declare(
      catch2_content
      GIT_REPOSITORY "https://github.com/catchorg/Catch2.git"
      GIT_TAG v2.11.0
      )
    FetchContent_Declare(
      benchmark_content
      GIT_REPOSITORY "https://github.com/google/benchmark.git"
      GIT_TAG v1.5.0
      )

    set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")

    FetchContent_MakeAvailable(catch2_content)
    FetchContent_MakeAvailable(benchmark_content)

    add_executable (test.${PROJECT_NAME}
      "src/test.container.cpp"
      "src/test.main.cpp"
      "src/test.regex.cpp"
      )
    target_link_libraries (test.${PROJECT_NAME}
      PRIVATE core.${PROJECT_NAME}
      PRIVATE Catch2::Catch2
      )
    add_test(NAME ${PROJECT_NAME}Tests
      COMMAND $<TARGET_FILE:test.${PROJECT_NAME}>
      )

    add_executable (benchmark.${PROJECT_NAME}
      "src/benchmark.main.cpp"
      "src/benchmark.regex.cpp"
      )
    target_link_libraries (benchmark.${PROJECT_NAME}
      PRIVATE core.${PROJECT_NAME}
      PRIVATE benchmark
      )
    add_test(NAME ${PROJECT_NAME}Benchmarks
      COMMAND $<TARGET_FILE:benchmark.${PROJECT_NAME}>
      )

    set_target_properties (benchmark.${PROJECT_NAME} test.${PROJECT_NAME}
      PROPERTIES CXX_STANDARD 17
      )
  endif()
endif()
