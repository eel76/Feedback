cmake_minimum_required (VERSION 3.16)

project (feedback_main
  VERSION 1.0.0
  DESCRIPTION "A simple to use feedback system for CMake based C++ projects."
  HOMEPAGE_URL "https://gitlab.volumegraphics.com/reinbach/feedback"
  LANGUAGES CXX
  )

if (NOT DEFINED FEEEBACK_MAIN_PROJECT)
  set (FEEEBACK_MAIN_PROJECT false)

  if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set (FEEEBACK_MAIN_PROJECT true)
  endif ()
endif ()

option (FEEDBACK_BUILD_TESTS "Build tests for the feedback project." "${FEEEBACK_MAIN_PROJECT}")
option (FEEDBACK_BUILD_EXAMPLES "Build examples for the feedback project." "${FEEEBACK_MAIN_PROJECT}")

if (FEEEBACK_MAIN_PROJECT)
  include (CTest)
  set_property(GLOBAL PROPERTY USE_FOLDERS true)
  add_library (modules_loaded INTERFACE)
endif ()

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/module")

include (Feedback)

# FIXME: add a target for this cmakelist.txt, the included cmake files, the README, ...

if (FEEDBACK_BUILD_TESTS AND BUILD_TESTING)
  add_subdirectory(tests)

  # another test scenario: build all examples like a project user would do

  include (ExternalProject)
  ExternalProject_Add(examples-test
    SOURCE_DIR "${feedback_main_SOURCE_DIR}/examples"
    BINARY_DIR "${feedback_main_BINARY_DIR}/examples-test"
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

  add_library (feedback_module STATIC EXCLUDE_FROM_ALL)
  target_link_libraries (feedback_module PRIVATE modules_loaded)

  target_sources (feedback_module PRIVATE "module/dummy.cpp")

  set ("FEEDBACK_GUIDELINES_WORKFLOW" "maintainer" CACHE STRING "Workflow for feedback project guidelines")
  set_property (CACHE "FEEDBACK_GUIDELINES_WORKFLOW" PROPERTY STRINGS "maintainer" "just_my_code")

  Feedback_Exclude ("Project guidelines" DIRECTORIES "generator/extern")
  Feedback_Add ("Project guidelines" RULES "doc/guidelines/rules.json" WORKFLOW "doc/guidelines/${FEEDBACK_GUIDELINES_WORKFLOW}_workflow.json" DIRECTORIES "${CMAKE_SOURCE_DIR}")
endif ()
