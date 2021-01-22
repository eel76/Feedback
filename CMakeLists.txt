cmake_minimum_required (VERSION 3.18)

project (feedback
  VERSION 1.0.0
  DESCRIPTION "A simple to use feedback system for CMake based C++ projects."
  HOMEPAGE_URL "https://gitlab.volumegraphics.com/reinbach/feedback"
  LANGUAGES CXX
  )

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

if (NOT DEFINED FEEDBACK_MASTER_PROJECT)
  set (FEEDBACK_MASTER_PROJECT false)

  if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set (FEEDBACK_MASTER_PROJECT true)
    set_property(GLOBAL PROPERTY USE_FOLDERS true)
    include (CTest)
  endif ()
endif ()

option (FEEDBACK_BUILD_TESTING "Enable testing for the feedback project." "${FEEDBACK_MASTER_PROJECT}")

# adding the genrator as an external project is preferable because we
#  * build the generator always in release configuration
#  * do not inherit compile options from the client project
#  * achieve a faster CMake configuration/generation of the client project
#  * can use the feedback system for our project itself
# even though we
#  * have to define imported targets manually
#  * will have a permanent build check
  
include (ExternalProject)
ExternalProject_Add(feedback-generator-build
  SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/generator"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generator-build"
  CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_TESTING=${FEEDBACK_BUILD_TESTING}"
  BUILD_COMMAND "${CMAKE_COMMAND}" "--build" "${CMAKE_CURRENT_BINARY_DIR}/generator-build" "--config" "Release"
  BUILD_ALWAYS "${FEEDBACK_MASTER_PROJECT}"
  INSTALL_COMMAND ""
  EXCLUDE_FROM_ALL)

add_executable (feedback-generator IMPORTED GLOBAL)
add_dependencies(feedback-generator feedback-generator-build)

ExternalProject_Get_Property(feedback-generator-build binary_dir)
set_target_properties(feedback-generator PROPERTIES IMPORTED_LOCATION "${binary_dir}/Release/generator${CMAKE_EXECUTABLE_SUFFIX}")

include ("feedback.cmake")

if (FEEDBACK_MASTER_PROJECT)
  set (GENERATOR_BUILD_TESTING "${FEEDBACK_BUILD_TESTING}")
  add_subdirectory("generator")

  Feedback_FindTargets (generator_targets DIRECTORIES "generator")
  set_target_properties (${generator_targets} PROPERTIES FOLDER "generator")

  # add a target for this cmakelist.txt and the included cmake files

  Feedback_Exclude (project_guidelines DIRECTORIES "generator/extern")
  Feedback_Add (project_guidelines RULES project_guidelines.json DIRECTORIES "${CMAKE_SOURCE_DIR}")
endif ()

if (BUILD_TESTING AND FEEDBACK_BUILD_TESTING)
  add_subdirectory(tests)
endif()

