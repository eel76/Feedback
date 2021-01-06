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

