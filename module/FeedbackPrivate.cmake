cmake_policy (VERSION 3.15)

if (TARGET modules_loaded)
  target_sources (modules_loaded INTERFACE "${CMAKE_CURRENT_LIST_DIR}/FeedbackPrivate.cmake")
endif ()

function (_Feedback_RelevantTargets relevant_targets_variable feedback_target)
  foreach (target IN LISTS ARGN)
    get_target_property (excluded_from_feedback "${target}" EXCLUDED_FROM_FEEDBACK)

    if (excluded_from_feedback)
      list (JOIN excluded_from_feedback "|" excluded_from_feedback_regex)

      if ("${feedback_target}" MATCHES "${excluded_from_feedback_regex}")
        continue ()
      endif ()
    endif ()

    list (APPEND relevant_targets "${target}")
  endforeach ()

  set ("${relevant_targets_variable}" ${relevant_targets} PARENT_SCOPE)
endfunction ()

function (_Feedback_SourceDir source_dir_variable)
  set (${source_dir_variable} "${CMAKE_BINARY_DIR}/feedback" PARENT_SCOPE)
endfunction ()

function (_Feedback_LinkLibraries libraries_variable target)
  unset (libraries)

  get_target_property (link_libraries "${target}" LINK_LIBRARIES)

  if (link_libraries)
    foreach (library IN LISTS link_libraries)
      if (NOT TARGET "${library}")
        continue ()
      endif ()

      get_target_property (interface_link_libraries "${library}" INTERFACE_LINK_LIBRARIES)

      foreach (interface_library IN LISTS interface_link_libraries)
        if (NOT TARGET "${interface_library}")
          continue ()
        endif ()

        list (APPEND libraries "${interface_library}")
      endforeach ()

      list (APPEND libraries "${library}")
    endforeach ()
  endif ()
 
  set ("${libraries_variable}" ${libraries} PARENT_SCOPE)
endfunction ()

function (_Feedback_InterfaceSources all_sources_variable)
  unset (all_sources)

  foreach (target IN LISTS ARGN)
    _Feedback_LinkLibraries (link_libraries "${target}")

    foreach (library IN LISTS link_libraries)
      get_target_property (interface_sources "${library}" INTERFACE_SOURCES)

      if (NOT interface_sources)
        continue ()
      endif ()

      unset (library_sources)

      foreach (source IN LISTS interface_sources)

        # skip generator expressions
        if (source MATCHES "^[$][<].+[>]$")
          continue ()
        endif ()

        # skip generated files
        unset (directory_scope)
        if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
          set (directory_scope TARGET_DIRECTORY "${target}")
        endif ()

        get_source_file_property (generated "${source}" ${directory_scope} GENERATED)

        if (generated)
          continue ()
        endif ()

        list (APPEND library_sources "${source}")
      endforeach ()

      list (APPEND all_sources ${library_sources})
    endforeach ()
  endforeach ()

  set ("${all_sources_variable}" ${all_sources} PARENT_SCOPE)
endfunction ()

function (_Feedback_RelevantSourcesFromTargets all_sources_variable)
  _Feedback_InterfaceSources (all_sources ${ARGN})

  foreach (target IN LISTS ARGN)
    get_target_property (sources "${target}" SOURCES)

    if (NOT sources)
      continue ()
    endif ()

    get_target_property (source_dir "${target}" SOURCE_DIR)

    unset (target_sources)

    foreach (source IN LISTS sources)

      # skip generator expressions
      if (source MATCHES "^[$][<].+[>]$")
        continue ()
      endif ()

      # to absolute path
      get_filename_component (source "${source}" ABSOLUTE BASE_DIR "${source_dir}")

      # skip generated files
      unset (directory_scope)
      if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
        set (directory_scope TARGET_DIRECTORY "${target}")
      endif ()

      get_source_file_property (generated "${source}" ${directory_scope} GENERATED)

      if (generated)
        continue ()
      endif ()

      list (APPEND target_sources "${source}")
    endforeach ()

    if (EXISTS "${source_dir}/CMakeLists.txt")
      list (APPEND target_sources "${source_dir}/CMakeLists.txt")
      list (REMOVE_DUPLICATES target_sources)
    endif ()

    list (APPEND all_sources ${target_sources})
  endforeach ()

  set ("${all_sources_variable}" ${all_sources} PARENT_SCOPE)
endfunction ()

function (_Feedback_WriteFileIfDifferent filename content)
  if (EXISTS "${filename}")
    file (READ "${filename}" old_content)

    if (content STREQUAL old_content)
      return ()
    endif ()
  endif ()

  file (WRITE "${filename}" "${content}")
endfunction ()

function (_Feedback_WriteFileList filename)
  string (REGEX REPLACE ";" "\n" file_list "${ARGN};")
  _Feedback_WriteFileIfDifferent ("${filename}" "${file_list}")
endfunction ()

function (_Feedback_Worktree worktree_variable)
  cmake_parse_arguments (parameter "" "HINT" "" ${ARGN})

  if (DEFINED parameter_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${parameter_UNPARSED_ARGUMENTS}")
  endif ()

  set (worktree "${CMAKE_SOURCE_DIR}")

  if (DEFINED parameter_HINT)
    set (worktree "${parameter_HINT}")
  endif ()

  while (NOT EXISTS "${worktree}/.git")
    if (NOT IS_DIRECTORY "${worktree}")
      message (FATAL_ERROR "Worktree not found")
    endif ()

    get_filename_component(worktree "${worktree}" DIRECTORY)
  endwhile ()

  set (${worktree_variable} "${worktree}" PARENT_SCOPE)
endfunction ()

function (_Feedback_Repository repository_variable)
  _Feedback_Worktree (worktree ${ARGN})

  if (NOT IS_DIRECTORY "${worktree}/.git")
    file (READ "${worktree}/.git" gitdir)
    string (REGEX REPLACE ".*gitdir[:] +(.*)\n.*" "\\1" gitdir "${gitdir}")
    _Feedback_Worktree (worktree HINT "${gitdir}")
  endif ()

  set (${repository_variable} "${worktree}" PARENT_SCOPE)
endfunction ()

function (ConfigureFeedbackTargetFromTargets name rules workflow changes)

  _Feedback_RelevantTargets (relevant_targets "${name}" ${ARGN})

  if (NOT relevant_targets)
    return ()
  endif ()

  find_package(Git REQUIRED)

  _Feedback_SourceDir (feedback_source_dir)

  _Feedback_Worktree (worktree)
  _Feedback_Repository (repository HINT "${worktree}")

  _Feedback_RelevantSourcesFromTargets (relevant_sources ${relevant_targets})

  string (MAKE_C_IDENTIFIER "${name}-diff" feedback_target_diff)
  string (TOLOWER "${feedback_target_diff}" feedback_target_diff)

  set (WORKING_DIRECTORY "${worktree}")
  set (UNTRACKED_FILES_DIFF "${feedback_source_dir}/${feedback_target_diff}/untracked.diff")

  configure_file ("${feedback_main_SOURCE_DIR}/module/diff-untracked-files.cmake" "${feedback_source_dir}/${feedback_target_diff}/diff-untracked-files.cmake" @ONLY)

  add_library ("${feedback_target_diff}" STATIC EXCLUDE_FROM_ALL)

  add_custom_command (
    OUTPUT "${UNTRACKED_FILES_DIFF}"
    COMMAND "${CMAKE_COMMAND}" "-P" "${feedback_source_dir}/${feedback_target_diff}/diff-untracked-files.cmake"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    DEPENDS "${repository}/.git/index" ${relevant_sources}
    )

  add_custom_command (
    OUTPUT "${feedback_source_dir}/${feedback_target_diff}/all.diff"
    COMMAND "${CMAKE_COMMAND}" "-E" "copy" "${UNTRACKED_FILES_DIFF}" "${feedback_source_dir}/${feedback_target_diff}/all.diff"
    COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "@{push}" >> "${feedback_source_dir}/${feedback_target_diff}/all.diff"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    DEPENDS Git::Git "${repository}/.git/FETCH_HEAD" "${UNTRACKED_FILES_DIFF}"
    )

  add_custom_command (
    OUTPUT "${feedback_source_dir}/${feedback_target_diff}/modified.diff"
    COMMAND "${CMAKE_COMMAND}" "-E" "copy" "${UNTRACKED_FILES_DIFF}" "${feedback_source_dir}/${feedback_target_diff}/modified.diff"
    COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" >> "${feedback_source_dir}/${feedback_target_diff}/modified.diff"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    DEPENDS Git::Git "${repository}/.git/index" "${UNTRACKED_FILES_DIFF}"
    )

  add_custom_command (
    OUTPUT "${feedback_source_dir}/${feedback_target_diff}/modified_or_staged.diff"
    COMMAND "${CMAKE_COMMAND}" "-E" "copy" "${UNTRACKED_FILES_DIFF}" "${feedback_source_dir}/${feedback_target_diff}/modified_or_staged.diff"
    COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "@" >> "${feedback_source_dir}/${feedback_target_diff}/modified_or_staged.diff"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    DEPENDS Git::Git "${repository}/.git/logs/HEAD" "${repository}/.git/HEAD" "${UNTRACKED_FILES_DIFF}"
    )

  add_custom_command (
    OUTPUT "${feedback_source_dir}/${feedback_target_diff}/staged.diff"
    COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "--staged" "@" > "${feedback_source_dir}/${feedback_target_diff}/staged.diff"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    DEPENDS Git::Git "${repository}/.git/logs/HEAD" "${repository}/.git/HEAD" "${repository}/.git/index"
    )

  add_custom_command (
    OUTPUT "${feedback_source_dir}/${feedback_target_diff}/staged_or_committed.diff"
    COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "--staged" "@{push}" > "${feedback_source_dir}/${feedback_target_diff}/staged_or_committed.diff"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    DEPENDS Git::Git "${repository}/.git/FETCH_HEAD" "${repository}/.git/index"
    )

  add_custom_command (
    OUTPUT "${feedback_source_dir}/${feedback_target_diff}/committed.diff"
    COMMAND "$<TARGET_FILE:Git::Git>" "log" "--unified=0" "--branches" "--not" "--remotes" "--format=format:" > "${feedback_source_dir}/${feedback_target_diff}/committed.diff"
    WORKING_DIRECTORY "${WORKING_DIRECTORY}"
    DEPENDS Git::Git "${repository}/.git/logs/HEAD" "${repository}/.git/HEAD" "${repository}/.git/FETCH_HEAD"
    )

  target_sources ("${feedback_target_diff}" PRIVATE "${UNTRACKED_FILES_DIFF}")
  target_sources ("${feedback_target_diff}" PRIVATE "${feedback_source_dir}/${feedback_target_diff}/${changes}.diff")
  set_target_properties ("${feedback_target_diff}" PROPERTIES LINKER_LANGUAGE "CXX" FOLDER "feedback" EXCLUDED_FROM_FEEDBACK "(^.*$)")

  string (MAKE_C_IDENTIFIER "${name}" feedback_target_library)
  string (TOLOWER "${feedback_target_library}" feedback_target_library)

  add_library ("${feedback_target_library}" STATIC EXCLUDE_FROM_ALL)
  add_dependencies ("${feedback_target_library}" "${feedback_target_diff}")

  target_sources ("${feedback_target_library}" PRIVATE "${rules}" "${workflow}")

  foreach (target IN LISTS relevant_targets)
    _Feedback_RelevantSourcesFromTargets (relevant_sources "${target}")
    _Feedback_WriteFileList ("${feedback_source_dir}/${feedback_target_library}/${target}.sources.txt" ${relevant_sources})

    add_custom_command (
      OUTPUT "${feedback_source_dir}/${feedback_target_library}/${target}.cpp"
      COMMAND "$<TARGET_FILE:feedback-generator>" "--workflow=${workflow}" "--diff=${feedback_source_dir}/${feedback_target_diff}/${changes}.diff" "${rules}" "${feedback_source_dir}/${feedback_target_library}/${target}.sources.txt" ">" "${feedback_source_dir}/${feedback_target_library}/${target}.cpp"
      DEPENDS feedback-generator "${rules}" "${workflow}" "${feedback_source_dir}/${feedback_target_library}/${target}.sources.txt" ${relevant_sources} # sic! no dependency to diff. really?
      )
    target_sources ("${feedback_target_library}" PRIVATE "${feedback_source_dir}/${feedback_target_library}/${target}.cpp")

    # FIXME: should we link against feedback_target_library?
    add_dependencies ("${target}" "${feedback_target_library}")
  endforeach()

  target_compile_options("${feedback_target_library}" PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wno-error>)
  target_compile_options("${feedback_target_library}" PRIVATE $<$<CXX_COMPILER_ID:MSVC>:-WX->)

  set_target_properties ("${feedback_target_library}" PROPERTIES FOLDER "feedback" EXCLUDED_FROM_FEEDBACK "(^.*$)")
endfunction ()

function (_Feedback_Configure)
  find_package (Git REQUIRED)

  define_property (TARGET PROPERTY EXCLUDED_FROM_FEEDBACK
                   BRIEF_DOCS "exclude this target from feedback targets"
                   FULL_DOCS "exclude this target from feedback targets")

  define_property (GLOBAL PROPERTY FEEDBACK_DEFAULT_WORKFLOW
                   BRIEF_DOCS "default workflow for feedback"
                   FULL_DOCS "default workflow for feedback")

  define_property (GLOBAL PROPERTY FEEDBACK_DEFAULT_RELEVANT_CHANGES
                   BRIEF_DOCS "default relevant changes for feedback"
                   FULL_DOCS "default relevant changes for feedback")

set_property(GLOBAL PROPERTY FEEDBACK_DEFAULT_WORKFLOW "${feedback_main_SOURCE_DIR}/module/default_workflow.json")
set_property(GLOBAL PROPERTY FEEDBACK_DEFAULT_RELEVANT_CHANGES "modified_or_staged")

  # adding the generator as an external project is preferable because we
  #  * build the generator always in release configuration
  #  * do not inherit compile options from the client project
  #  * achieve a faster CMake configuration/generation of the client project
  #  * can use the feedback system for our project itself
  # even though we
  #  * have to define imported targets manually
  #  * will have a permanent build check

  # FIXME: use feedback_module target instead of FEEDBACK_MAIN_PROJECT all_sources_variable?

  include (ExternalProject)
  ExternalProject_Add(generator-build
    SOURCE_DIR "${feedback_main_SOURCE_DIR}/generator"
    BINARY_DIR "${feedback_main_BINARY_DIR}/generator-build"
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release" "-DBUILD_TESTING=OFF"
    BUILD_COMMAND "${CMAKE_COMMAND}" "--build" "${feedback_main_BINARY_DIR}/generator-build" "--config" "Release"
    BUILD_ALWAYS "${FEEEBACK_MAIN_PROJECT}"
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL ON)
  set_target_properties (generator-build PROPERTIES FOLDER "feedback" EXCLUDED_FROM_FEEDBACK "(^.*$)")

  add_executable (feedback-generator IMPORTED GLOBAL)
  add_dependencies(feedback-generator generator-build)

  set (cfg_intdir "${CMAKE_CFG_INTDIR}")

  if (NOT cfg_intdir STREQUAL ".")
    set (cfg_intdir "Release")
  endif ()

  ExternalProject_Get_Property(generator-build binary_dir)
  set_target_properties(feedback-generator PROPERTIES IMPORTED_LOCATION "${binary_dir}/${cfg_intdir}/generator${CMAKE_EXECUTABLE_SUFFIX}")
endfunction ()

