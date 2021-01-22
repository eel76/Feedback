cmake_minimum_required (VERSION 3.15)

project (example)
set_property(GLOBAL PROPERTY USE_FOLDERS true)

add_subdirectory (extern)

add_executable (included1 main1.cpp)
add_executable (included2 main2.cpp)

add_executable (excluded1 main1.cpp)
Feedback_Exclude (example1 TARGETS excluded1)

Feedback_Add (example1 RULES rules.json DIRECTORIES "${CMAKE_SOURCE_DIR}")
Feedback_Add (example2 RULES rules.json DIRECTORIES "${CMAKE_SOURCE_DIR}")
