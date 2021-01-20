cmake_minimum_required (VERSION 3.18)

project (feedback)

if (NOT DEFINED FEEDBACK_MASTER_PROJECT)
  set (FEEDBACK_MASTER_PROJECT false)

  if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set (FEEDBACK_MASTER_PROJECT true)
    set_property(GLOBAL PROPERTY USE_FOLDERS true)
    include (CTest)
  endif ()
endif ()

option (FEEDBACK_BUILD_TESTING "Enable testing for the feedback project." "${FEEDBACK_MASTER_PROJECT}")
option (FEEDBACK_NO_EXTERNAL_PROJECT "Feedback project does not use ExternalProject module." "${FEEDBACK_MASTER_PROJECT}")

if (FEEDBACK_NO_EXTERNAL_PROJECT)
  set (GENERATOR_BUILD_TESTING "${FEEDBACK_BUILD_TESTING}")
  add_subdirectory("generator")
else()

  # this is preferable for clients because we
  #  * build the generator always in release configuration
  #  * do not inherit compile options from the client project
  #  * achieve a faster CMake configuration/generation of the client project
  # even though we
  #  * have a build check every time

  include (ExternalProject)
  ExternalProject_Add(generator_build
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/generator"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generator"
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_TESTING=${FEEDBACK_BUILD_TESTING}"
    BUILD_COMMAND "${CMAKE_COMMAND}" "--build" "${CMAKE_CURRENT_BINARY_DIR}/generator" "--config" "Release"
    BUILD_ALWAYS "${FEEDBACK_MASTER_PROJECT}"
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL)

  add_executable (generator IMPORTED GLOBAL)
  add_dependencies(generator generator_build)

  ExternalProject_Get_Property(generator_build binary_dir)
  set_target_properties(generator PROPERTIES IMPORTED_LOCATION "${binary_dir}/Release/generator${CMAKE_EXECUTABLE_SUFFIX}")
endif()

function (Feedback_setup)
  include ("feedback.cmake")

  Feedback_FindTargets (feedback_targets DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}")
  Feedback_GroupTargetsInFolder (${feedback_targets} FOLDER "generator")
  Feedback_MarkInternalTargets (${feedback_targets})

  unset (feedback_targets)
endfunction ()

Feedback_setup ()

if (BUILD_TESTING AND FEEDBACK_BUILD_TESTING)
  add_subdirectory(tests)
endif()
