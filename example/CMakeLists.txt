cmake_minimum_required (VERSION 3.18)

project (example)
set_property(GLOBAL PROPERTY USE_FOLDERS true)

include(FetchContent)

FetchContent_Declare(Feedback SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(Feedback)

add_executable (example main.cpp)

add_executable (excluded_example main.cpp)
set_target_properties(excluded_example PROPERTIES EXCLUDE_FROM_FEEDBACK TRUE)

Feedback_FindTargets (all_targets DIRECTORY .)

Feedback_ReportWarnings (example_warnings RULES warnings.json TARGETS ${all_targets})
Feedback_ReportErrors (example_errors RULES errors.json TARGETS ${all_targets})

