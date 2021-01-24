cmake_minimum_required (VERSION 3.15)

add_executable (client main.cpp)

Feedback_Add (test_errors RULES errors.json TARGETS client)

# FIXME: test API, e.g. FindTargets
