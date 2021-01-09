cmake_policy (VERSION 3.7.0)

find_package (Git REQUIRED)

include (feedback_private.cmake)

function (Feedback_FindTargets targets_variable)
  cmake_parse_arguments (Feedback "" "DIRECTORY" "TYPES" ${ARGN})

  if (DEFINED Feedback_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${Feedback_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED Feedback_DIRECTORY)
    message (FATAL_ERROR "No directory given.")
  endif ()

  if (NOT DEFINED Feedback_TYPES)
    set (Feedback_TYPES STATIC_LIBRARY MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE)
  endif ()

  get_directory_property(directory_targets DIRECTORY "${Feedback_DIRECTORY}" BUILDSYSTEM_TARGETS)
  list(JOIN Feedback_TYPES " " types)

  foreach (target IN LISTS directory_targets)
    get_target_property (type "${target}" TYPE)
    if (" ${types} " MATCHES " ${type} ")
      list (APPEND targets "${target}")
    endif ()
  endforeach ()

  get_directory_property(subdirectories DIRECTORY "${Feedback_DIRECTORY}" SUBDIRECTORIES)

  foreach (subdirectory IN LISTS subdirectories)
    Feedback_FindTargets (directory_targets DIRECTORY "${subdirectory}" TYPES ${Feedback_TYPES})
    list (APPEND targets ${subdirectory_targets})
  endforeach ()

  set (${targets_variable} ${targets} PARENT_SCOPE)
endfunction ()

function (Feedback_ReportWarnings name)
  cmake_parse_arguments (Feedback "" "RULES" "TARGETS" ${ARGN})

  if (DEFINED Feedback_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${Feedback_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED Feedback_RULES)
    message (FATAL_ERROR "No rules file given.")
  endif ()

  if (NOT DEFINED Feedback_TARGETS)
    message (FATAL_ERROR "No targets given.")
  endif ()

  get_filename_component (Feedback_RULES "${Feedback_RULES}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
  ConfigureFeedbackTargetFromTargets (WARNINGS ${name} "${Feedback_RULES}" ${Feedback_TARGETS})
endfunction ()

function (Feedback_ReportErrors name)
  cmake_parse_arguments (Feedback "" "RULES" "TARGETS" ${ARGN})

  if (DEFINED Feedback_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${Feedback_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED Feedback_RULES)
    message (FATAL_ERROR "No rules file given.")
  endif ()

  if (NOT DEFINED Feedback_TARGETS)
    message (FATAL_ERROR "No targets given.")
  endif ()

  get_filename_component (Feedback_RULES "${Feedback_RULES}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
  ConfigureFeedbackTargetFromTargets (ERRORS ${name} "${Feedback_RULES}" ${Feedback_TARGETS})
endfunction ()

# name FROM_FILE json WORKFLOW ci TARGETS ...
#function (Feedback_AddRules name)
#endfunction ()

#function (Feedback_ConfigureRules type)
#  cmake_parse_arguments (Feedback "" "CHECK;RESPONSE" "" ${ARGN})

#  if (DEFINED Feedback_UNPARSED_ARGUMENTS)
#    message (FATAL_ERROR "Unparsed arguments: ${Feedback_UNPARSED_ARGUMENTS}")
#  endif ()

#  set(FEEDBACK_CHECK_xyz value CACHE INTERNAL "")

  # wenn etwas fehlt, dann nimm den default
#endfunction ()

#function (Feedback_ConfigureWorkflow workflow type)
#  if (workflow STREQUAL "CI")
#    Feedback_ConfigureRules (Requirement CHECK ALL_CODE_LINES     RESPONSE ERROR)
#    Feedback_ConfigureRules (Guideline   CHECK ALL_CODE_LINES     RESPONSE WARNING)
#    Feedback_ConfigureRules (Improvement CHECK ALL_CODE_LINES     RESPONSE WARNING)
#    Feedback_ConfigureRules (Suggestion  CHECK ALL_CODE_LINES     RESPONSE WARNING)
#  elseif(workflow STREQUAL "MAINTAINER")
#    Feedback_ConfigureRules (Requirement CHECK ALL_CODE_LINES     RESPONSE ERROR)
#    Feedback_ConfigureRules (Guideline   CHECK ALL_CODE_LINES     RESPONSE WARNING)
#    Feedback_ConfigureRules (Improvement CHECK ALL_CODE_LINES     RESPONSE WARNING)
#    Feedback_ConfigureRules (Suggestion  CHECK CHANGED_FILES      RESPONSE WARNING)
#  elseif(workflow STREQUAL "COMPONENT_BUILDER")
#    Feedback_ConfigureRules (Requirement CHECK ALL_CODE_LINES     RESPONSE ERROR)
#    Feedback_ConfigureRules (Guideline   CHECK ALL_CODE_LINES     RESPONSE WARNING)
#    Feedback_ConfigureRules (Improvement CHECK CHANGED_FILES      RESPONSE WARNING)
#    Feedback_ConfigureRules (Suggestion  CHECK CHANGED_FILES      RESPONSE WARNING)
#  elseif(workflow STREQUAL "DEVELOPER")
#    Feedback_ConfigureRules (Requirement CHECK ALL_CODE_LINES     RESPONSE ERROR)
#    Feedback_ConfigureRules (Guideline   CHECK ALL_CODE_LINES     RESPONSE WARNING)
#    Feedback_ConfigureRules (Improvement CHECK CHANGED_FILES      RESPONSE WARNING)
#    Feedback_ConfigureRules (Suggestion  CHECK CHANGED_CODE_LINES RESPONSE WARNING)
#  elseif(workflow STREQUAL "SOLUTION_PROVIDER")
#    Feedback_ConfigureRules (Requirement CHECK CHANGED_FILES      RESPONSE ERROR)
#    Feedback_ConfigureRules (Guideline   CHECK CHANGED_CODE_LINES RESPONSE WARNING)
#    Feedback_ConfigureRules (Improvement CHECK NO_CODE_LINES      RESPONSE WARNING)
#    Feedback_ConfigureRules (Suggestion  CHECK NO_CODE_LINES      RESPONSE WARNING)
#  elseif(workflow STREQUAL "DISABLED")
#    Feedback_ConfigureRules (Requirement CHECK NO_CODE_LINES      RESPONSE ERROR)
#    Feedback_ConfigureRules (Guideline   CHECK NO_CODE_LINES      RESPONSE WARNING)
#    Feedback_ConfigureRules (Improvement CHECK NO_CODE_LINES      RESPONSE WARNING)
#    Feedback_ConfigureRules (Suggestion  CHECK NO_CODE_LINES      RESPONSE WARNING)
#  else()
#    # FIXME parse den string als JSON mit dem workflow?
#    message (FATAL_ERROR "Unknown workflow: ${workflow}")
#  endif()
#endfunction ()

