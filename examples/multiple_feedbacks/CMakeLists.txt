cmake_minimum_required (VERSION 3.15)

add_executable (multiple_feedbacks main.cpp)

Feedback_Add (feedback1 RULES ../rules.json DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
Feedback_Add (feedback2 RULES ../rules.json DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
