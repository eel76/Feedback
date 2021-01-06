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

add_subdirectory("generator")

if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
# FIXME
#  target_compile_options (generator BEFORE PRIVATE $<$<CONFIG:Debug>:-O2> $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:-Ob2>)
endif ()

include ("feedback.cmake")

#Feedback_FindTargets (generator_targets DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/generator")
Feedback_FindTargets (generator_targets DIRECTORY "generator")
set_target_properties (${generator_targets} PROPERTIES FOLDER "generator")

Feedback_InternalTargets (${generator_targets})

if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR FEEDBACK_BUILD_TESTING) AND BUILD_TESTING)
  add_subdirectory(tests)
endif()

