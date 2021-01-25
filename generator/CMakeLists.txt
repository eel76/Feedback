cmake_minimum_required(VERSION 3.15)

project (generator LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include (CTest)
  set_property(GLOBAL PROPERTY USE_FOLDERS true)

  set (GENERATOR_BUILD_TESTS ON CACHE INTERNAL "")
else ()
  option (GENERATOR_BUILD_TESTS "Enable testing for ${PROJECT_NAME} project." OFF)
endif ()

find_package (Threads REQUIRED)
add_subdirectory (extern)

add_library (${PROJECT_NAME}.core STATIC
  "core/src/generator/json.cpp"
  "core/src/generator/output.cpp"
  "core/src/generator/regex.cpp"
  "core/src/generator/scm.cpp"
  "core/src/generator/text.cpp"
  "core/include/cxx20/syncstream"
  "core/include/generator/async.h"
  "core/include/generator/container.h"
  "core/include/generator/feedback.h"
  "core/include/generator/format.h"
  "core/include/generator/json.h"
  "core/include/generator/macros.h"
  "core/include/generator/output.h"
  "core/include/generator/regex.h"
  "core/include/generator/scm.h"
  "core/include/generator/text.h"
  )
target_link_libraries (${PROJECT_NAME}.core
  PUBLIC ${CMAKE_THREAD_LIBS_INIT}
  PUBLIC fmt::fmt-header-only
  PRIVATE nlohmann_json::nlohmann_json
  PRIVATE re2::re2
  )
target_include_directories (${PROJECT_NAME}.core
  PUBLIC "core/include"
  )
target_compile_options (${PROJECT_NAME}.core
  PUBLIC $<$<CXX_COMPILER_ID:MSVC>:-permissive- -W4 -WX> $<$<CXX_COMPILER_ID:Clang,GNU>:-Wall -Werror>
  )

add_executable (${PROJECT_NAME}
  "src/generator/main.cpp"
  "src/generator/cli.cpp"
  "include/generator/cli.h"
  )
target_link_libraries (${PROJECT_NAME}
  PRIVATE ${PROJECT_NAME}.core
  PRIVATE bfg::Lyra
  PRIVATE $<$<CXX_COMPILER_ID:Clang,GNU>:-ltbb>
  )
target_include_directories (${PROJECT_NAME}
  PRIVATE "include"
  )

if (BUILD_TESTING AND GENERATOR_BUILD_TESTS)
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
  set_target_properties ("${PROJECT_NAME}.test" "${PROJECT_NAME}.benchmark" PROPERTIES FOLDER "tests")
endif()
