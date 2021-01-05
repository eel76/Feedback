cmake_minimum_required (VERSION 3.18)

project (example)
set_property(GLOBAL PROPERTY USE_FOLDERS true)

include(FetchContent)

FetchContent_Declare(Feedback SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(Feedback)

add_executable (example main.cpp)

add_executable (excluded_example main.cpp)
set_target_properties(excluded_example PROPERTIES EXCLUDE_FROM_FEEDBACK TRUE)

Feedback_ReportWarnings (example_warnings RULES warnings.json TARGETS example excluded_example)
Feedback_ReportErrors (example_errors RULES errors.json TARGETS example excluded_example)

#Feedback_DefaultWarnings (CHECK ALL_FILES)
#Feedback_DefaultWarnings (CHECK MODIFIED_FILES)
#Feedback_DefaultWarnings (CHECK CHANGED_LINES)

## Globale Einstellung für jeden Entwickler?
#Feedback_DelayWarnings (ALL_CHANGES)
#Feedback_DelayWarnings (STAGED_CHANGES)
#Feedback_DelayWarnings (COMMITTED_CHANGES)

#ALL_FILES
#MODIFIED_FILES
#MODIFIED_LINES

#ALL FILES
#ALL CHANGES
#NOT_PUSHED FILES
#NOT_PUSHED CHANGES
#NOT_COMMITTED FILES
#NOT_COMMITTED CHANGES
#NOT_STAGED FILES
#NOT_STAGED CHANGES

#z.B. Enhancements nur in LINES
