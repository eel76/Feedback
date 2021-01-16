cmake_minimum_required (VERSION 3.18)

project (feedback)
option (FEEDBACK_BUILD_TESTING "Enable testing for ${PROJECT_NAME} project.")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include (CTest)
  set_property(GLOBAL PROPERTY USE_FOLDERS true)
endif ()

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR FEEDBACK_BUILD_TESTING)
  set(GENERATOR_BUILD_TESTING ON CACHE INTERNAL "")
endif()

if (TRUE)
  add_subdirectory("generator")
else ()
  include (ExternalProject)
  ExternalProject_Add(generator_build SOURCE_DIR "generator"
# release config auswählen!!!  CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release"
  EXCLUDE_FROM_ALL)

  add_executable (generator IMPORTED)
  add_dependencies(generator generator_build)

  ExternalProject_Get_Property(generator_build binary_dir)
  set_target_properties(generator PROPERTIES IMPORTED_LOCATION "${binary_dir}/generator${CMAKE_EXECUTABLE_SUFFIX}")
endif ()

include ("feedback.cmake")

#Feedback_FindTargets (generator_targets DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/generator")
Feedback_FindTargets (generator_targets DIRECTORY "generator")
set_target_properties (${generator_targets} PROPERTIES FOLDER "generator")

Feedback_InternalTargets (${generator_targets})

if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR FEEDBACK_BUILD_TESTING) AND BUILD_TESTING)
  add_subdirectory(tests)
endif()
