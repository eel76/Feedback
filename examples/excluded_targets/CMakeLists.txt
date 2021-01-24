cmake_minimum_required (VERSION 3.15)

add_executable (excluded main.cpp)

Feedback_Exclude (excluded_feedback TARGETS excluded)
Feedback_Add (excluded_feedback RULES ../rules.json DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
