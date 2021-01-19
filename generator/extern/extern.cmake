include(FetchContent)

function (Extern_FindTargets targets_variable)
  cmake_parse_arguments (parameter "" "IMPORTED" "DIRECTORIES;TYPES" ${ARGN})

  if (DEFINED parameter_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${parameter_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED parameter_DIRECTORIES)
    message (FATAL_ERROR "No directories given.")
  endif ()

  if (NOT DEFINED parameter_IMPORTED)
    set (parameter_IMPORTED FALSE)
  endif ()

  if (NOT DEFINED parameter_TYPES)
    set (parameter_TYPES STATIC_LIBRARY MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE)
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
      Extern_FindTargets (subdirectory_targets DIRECTORIES ${subdirectories} TYPES ${parameter_TYPES})
      list (APPEND targets ${subdirectory_targets})
    endif ()
  endforeach ()

  set ("${targets_variable}" ${targets} PARENT_SCOPE)
endfunction ()

function (Extern_MakeAvailable package)
  cmake_parse_arguments(parameter "" "SOURCE_DIR_VARIABLE" "" ${ARGN})

  FetchContent_Declare("${package}" ${parameter_UNPARSED_ARGUMENTS})
  FetchContent_MakeAvailable("${package}")

  Extern_FindTargets (extern_targets DIRECTORIES "${${package}_SOURCE_DIR}")
  set_target_properties (${extern_targets} PROPERTIES FOLDER "extern/${package}")

  if (DEFINED parameter_SOURCE_DIR_VARIABLE)
    set ("${parameter_SOURCE_DIR_VARIABLE}" "${${package}_SOURCE_DIR}" PARENT_SCOPE)
  endif ()
endfunction ()
