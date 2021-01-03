cmake_minimum_required (VERSION 3.18)

project (example)
set_property(GLOBAL PROPERTY USE_FOLDERS true)

include(FetchContent)

FetchContent_Declare(Feedback SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(Feedback)

add_executable (example main.cpp)
Feedback_Create (example_warnings RULES_FILE warnings.json TARGETS example)
Feedback_Create (example_errors RULES_FILE errors.json TARGETS example)

