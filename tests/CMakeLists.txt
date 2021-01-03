
message (STATUS "feedback examples")

add_executable (client main.cpp)

Feedback_Create (test_errors RULES_FILE errors.json TARGETS client)
