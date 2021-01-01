cmake_minimum_required (VERSION 3.18)

project (feedback)
option (FEEDBACK_BUILD_TESTING "Enable testing for ${PROJECT_NAME} project.")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include (CTest)
  set_property(GLOBAL PROPERTY USE_FOLDERS true)
endif ()

find_package (Git REQUIRED)

include ("feedback.cmake")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR FEEDBACK_BUILD_TESTING)
  set(CHECKER_BUILD_TESTING ON CACHE INTERNAL "")
endif()

add_subdirectory("checker")

if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
# FIXME
#  target_compile_options (FeedbackChecker BEFORE PRIVATE $<$<CONFIG:Debug>:-O2> $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:-Ob2>)
endif ()

RelevantTargetsInDirectory (targets "${CMAKE_CURRENT_LIST_DIR}/checker")

ExcludeFromFeedback (${targets})
GroupInFeedbackFolder (${targets})

if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR FEEDBACK_BUILD_TESTING) AND BUILD_TESTING)
  add_subdirectory(tests)
endif()

