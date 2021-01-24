cmake_minimum_required (VERSION 3.15)

project (examples LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS true)

option (FEEDBACK_AVAILABLE "Feedback project is already available." OFF)

if (NOT FEEDBACK_AVAILABLE)
  # FIXME: Unsere repository URL eintragen und mit git shallow clonen

  include (FetchContent)

  FetchContent_Declare (feedback SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
  FetchContent_MakeAvailable(feedback)
endif ()

add_subdirectory(excluded_targets)
add_subdirectory(multiple_feedbacks)
