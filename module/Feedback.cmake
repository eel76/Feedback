cmake_policy (VERSION 3.15)

include (FeedbackPrivate)

if (TARGET modules_loaded)
  target_sources (modules_loaded INTERFACE "${CMAKE_CURRENT_LIST_DIR}/Feedback.cmake")
endif ()

function (Feedback_FindTargets targets_variable)
  cmake_parse_arguments (parameter "" "IMPORTED" "DIRECTORIES;TARGETS;TYPES" ${ARGN})

  if (DEFINED parameter_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${parameter_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED parameter_DIRECTORIES AND NOT DEFINED parameter_TARGETS)
    message (FATAL_ERROR "No directories given.")
  endif ()

  if (NOT DEFINED parameter_IMPORTED)
    set (parameter_IMPORTED FALSE)
  endif ()

  if (NOT DEFINED parameter_TYPES)
    set (parameter_TYPES STATIC_LIBRARY MODULE_LIBRARY SHARED_LIBRARY OBJECT_LIBRARY EXECUTABLE)
  endif ()

  list(JOIN parameter_TYPES " " types)
  unset (targets)

  foreach (directory IN LISTS parameter_DIRECTORIES)
    get_directory_property(buildsystem_targets DIRECTORY "${directory}" BUILDSYSTEM_TARGETS)

    foreach (target IN LISTS buildsystem_targets)
      get_target_property (type "${target}" TYPE)
      get_target_property (imported "${target}" IMPORTED)

      if (" ${types} " MATCHES " ${type} " AND (NOT imported OR parameter_IMPORTED))
        list (APPEND targets "${target}")
      endif ()
    endforeach ()

    get_directory_property(subdirectories DIRECTORY "${directory}" SUBDIRECTORIES)

    if (subdirectories)
      Feedback_FindTargets (subdirectory_targets DIRECTORIES ${subdirectories} TYPES ${parameter_TYPES})
      list (APPEND targets ${subdirectory_targets})
    endif ()
  endforeach ()

  if (parameter_TARGETS)
    list (APPEND targets ${parameter_TARGETS})
    list (REMOVE_DUPLICATES targets)
  endif ()

  set (${targets_variable} ${targets} PARENT_SCOPE)
endfunction ()

function (Feedback_Exclude name)
  cmake_parse_arguments (parameter "" "" "" ${ARGN})

  string (MAKE_C_IDENTIFIER "${name}" feedback_target_library)
  string (TOLOWER "${feedback_target_library}" feedback_target_library)

  if (TARGET "${feedback_target_library}")
    message (AUTHOR_WARNING "Excluding targets *after* feedback '${name}' has already been added has no effect.")
  endif ()

  Feedback_FindTargets (targets ${parameter_UNPARSED_ARGUMENTS})

  foreach (target IN LISTS targets)
    get_target_property (excluded_from_feedback "${target}" EXCLUDED_FROM_FEEDBACK)

    if (NOT excluded_from_feedback)
      unset (excluded_from_feedback)
    endif ()

    list (APPEND excluded_from_feedback "(^${name}$)")
    set_target_properties ("${target}" PROPERTIES EXCLUDED_FROM_FEEDBACK "${excluded_from_feedback}")
  endforeach ()
endfunction ()

function (Feedback_GroupTargets)
  cmake_parse_arguments (parameter "" "FOLDER" "" ${ARGN})

  if (NOT parameter_FOLDER)
    message (FATAL_ERROR "No folder given.")
  endif ()

  Feedback_FindTargets (targets ${parameter_UNPARSED_ARGUMENTS})

  foreach (target IN LISTS targets)
    get_target_property (folder "${target}" FOLDER)

    if (folder)
      string (PREPEND folder "${parameter_FOLDER}/")
    else ()
      set (folder "${parameter_FOLDER}")
    endif ()

    set_target_properties ("${target}" PROPERTIES FOLDER "${folder}")
  endforeach ()
endfunction ()

function (Feedback_Add name)
  cmake_parse_arguments (parameter "" "RULES;WORKFLOW;RELEVANT_CHANGES" "" ${ARGN})

  if (NOT DEFINED parameter_RULES)
    message (FATAL_ERROR "No rules given.")
  endif ()

  if (NOT DEFINED parameter_WORKFLOW)
    get_property(parameter_WORKFLOW GLOBAL PROPERTY FEEDBACK_DEFAULT_WORKFLOW)
  endif ()

  if (NOT DEFINED parameter_RELEVANT_CHANGES)
    set (parameter_RELEVANT_CHANGES "default")
  endif ()

  if (parameter_RELEVANT_CHANGES STREQUAL "default")
    get_property(parameter_RELEVANT_CHANGES GLOBAL PROPERTY FEEDBACK_DEFAULT_RELEVANT_CHANGES)
  endif ()

  Feedback_FindTargets (targets ${parameter_UNPARSED_ARGUMENTS})

  if (NOT targets)
    message (FATAL_ERROR "No targets given.")
  endif ()

  get_filename_component (parameter_RULES "${parameter_RULES}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  get_filename_component (parameter_WORKFLOW "${parameter_WORKFLOW}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

  message (STATUS "Adding feedback: ${name}")
  ConfigureFeedbackTargetFromTargets ("${name}" "${parameter_RULES}" "${parameter_WORKFLOW}" "${parameter_RELEVANT_CHANGES}" ${targets})
endfunction ()

function (Feedback_SetDefaults)
  cmake_parse_arguments (parameter "" "WORKFLOW;RELEVANT_CHANGES" "" ${ARGN})

  if (DEFINED parameter_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${parameter_UNPARSED_ARGUMENTS}")
  endif ()

  if (DEFINED parameter_WORKFLOW)
    set_property(GLOBAL PROPERTY FEEDBACK_DEFAULT_WORKFLOW "${parameter_WORKFLOW}")
  endif ()

  if (DEFINED parameter_RELEVANT_CHANGES)
    set_property(GLOBAL PROPERTY FEEDBACK_DEFAULT_RELEVANT_CHANGES "${parameter_RELEVANT_CHANGES}")
  endif ()
endfunction ()

#  Feedback_AddWorkflow (ci)
#  Feedback_AddWorkflow (maintainer)
#  Feedback_AddWorkflow (component_builder)
#  Feedback_AddWorkflow (developer)
#  Feedback_AddWorkflow (solution_provider)
#  Feedback_AddWorkflow (disabled)

#  Feedback_ConfigureWorkflow (ci                TYPE requirement CHECK all_lines     RESPONSE error)
#  Feedback_ConfigureWorkflow (ci                TYPE guideline   CHECK all_lines     RESPONSE warning)
#  Feedback_ConfigureWorkflow (ci                TYPE improvement CHECK all_lines     RESPONSE warning)
#  Feedback_ConfigureWorkflow (ci                TYPE suggestion  CHECK all_lines     RESPONSE warning)

#  Feedback_ConfigureWorkflow (maintainer        TYPE requirement CHECK all_lines     RESPONSE error)
#  Feedback_ConfigureWorkflow (maintainer        TYPE guideline   CHECK all_lines     RESPONSE warning)
#  Feedback_ConfigureWorkflow (maintainer        TYPE improvement CHECK all_lines     RESPONSE warning)
#  Feedback_ConfigureWorkflow (maintainer        TYPE suggestion  CHECK changed_files RESPONSE warning)

#  Feedback_ConfigureWorkflow (component_builder TYPE requirement CHECK all_lines     RESPONSE error)
#  Feedback_ConfigureWorkflow (component_builder TYPE guideline   CHECK all_lines     RESPONSE warning)
#  Feedback_ConfigureWorkflow (component_builder TYPE improvement CHECK changed_files RESPONSE warning)
#  Feedback_ConfigureWorkflow (component_builder TYPE suggestion  CHECK changed_files RESPONSE warning)

#  Feedback_ConfigureWorkflow (developer         TYPE requirement CHECK all_lines     RESPONSE error)
#  Feedback_ConfigureWorkflow (developer         TYPE guideline   CHECK all_lines     RESPONSE warning)
#  Feedback_ConfigureWorkflow (developer         TYPE improvement CHECK changed_files RESPONSE warning)
#  Feedback_ConfigureWorkflow (developer         TYPE suggestion  CHECK changed_lines RESPONSE warning)
   
#  Feedback_ConfigureWorkflow (solution_provider TYPE requirement CHECK changed_files RESPONSE error)
#  Feedback_ConfigureWorkflow (solution_provider TYPE guideline   CHECK changed_lines RESPONSE warning)
#  Feedback_ConfigureWorkflow (solution_provider TYPE improvement CHECK no_lines      RESPONSE warning)
#  Feedback_ConfigureWorkflow (solution_provider TYPE suggestion  CHECK no_lines      RESPONSE warning)

#  Feedback_ConfigureWorkflow (disabled          TYPE requirement CHECK no_lines      RESPONSE error)
#  Feedback_ConfigureWorkflow (disabled          TYPE guideline   CHECK no_lines      RESPONSE warning)
#  Feedback_ConfigureWorkflow (disabled          TYPE improvement CHECK no_lines      RESPONSE warning)
#  Feedback_ConfigureWorkflow (disabled          TYPE suggestion  CHECK no_lines      RESPONSE warning)

_Feedback_Configure ()
