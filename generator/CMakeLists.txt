cmake_minimum_required(VERSION 3.15)

project (generator)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include (CTest)
  set_property(GLOBAL PROPERTY USE_FOLDERS true)

  set (GENERATOR_BUILD_TESTING ON CACHE INTERNAL "")
else ()
  option (GENERATOR_BUILD_TESTING "Enable testing for ${PROJECT_NAME} project." OFF)
endif ()

add_subdirectory (extern)

add_library (${PROJECT_NAME}.core STATIC
  "src/regex.cpp"
  "src/scm.cpp"
  "src/text.cpp"
  "include/cxx20/syncstream"
  "include/feedback/async.h"
  "include/feedback/container.h"
  "include/feedback/format.h"
  "include/feedback/macros.h"
  "include/feedback/regex.h"
  "include/feedback/scm.h"
  "include/feedback/stream.h"
  "include/feedback/text.h"
  )
target_link_libraries (${PROJECT_NAME}.core
  PUBLIC ${CMAKE_THREAD_LIBS_INIT}
  PUBLIC fmt::fmt-header-only
  PRIVATE re2::re2
  )
target_include_directories (${PROJECT_NAME}.core
  PUBLIC include
  )
target_compile_options (${PROJECT_NAME}.core
  PUBLIC $<$<CXX_COMPILER_ID:MSVC>:-permissive- -W4 -WX>
  )

add_executable (${PROJECT_NAME}
  "src/${PROJECT_NAME}/main.cpp"
  "src/${PROJECT_NAME}/cli.cpp"
  "include/${PROJECT_NAME}/cli.h"
  )
target_link_libraries (${PROJECT_NAME}
  PRIVATE ${PROJECT_NAME}.core
  PRIVATE nlohmann_json::nlohmann_json
  PRIVATE bfg::Lyra
  )
target_include_directories (${PROJECT_NAME}
  PRIVATE include
  )

set_target_properties (${PROJECT_NAME}.core ${PROJECT_NAME}
  PROPERTIES CXX_STANDARD 17
  )

if (GENERATOR_BUILD_TESTING AND BUILD_TESTING)
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
