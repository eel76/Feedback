cmake_minimum_required (VERSION 3.18)

project (example)
set_property(GLOBAL PROPERTY USE_FOLDERS true)

add_subdirectory (extern)

add_executable (included main.cpp)
add_executable (excluded main.cpp)

Feedback_Exclude (excluded)
Feedback_Add (example RULES rules.json DIRECTORIES "${CMAKE_SOURCE_DIR}")
