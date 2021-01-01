cmake_minimum_required (VERSION 3.18)

project (feedback)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include (CTest)
endif ()

find_package (Git REQUIRED)

include ("feedback.cmake")

add_subdirectory("checker")

if (NOT CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
# FIXME
#  target_compile_options (FeedbackChecker BEFORE PRIVATE $<$<CONFIG:Debug>:-O2> $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:-Ob2>)
endif ()

RelevantTargetsInDirectory (targets "${CMAKE_CURRENT_LIST_DIR}/checker")

ExcludeFromFeedback (${targets})
GroupInFeedbackFolder (${targets})

option (FEEDBACK_BUILD_TESTING "Enable testing for ${PROJECT_NAME} project.")

if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR FEEDBACK_BUILD_TESTING) AND BUILD_TESTING)
  add_subdirectory(tests)
endif()


# FIXME
function (feedback_test)
  message (STATUS "just a test")
endfunction ()