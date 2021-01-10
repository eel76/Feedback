cmake_minimum_required (VERSION 3.18)

project (example)
set_property(GLOBAL PROPERTY USE_FOLDERS true)

add_executable (example main.cpp)

add_executable (excluded_example main.cpp)
set_target_properties(excluded_example PROPERTIES EXCLUDE_FROM_FEEDBACK TRUE)

include(FetchContent)

# FIXME: Repository URL eintragen

FetchContent_Declare(Feedback SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(Feedback)

set (CODING_GUIDELINES_WORKFLOW "developer" CACHE STRING "Coding guidelines workflow")
set_property(CACHE CODING_GUIDELINES_WORKFLOW PROPERTY STRINGS "ci" "maintainer" "component_builder" "developer" "solution_provider" "disabled")

set (CODING_GUIDELINES_RELEVANT_CHANGES "all" CACHE STRING "Coding guidelines relevant changes")
set_property(CACHE CODING_GUIDELINES_RELEVANT_CHANGES PROPERTY STRINGS "all" "modified" "modified_or_staged" "staged" "staged_or_committed" "committed")

Feedback_CreateDefaultWorkflows ()
Feedback_FindTargets (all_targets DIRECTORY .)
Feedback_AddRules (CodingGuidelines RULES_FILE warnings.json WORKFLOW "${CODING_GUIDELINES_WORKFLOW}" RELEVANT_CHANGES "${CODING_GUIDELINES_RELEVANT_CHANGES}" TARGETS ${all_targets})
