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

function (Feedback_AddRules name)
  cmake_parse_arguments (Feedback "" "RULES_FILE;WORKFLOW;RELEVANT_CHANGES" "TARGETS" ${ARGN})

  if (DEFINED Feedback_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${Feedback_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED Feedback_RULES_FILE)
    message (FATAL_ERROR "No rules file given.")
  endif ()

  if (NOT DEFINED Feedback_WORKFLOW)
    message (FATAL_ERROR "No workflow given.")
  endif ()

  if (NOT DEFINED Feedback_RELEVANT_CHANGES)
    message (FATAL_ERROR "No relevant changes given.")
  endif ()

  if (NOT DEFINED Feedback_TARGETS)
    message (FATAL_ERROR "No targets given.")
  endif ()

  get_filename_component (Feedback_RULES_FILE "${Feedback_RULES_FILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
  ConfigureFeedbackTargetFromTargets ("${name}" "${Feedback_RULES_FILE}" "${Feedback_WORKFLOW}" ${Feedback_TARGETS})
endfunction ()

function (Feedback_CreateWorkflow name)
  set ("FEEDBACK_WORKFLOW_${name}" "FeedbackWorkflow" CACHE INTERNAL .)
  unset ("FEEDBACK_WORKFLOW_${name}_TYPES" CACHE)
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

function (Feedback_CreateDefaultWorkflows)
  Feedback_CreateWorkflow (ci)
  Feedback_CreateWorkflow (maintainer)
  Feedback_CreateWorkflow (component_builder)
  Feedback_CreateWorkflow (developer)
  Feedback_CreateWorkflow (solution_provider)
  Feedback_CreateWorkflow (disabled)

  Feedback_ConfigureWorkflow (ci                TYPE requirement CHECK all_code_lines     RESPONSE error)
  Feedback_ConfigureWorkflow (ci                TYPE guideline   CHECK all_code_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (ci                TYPE improvement CHECK all_code_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (ci                TYPE suggestion  CHECK all_code_lines     RESPONSE warning)

  Feedback_ConfigureWorkflow (maintainer        TYPE requirement CHECK all_code_lines     RESPONSE error)
  Feedback_ConfigureWorkflow (maintainer        TYPE guideline   CHECK all_code_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (maintainer        TYPE improvement CHECK all_code_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (maintainer        TYPE suggestion  CHECK changed_files      RESPONSE warning)

  Feedback_ConfigureWorkflow (component_builder TYPE requirement CHECK all_code_lines     RESPONSE error)
  Feedback_ConfigureWorkflow (component_builder TYPE guideline   CHECK all_code_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (component_builder TYPE improvement CHECK changed_files      RESPONSE warning)
  Feedback_ConfigureWorkflow (component_builder TYPE suggestion  CHECK changed_files      RESPONSE warning)

  Feedback_ConfigureWorkflow (developer         TYPE requirement CHECK all_code_lines     RESPONSE error)
  Feedback_ConfigureWorkflow (developer         TYPE guideline   CHECK all_code_lines     RESPONSE warning)
  Feedback_ConfigureWorkflow (developer         TYPE improvement CHECK changed_files      RESPONSE warning)
  Feedback_ConfigureWorkflow (developer         TYPE suggestion  CHECK changed_code_lines RESPONSE warning)
   
  Feedback_ConfigureWorkflow (solution_provider TYPE requirement CHECK changed_files      RESPONSE error)
  Feedback_ConfigureWorkflow (solution_provider TYPE guideline   CHECK changed_code_lines RESPONSE warning)
  Feedback_ConfigureWorkflow (solution_provider TYPE improvement CHECK no_code_lines      RESPONSE warning)
  Feedback_ConfigureWorkflow (solution_provider TYPE suggestion  CHECK no_code_lines      RESPONSE warning)

  Feedback_ConfigureWorkflow (disabled          TYPE requirement CHECK no_code_lines      RESPONSE error)
  Feedback_ConfigureWorkflow (disabled          TYPE guideline   CHECK no_code_lines      RESPONSE warning)
  Feedback_ConfigureWorkflow (disabled          TYPE improvement CHECK no_code_lines      RESPONSE warning)
  Feedback_ConfigureWorkflow (disabled          TYPE suggestion  CHECK no_code_lines      RESPONSE warning)
endfunction ()
