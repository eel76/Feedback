cmake_minimum_required (VERSION 3.18)

project (example)

include(FetchContent)

FetchContent_Declare(feedback_content SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(feedback_content)

add_executable (example main.cpp)
ConfigureFeedbackForTargets (warnings.json example)
# ConfigureFeedbackForTargets (errors.json example)

set_property(GLOBAL PROPERTY USE_FOLDERS true)
