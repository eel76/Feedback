
message (STATUS "feedback examples")

add_executable (client main.cpp)

Feedback_ReportErrors (test_errors RULES errors.json TARGETS client)
