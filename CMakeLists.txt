cmake_minimum_required (VERSION 3.18)

project (feedback_main
  VERSION 1.0.0
  DESCRIPTION "A simple to use feedback system for CMake based C++ projects."
  HOMEPAGE_URL "https://gitlab.volumegraphics.com/reinbach/feedback"
  LANGUAGES CXX
  )

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

if (NOT DEFINED FEEEBACK_MAIN_PROJECT)
  set (FEEEBACK_MAIN_PROJECT false)

  if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set (FEEEBACK_MAIN_PROJECT true)
    set_property(GLOBAL PROPERTY USE_FOLDERS true)
    include (CTest)
  endif ()
endif ()

option (FEEDBACK_BUILD_TESTS "Build tests for the feedback project." "${FEEEBACK_MAIN_PROJECT}")
option (FEEDBACK_BUILD_EXAMPLES "Build examples for the feedback project." "${FEEEBACK_MAIN_PROJECT}")

include ("module/feedback.cmake")

# FIXME: add a target for this cmakelist.txt, the included cmake files, the README, ...

if (FEEDBACK_BUILD_TESTS AND BUILD_TESTING)
  add_subdirectory(tests)

  # also: build all examples like a client would do

  include (ExternalProject)
  ExternalProject_Add(examples-test
    SOURCE_DIR "${feedback_main_SOURCE_DIR}/examples"
    BINARY_DIR "${feedback_main_BINARY_DIR}/examples-test"
#    CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_TESTING=OFF"
#    BUILD_COMMAND "${CMAKE_COMMAND}" "--build" "${CMAKE_CURRENT_BINARY_DIR}/examples-build" "--config" "Release"
    BUILD_ALWAYS "ON"
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL OFF)

  Feedback_GroupTargets (DIRECTORIES "tests" TARGETS "examples-test" FOLDER "tests")
endif()

if (FEEDBACK_BUILD_EXAMPLES)
  set (FEEDBACK_AVAILABLE "ON")
  add_subdirectory(examples)

  Feedback_GroupTargets (DIRECTORIES "examples" FOLDER "examples")
endif()

if (FEEEBACK_MAIN_PROJECT)
  set (GENERATOR_BUILD_TESTS "${FEEDBACK_BUILD_TESTS}")
  add_subdirectory("generator")

  Feedback_GroupTargets (DIRECTORIES "generator" FOLDER "generator")

  Feedback_Exclude (project_guidelines DIRECTORIES "generator/extern")
  Feedback_Add (project_guidelines RULES project_guidelines.json DIRECTORIES "${CMAKE_SOURCE_DIR}")
endif ()
