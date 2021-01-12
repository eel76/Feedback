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

function (Feedback_Add name)
  cmake_parse_arguments (Feedback "" "RULES;WORKFLOW;RELEVANT_CHANGES" "TARGETS" ${ARGN})

  if (DEFINED Feedback_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${Feedback_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED Feedback_RULES)
    message (FATAL_ERROR "No rules file given.")
  endif ()

  if (NOT DEFINED Feedback_WORKFLOW)
    set (Feedback_WORKFLOW "configurable")
  endif ()

  if (NOT DEFINED Feedback_RELEVANT_CHANGES)
    set (Feedback_RELEVANT_CHANGES "configurable")
  endif ()

  if (NOT DEFINED Feedback_TARGETS)
    message (FATAL_ERROR "No targets given.")
  endif ()

  if (Feedback_WORKFLOW STREQUAL "configurable")
    set ("${name}_WORKFLOW" "developer" CACHE STRING "Workflow for '${name}' feedback")
    set_property(CACHE "${name}_WORKFLOW" PROPERTY STRINGS "ci" "maintainer" "component_builder" "developer" "solution_provider" "disabled")

    set (Feedback_WORKFLOW "${${name}_WORKFLOW}")
  endif ()

  if (Feedback_RELEVANT_CHANGES STREQUAL "configurable")
    set ("${name}_RELEVANT_CHANGES" "all" CACHE STRING "Relevant changes for '${name}' feedback")
    set_property(CACHE "${name}_RELEVANT_CHANGES" PROPERTY STRINGS "all" "modified" "modified_or_staged" "staged" "staged_or_committed" "committed")

    set (Feedback_RELEVANT_CHANGES "${${name}_RELEVANT_CHANGES}")
  endif ()

  set (is_disabled TRUE)

  foreach (type IN LISTS "FEEDBACK_WORKFLOW_${Feedback_WORKFLOW}_TYPES")
    if (" all_files all_lines changed_files changed_lines " MATCHES " ${FEEDBACK_WORKFLOW_${Feedback_WORKFLOW}_CHECK_${type}} ")
      set (is_disabled FALSE)
    endif ()
  endforeach ()

  if (NOT is_disabled)
    get_filename_component (Feedback_RULES "${Feedback_RULES}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    ConfigureFeedbackTargetFromTargets ("${name}" "${Feedback_RULES}" "${Feedback_WORKFLOW}" "${Feedback_RELEVANT_CHANGES}" ${Feedback_TARGETS})
  endif ()
endfunction ()

function (Feedback_AddWorkflow name)
  set ("FEEDBACK_WORKFLOW_${name}" "FeedbackWorkflow" CACHE INTERNAL .)
  unset ("FEEDBACK_WORKFLOW_${name}_TYPES" CACHE)
endfunction ()

function (Feedback_IsWorkflowDisabled name)

  if (NOT "${FEEDBACK_WORKFLOW_${name}}" STREQUAL "FeedbackWorkflow")
    message (FATAL_ERROR "${name} is not a feedback workflow")
  endif()
endfunction ()

function (Feedback_ConfigureWorkflow name)
  if (NOT "${FEEDBACK_WORKFLOW_${name}}" STREQUAL "FeedbackWorkflow")
    message (FATAL_ERROR "${name} is not a feedback workflow")
  endif()

  cmake_parse_arguments (Feedback "" "TYPE;CHECK;RESPONSE" "" ${ARGN})

  if (DEFINED Feedback_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${Feedback_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED Feedback_TYPE)
    message (FATAL_ERROR "No type given.")
  endif ()

  if (NOT DEFINED Feedback_CHECK)
    set (Feedback_CHECK "${FEEDBACK_WORKFLOW_${name}_CHECK_${Feedback_TYPE}}")
    if (NOT Feedback_CHECK)
      set (Feedback_CHECK "all_files")
    endif ()
  endif ()

  if (NOT DEFINED Feedback_RESPONSE)
    set (Feedback_RESPONSE "${FEEDBACK_WORKFLOW_${name}_RESPONSE_${Feedback_TYPE}}")
    if (NOT Feedback_RESPONSE)
      set (Feedback_RESPONSE "warning")
    endif ()
  endif ()

  list (APPEND "FEEDBACK_WORKFLOW_${name}_TYPES" "${Feedback_TYPE}")
  list (REMOVE_DUPLICATES "FEEDBACK_WORKFLOW_${name}_TYPES")

  set ("FEEDBACK_WORKFLOW_${name}_TYPES" ${FEEDBACK_WORKFLOW_${name}_TYPES} CACHE INTERNAL .)
  set ("FEEDBACK_WORKFLOW_${name}_CHECK_${Feedback_TYPE}" "${Feedback_CHECK}" CACHE INTERNAL .)
  set ("FEEDBACK_WORKFLOW_${name}_RESPONSE_${Feedback_TYPE}" "${Feedback_RESPONSE}" CACHE INTERNAL .)
endfunction ()

function (Feedback_AddDefaultWorkflows)
  Feedback_AddWorkflow (ci)
  Feedback_AddWorkflow (maintainer)
  Feedback_AddWorkflow (component_builder)
  Feedback_AddWorkflow (developer)
  Feedback_AddWorkflow (solution_provider)
  Feedback_AddWorkflow (disabled)

  Feedback_ConfigureWorkflow (ci                TYPE requirement CHECK all_lines     RESPONSE error)
  Feedback_ConfigureWorkflow (ci                TYPE guideline   CHECK all_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (ci                TYPE improvement CHECK all_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (ci                TYPE suggestion  CHECK all_lines     RESPONSE warning)

  Feedback_ConfigureWorkflow (maintainer        TYPE requirement CHECK all_lines     RESPONSE error)
  Feedback_ConfigureWorkflow (maintainer        TYPE guideline   CHECK all_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (maintainer        TYPE improvement CHECK all_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (maintainer        TYPE suggestion  CHECK changed_files RESPONSE warning)

  Feedback_ConfigureWorkflow (component_builder TYPE requirement CHECK all_lines     RESPONSE error)
  Feedback_ConfigureWorkflow (component_builder TYPE guideline   CHECK all_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (component_builder TYPE improvement CHECK changed_files RESPONSE warning)
  Feedback_ConfigureWorkflow (component_builder TYPE suggestion  CHECK changed_files RESPONSE warning)

  Feedback_ConfigureWorkflow (developer         TYPE requirement CHECK all_lines     RESPONSE error)
  Feedback_ConfigureWorkflow (developer         TYPE guideline   CHECK all_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (developer         TYPE improvement CHECK changed_files RESPONSE warning)
  Feedback_ConfigureWorkflow (developer         TYPE suggestion  CHECK changed_lines RESPONSE warning)
   
  Feedback_ConfigureWorkflow (solution_provider TYPE requirement CHECK changed_files RESPONSE error)
  Feedback_ConfigureWorkflow (solution_provider TYPE guideline   CHECK changed_lines RESPONSE warning)
  Feedback_ConfigureWorkflow (solution_provider TYPE improvement CHECK no_lines      RESPONSE warning)
  Feedback_ConfigureWorkflow (solution_provider TYPE suggestion  CHECK no_lines      RESPONSE warning)

  Feedback_ConfigureWorkflow (disabled          TYPE requirement CHECK no_lines      RESPONSE error)
  Feedback_ConfigureWorkflow (disabled          TYPE guideline   CHECK no_lines      RESPONSE warning)
  Feedback_ConfigureWorkflow (disabled          TYPE improvement CHECK no_lines      RESPONSE warning)
  Feedback_ConfigureWorkflow (disabled          TYPE suggestion  CHECK no_lines      RESPONSE warning)
endfunction ()

Feedback_AddDefaultWorkflows ()
