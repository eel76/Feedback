cmake_policy (VERSION 3.15)

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
  cmake_parse_arguments(parameter "" "GIT_REPOSITORY;GIT_TAG;GIT_SHALLOW;SOURCE_DIR_VARIABLE" "" ${ARGN})

  # CMake's GIT_SHALLOW is not shallow enough (https://gitlab.kitware.com/cmake/cmake/-/issues/17770) ... so we do it ourselves
  if (parameter_GIT_REPOSITORY AND parameter_GIT_TAG AND parameter_GIT_SHALLOW)
    string (MAKE_C_IDENTIFIER "${parameter_GIT_REPOSITORY}-${parameter_GIT_TAG}-src" package_src)
    
    if (NOT EXISTS "${FETCHCONTENT_BASE_DIR}/${package_src}")
      execute_process (COMMAND "${GIT_EXECUTABLE}" clone --depth 1 --single-branch "--branch=${parameter_GIT_TAG}" "${parameter_GIT_REPOSITORY}" "${FETCHCONTENT_BASE_DIR}/${package_src}" ERROR_QUIET)
      message (STATUS "Cloning into '${FETCHCONTENT_BASE_DIR}/${package_src}'...")
    endif ()

    FetchContent_Declare("${package}" SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/${package_src}" ${parameter_UNPARSED_ARGUMENTS})
  else ()
    unset (passthrough_parameters)

    foreach (option GIT_REPOSITORY GIT_TAG GIT_SHALLOW)
      if (DEFINED parameter_${option})
        list (APPEND passthrough_parameters "${option}" "${parameter_${option}}")
      endif ()
    endforeach ()

    FetchContent_Declare("${package}" ${passthrough_parameters} ${parameter_UNPARSED_ARGUMENTS})
  endif ()

  FetchContent_MakeAvailable("${package}")

  Extern_FindTargets (extern_targets DIRECTORIES "${${package}_SOURCE_DIR}")
  set_target_properties (${extern_targets} PROPERTIES FOLDER "extern/${package}")

  if (DEFINED parameter_SOURCE_DIR_VARIABLE)
    set ("${parameter_SOURCE_DIR_VARIABLE}" "${${package}_SOURCE_DIR}" PARENT_SCOPE)
  endif ()
endfunction ()
