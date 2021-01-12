cmake_minimum_required (VERSION 3.18)

project (example)
set_property(GLOBAL PROPERTY USE_FOLDERS true)

add_subdirectory (extern)

add_executable (example main.cpp)

add_executable (excluded_example main.cpp)
set_target_properties(excluded_example PROPERTIES EXCLUDE_FROM_FEEDBACK TRUE)

Feedback_FindTargets (all_targets DIRECTORY .)

Feedback_Add (DEVELOPER RULES example_rules.json TARGETS ${all_targets})
Feedback_Add (COMPONENT_BUILDER RULES example_rules.json WORKFLOW component_builder TARGETS ${all_targets})
Feedback_Add (SOLUTION_PROVIDER RULES example_rules.json WORKFLOW solution_provider TARGETS ${all_targets})
