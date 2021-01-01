cmake_minimum_required(VERSION 3.15)

project (generator)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include (CTest)
endif ()

find_package (Threads REQUIRED)

include(FetchContent)

function (Feedback_MakeAvailable package target git_repository git_tag)
  if (NOT TARGET ${target})
    message (STATUS "Make available: ${git_repository} at ${git_tag}")
    cmake_parse_arguments(MakeAvailable "" "BUILD_TESTING_VARIABLE" "" ${ARGN})

    if (DEFINED MakeAvailable_UNPARSED_ARGUMENTS)
      message (FATAL_ERROR "Unparsed arguments: ${MakeAvailable_UNPARSED_ARGUMENTS}")
    endif ()

    if (DEFINED MakeAvailable_BUILD_TESTING_VARIABLE)
      set(${MakeAvailable_BUILD_TESTING_VARIABLE} OFF CACHE INTERNAL "")
    endif ()

    FetchContent_Declare(
      ${package}
      GIT_REPOSITORY ${git_repository}
      GIT_TAG ${git_tag}
      GIT_SHALLOW TRUE
      )
    FetchContent_MakeAvailable(${package})

    set (${package}_SOURCE_DIR ${${package}_SOURCE_DIR} PARENT_SCOPE)
  endif ()
endfunction ()

Feedback_MakeAvailable (fmt_package fmt::fmt-header-only "https://github.com/fmtlib/fmt.git" 7.1.3)
Feedback_MakeAvailable (lyra_package bfg::Lyra "https://github.com/bfgroup/Lyra.git" 1.5.1)
Feedback_MakeAvailable (nlohmann_json_package nlohmann_json::nlohmann_json "https://github.com/nlohmann/json.git" v3.9.1 BUILD_TESTING_VARIABLE JSON_BuildTests)
Feedback_MakeAvailable (re2_package re2::re2 "https://github.com/google/re2.git" 2020-11-01 BUILD_TESTING_VARIABLE RE2_BUILD_TESTING)

target_compile_options (re2 PRIVATE $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:CLANG>>:-Wno-error=unused-parameter -Wno-error=missing-field-initializers>)
target_include_directories (re2 INTERFACE $<BUILD_INTERFACE:${re2_package_SOURCE_DIR}>)

add_library (${PROJECT_NAME}.core STATIC
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
target_link_libraries (${PROJECT_NAME}.core
  PUBLIC ${CMAKE_THREAD_LIBS_INIT}
  PUBLIC bfg::Lyra
  PUBLIC fmt::fmt-header-only
  PRIVATE re2::re2
  )
target_include_directories (${PROJECT_NAME}.core
  PUBLIC include
  )
target_compile_options (${PROJECT_NAME}.core
  PUBLIC $<$<CXX_COMPILER_ID:MSVC>:-permissive->
  )

add_executable (${PROJECT_NAME}
  "src/main.cpp"
  )
target_link_libraries (${PROJECT_NAME}
  PRIVATE ${PROJECT_NAME}.core
  PRIVATE nlohmann_json::nlohmann_json
  )

set_target_properties (${PROJECT_NAME}.core ${PROJECT_NAME}
  PROPERTIES CXX_STANDARD 17
  )

if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR CHECKER_BUILD_TESTING) AND BUILD_TESTING)
  Feedback_MakeAvailable (catch2_package Catch2::Catch2 "https://github.com/catchorg/Catch2.git" v2.13.4)
  Feedback_MakeAvailable (benchmark_package benchmark "https://github.com/google/benchmark.git" v1.5.2 BUILD_TESTING_VARIABLE BENCHMARK_ENABLE_TESTING)

  add_executable (${PROJECT_NAME}.test
    "src/test.container.cpp"
    "src/test.main.cpp"
    "src/test.regex.cpp"
    )
  target_link_libraries (${PROJECT_NAME}.test
    PRIVATE ${PROJECT_NAME}.core
    PRIVATE Catch2::Catch2
    )
  add_test(NAME ${PROJECT_NAME}Tests
    COMMAND $<TARGET_FILE:${PROJECT_NAME}.test>
    )

  add_executable (${PROJECT_NAME}.benchmark
    "src/benchmark.main.cpp"
    "src/benchmark.regex.cpp"
    )
  target_link_libraries (${PROJECT_NAME}.benchmark
    PRIVATE ${PROJECT_NAME}.core
    PRIVATE benchmark
    )
  add_test(NAME ${PROJECT_NAME}Benchmarks
    COMMAND $<TARGET_FILE:${PROJECT_NAME}.benchmark>
    )

  set_target_properties (${PROJECT_NAME}.benchmark ${PROJECT_NAME}.test
    PROPERTIES CXX_STANDARD 17
    )
endif()
