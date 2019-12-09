cmake_minimum_required(VERSION 3.11)

project (CodingGuidelinesChecker)
find_package (Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(
  lyra_content
  GIT_REPOSITORY "http://bitbucket.volumegraphics.com:7990/scm/mirror/lyra.git"
  GIT_TAG 1.2
  )
FetchContent_Declare(
  json_content
  GIT_REPOSITORY "http://bitbucket.volumegraphics.com:7990/scm/mirror/json.git"
  GIT_TAG v3.7.3
  )
FetchContent_Declare(
  fmt_content
  GIT_REPOSITORY "http://bitbucket.volumegraphics.com:7990/scm/mirror/fmt.git"
  GIT_TAG 6.1.0
  )
FetchContent_Declare(
  re2_content
  GIT_REPOSITORY "http://bitbucket.volumegraphics.com:7990/scm/mirror/re2.git"
  GIT_TAG 2019-12-01
  )
FetchContent_Declare(
  catch2_content
  GIT_REPOSITORY "http://bitbucket.volumegraphics.com:7990/scm/mirror/catch2.git"
  GIT_TAG v2.11.0
  )

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(RE2_BUILD_TESTING OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(lyra_content)
FetchContent_MakeAvailable(json_content)
FetchContent_MakeAvailable(fmt_content)
FetchContent_MakeAvailable(re2_content)
FetchContent_MakeAvailable(catch2_content)

target_compile_options (re2 BEFORE PRIVATE $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:CLANG>>:-Wno-error=unused-parameter -Wno-error=missing-field-initializers>)
target_include_directories (re2 INTERFACE $<BUILD_INTERFACE:${re2_content_SOURCE_DIR}>)

add_library (core.${PROJECT_NAME} STATIC
  "src/parameter.cpp"
  "src/regex.cpp"
  "include/async.h"
  "include/benchmark.h"
  "include/container.h"
  "include/format.h"
  "include/parameter.h"
  "include/regex.h"
  "include/stream.h"
  "include/syncstream"
  )
target_link_libraries (core.${PROJECT_NAME}
  PUBLIC ${CMAKE_THREAD_LIBS_INIT}
  PUBLIC BFG::Lyra
  PUBLIC nlohmann_json::nlohmann_json
  PUBLIC fmt::fmt-header-only
  PRIVATE re2::re2
  )
target_include_directories (core.${PROJECT_NAME}
  PUBLIC include
  )

add_executable (${PROJECT_NAME}
  "src/main.cpp"
  )
target_link_libraries (${PROJECT_NAME}
  PRIVATE core.${PROJECT_NAME}
  )

add_executable (test.${PROJECT_NAME}
  "src/test.container.cpp"
  "src/test.main.cpp"
  "src/test.regex.cpp"
  )
target_link_libraries (test.${PROJECT_NAME}
  PRIVATE core.${PROJECT_NAME}
  PRIVATE Catch2::Catch2
  )

set_target_properties (core.${PROJECT_NAME} test.${PROJECT_NAME} ${PROJECT_NAME}
  PROPERTIES CXX_STANDARD 17
  )
