include(FetchContent)

function (Extern_FindTargets targets_variable directory)
  get_directory_property(targets DIRECTORY ${directory} BUILDSYSTEM_TARGETS)
  get_directory_property(subdirectories DIRECTORY ${directory} SUBDIRECTORIES)

  foreach (subdirectory IN LISTS subdirectories)
    Extern_FindTargets (subdirectory_targets ${subdirectory})
    list (APPEND targets ${subdirectory_targets})
  endforeach ()

  set (${targets_variable} ${targets} PARENT_SCOPE)
endfunction ()

function (Extern_MakeAvailable package git_repository git_tag)
  cmake_parse_arguments(MakeAvailable "" "BUILD_TESTING_VARIABLE;SOURCE_DIR_VARIABLE" "" ${ARGN})

  if (DEFINED MakeAvailable_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${MakeAvailable_UNPARSED_ARGUMENTS}")
  endif ()

  if (DEFINED MakeAvailable_BUILD_TESTING_VARIABLE)
    set(${MakeAvailable_BUILD_TESTING_VARIABLE} OFF CACHE INTERNAL "")
  endif ()

  FetchContent_Declare(
    ${package}
    GIT_REPOSITORY ${git_repository}
    GIT_TAG ${git_tag}
    GIT_SHALLOW TRUE
    )
  FetchContent_MakeAvailable(${package})

  Extern_FindTargets (targets "${${package}_SOURCE_DIR}")

  foreach (target IN LISTS targets)
    get_target_property (type ${target} TYPE)
    if (" STATIC_LIBRARY MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE " MATCHES " ${type} ")
      set_target_properties (${target} PROPERTIES FOLDER "extern/${package}")
    endif ()
  endforeach ()

  if (DEFINED MakeAvailable_SOURCE_DIR_VARIABLE)
    set (${MakeAvailable_SOURCE_DIR_VARIABLE} ${${package}_SOURCE_DIR} PARENT_SCOPE)
  endif ()
endfunction ()
