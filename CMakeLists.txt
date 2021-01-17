cmake_minimum_required (VERSION 3.18)

project (feedback)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include (CTest)
  set_property(GLOBAL PROPERTY USE_FOLDERS true)
  set (FEEDBACK_BUILD_TESTING ON)
else ()
  option (FEEDBACK_BUILD_TESTING "Enable testing for ${PROJECT_NAME} project." OFF)
endif ()

if (DEFINED BUILD_TESTING AND BUILD_TESTING AND FEEDBACK_BUILD_TESTING)
  set (TESTING ON)
else ()
  set (TESTING OFF)
endif ()

option (FEEDBACK_NO_EXTERNAL_PROJECT "Feedback does not use ExternalProject module." OFF)

if (FEEDBACK_NO_EXTERNAL_PROJECT)
  set (GENERATOR_BUILD_TESTING "${TESTING}")
  add_subdirectory("generator")
else()

  # we do not add_subdirectory("generator") here because
  #  * always build generator in release configuration
  #  * do not inherit compile options from client project
  #  * faster CMake configuration/generation

  include (ExternalProject)
  ExternalProject_Add(generator_build
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/generator"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generator"
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_TESTING=${TESTING}"
    BUILD_COMMAND "${CMAKE_COMMAND}" "--build" "${CMAKE_CURRENT_BINARY_DIR}/generator" "--config" "Release"
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL)

  add_executable (generator IMPORTED GLOBAL)
  add_dependencies(generator generator_build)

  ExternalProject_Get_Property(generator_build binary_dir)
  set_target_properties(generator PROPERTIES IMPORTED_LOCATION "${binary_dir}/Release/generator${CMAKE_EXECUTABLE_SUFFIX}")
endif()

include ("feedback.cmake")

# FIXME: as a function?

Feedback_FindTargets (feedback_targets DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
Feedback_GroupTargetsInFolder ("generator" ${feedback_targets})
Feedback_InternalTargets (${feedback_targets})

if (TESTING)
  add_subdirectory(tests)
endif()
