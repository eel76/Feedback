
message (STATUS "feedback examples")

add_executable (client main.cpp)

ConfigureFeedbackForTargets ("errors.json" client)
