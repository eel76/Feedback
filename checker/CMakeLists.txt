cmake_minimum_required(VERSION 3.11)

project (CodingGuidelinesChecker)

include(FetchContent)

FetchContent_Declare(
  lyra_content
  GIT_REPOSITORY "http://bitbucket.volumegraphics.com:7990/scm/mirror/lyra.git"
  GIT_TAG master
  )

FetchContent_MakeAvailable(lyra_content)
FetchContent_Declare(
  json_content
  GIT_REPOSITORY "http://bitbucket.volumegraphics.com:7990/scm/mirror/json.git"
  GIT_TAG master
  )

set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(json_content)

FetchContent_Declare(
  fmt_content
  GIT_REPOSITORY "http://bitbucket.volumegraphics.com:7990/scm/mirror/fmt.git"
  GIT_TAG master
  )
FetchContent_MakeAvailable(fmt_content)

add_executable (${PROJECT_NAME})

target_sources (${PROJECT_NAME} PRIVATE "CodingGuidelinesChecker.cpp")
target_sources (${PROJECT_NAME} PRIVATE "include/async.h" "include/benchmark.h" "include/format.h" "include/parameter.h" "include/stream.h")
target_sources (${PROJECT_NAME} PRIVATE "include/syncstream")

target_include_directories (${PROJECT_NAME} PRIVATE include)

target_link_libraries (${PROJECT_NAME} PRIVATE BFG::Lyra)
target_link_libraries (${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries (${PROJECT_NAME} PRIVATE fmt::fmt-header-only)

target_link_options (${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-lpthread>)

set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
