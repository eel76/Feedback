cmake_policy (VERSION 3.15)

define_property (TARGET PROPERTY EXCLUDE_FROM_FEEDBACK
                 BRIEF_DOCS "exclude target from any feedback"
                 FULL_DOCS "exclude target from any feedback")

function (Feedback_ExcludeTargets)
  set_target_properties (${ARGN} PROPERTIES EXCLUDED_FROM_FEEDBACK true)
endfunction ()

function (Feedback_RelevantTargets relevant_targets_variable)
  foreach (target IN LISTS ARGN)
    get_target_property (excluded_from_feedback "${target}" EXCLUDED_FROM_FEEDBACK)

    if (NOT excluded_from_feedback)
      list (APPEND relevant_targets "${target}")
    endif ()
  endforeach ()

  set ("${relevant_targets_variable}" ${relevant_targets} PARENT_SCOPE)
endfunction ()

function (Feedback_MarkInternalTargets)
  Feedback_GroupTargetsInFolder (${ARGN} FOLDER "feedback")
  Feedback_ExcludeTargets (${ARGN})
endfunction ()

function (GetFeedbackSourceDir source_dir_variable)
  set (${source_dir_variable} "${CMAKE_BINARY_DIR}/feedback" PARENT_SCOPE)
endfunction ()

function (ToAbsolutePath paths_variable base_dir)
  foreach (path IN LISTS ${paths_variable})
    get_filename_component (path "${path}" ABSOLUTE BASE_DIR "${base_dir}")
    list (APPEND paths "${path}")
  endforeach ()

  set (${paths_variable} ${paths} PARENT_SCOPE)
endfunction ()

function (RemoveGeneratorExpressions files_variable)
  foreach (file IN LISTS ${files_variable})
    if (NOT file MATCHES "^[$][<].+[>]$")
      list (APPEND files "${file}")
    endif ()
  endforeach ()

  set (${files_variable} ${files} PARENT_SCOPE)
endfunction ()

function (RemoveFilesFromDirectory files_variable directory)
  foreach (file IN LISTS ${files_variable})
    string (FIND "${file}" "${directory}" pos)
    if (pos EQUAL -1)
      list (APPEND files "${file}")
    endif ()
  endforeach ()

  set (${files_variable} ${files} PARENT_SCOPE)
endfunction ()

function (RemoveMissingFiles files_variable)
  foreach (file IN LISTS ${files_variable})
    if (EXISTS "${file}")
      list (APPEND files "${file}")
    endif ()
  endforeach ()

  set (${files_variable} ${files} PARENT_SCOPE)
endfunction ()

function (RelevantSourcesFromTarget sources_variable target)
  get_target_property (sources ${target} SOURCES)
  get_target_property (source_dir ${target} SOURCE_DIR)

  RemoveGeneratorExpressions (sources)
  ToAbsolutePath (sources "${source_dir}")
  RemoveFilesFromDirectory (sources "${CMAKE_BINARY_DIR}")

  list (APPEND sources "${source_dir}/CMakeLists.txt")
  RemoveMissingFiles (sources)
  
  set (${sources_variable} ${sources} PARENT_SCOPE)
endfunction ()

function (WriteFileIfDifferent filename content)
  if (EXISTS "${filename}")
    file (READ "${filename}" old_content)

    if (content STREQUAL old_content)
      return ()
    endif ()
  endif ()

  file (WRITE "${filename}" "${content}")
endfunction ()

function (WriteFileList filename)
  string (REGEX REPLACE ";" "\n" file_list "${ARGN};")
  WriteFileIfDifferent ("${filename}" "${file_list}")
endfunction ()

function (RemoveFirstElementsFromList chunk_variable count list_variable)
  set (list ${${list_variable}})

  foreach (iteration RANGE 1 ${count})
    if (list)
      list (GET list 0 value)
      list (REMOVE_AT list 0)
      list (APPEND chunk "${value}")
    endif ()
  endforeach ()
  
  set (${chunk_variable} ${chunk} PARENT_SCOPE)
  set (${list_variable} ${list} PARENT_SCOPE)
endfunction ()

function (Feedback_GroupTargetsInFolder)
  cmake_parse_arguments (parameter "" "FOLDER" "" ${ARGN})

  if (NOT DEFINED parameter_FOLDER)
    message (FATAL_ERROR "No folder given.")
  endif ()

  foreach (target IN LISTS parameter_UNPARSED_ARGUMENTS)
    get_target_property(current_folder ${target} FOLDER)

    if (current_folder)
      set_target_properties (${target} PROPERTIES FOLDER "${parameter_FOLDER}/${current_folder}")
    else ()
      set_target_properties (${target} PROPERTIES FOLDER "${parameter_FOLDER}")
    endif ()
  endforeach()
endfunction ()

function (GetWorktree worktree_variable)
  cmake_parse_arguments (Worktree "" "HINT" "" ${ARGN})

  if (DEFINED Worktree_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR "Unparsed arguments: ${Worktree_UNPARSED_ARGUMENTS}")
  endif ()

  set (worktree "${CMAKE_SOURCE_DIR}")

  if (DEFINED Worktree_HINT)
    set (worktree "${Worktree_HINT}")
  endif ()

  while (NOT EXISTS "${worktree}/.git")
    if (NOT IS_DIRECTORY "${worktree}")
      message (FATAL_ERROR "Worktree not found")
    endif ()

    get_filename_component(worktree "${worktree}" DIRECTORY)
  endwhile ()

  set (${worktree_variable} "${worktree}" PARENT_SCOPE)
endfunction ()

function (GetRepository repository_variable)
  GetWorktree (worktree ${ARGN})

  if (NOT IS_DIRECTORY "${worktree}/.git")
    file (READ "${worktree}/.git" gitdir)
    string (REGEX REPLACE ".*gitdir[:] +(.*)\n.*" "\\1" gitdir "${gitdir}")
    GetWorktree (worktree HINT "${gitdir}")
  endif ()

  set (${repository_variable} "${worktree}" PARENT_SCOPE)
endfunction ()

function (ConfigureFeedbackTargetFromTargets feedback_target rules workflow changes)

  Feedback_RelevantTargets (relevant_targets ${ARGN})

  if (NOT relevant_targets)
    return ()
  endif ()

  GetFeedbackSourceDir (feedback_source_dir)

  if (NOT TARGET DIFF)

    find_package(Git REQUIRED)

    add_library(DIFF STATIC EXCLUDE_FROM_ALL)

    # FIXME: read HEAD file and parse ref, depend on that ref, too!
#    file (GLOB_RECURSE refs CONFIGURE_DEPENDS "${repository}/.git/refs/*")

#    add_custom_command (
#      OUTPUT "${feedback_source_dir}/all.diff"
#      COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "@{push}" ">" "${feedback_source_dir}/all.diff"
#      WORKING_DIRECTORY "${worktree}"
#      DEPENDS Git::Git ${refs} "${repository}/.git/HEAD" "${repository}/.git/index"
#      )

#    add_custom_command (
#      OUTPUT "${feedback_source_dir}/modified.diff"
#      COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" ">" "${feedback_source_dir}/modified.diff"
#      WORKING_DIRECTORY "${worktree}"
#      DEPENDS Git::Git ${refs} "${repository}/.git/HEAD" "${repository}/.git/index"
#      )

    GetWorktree (worktree)
    GetRepository (repository HINT "${worktree}")

    add_custom_command (
      OUTPUT "${feedback_source_dir}/DIFF/${changes}.cpp"
      COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "@" ">" "${feedback_source_dir}/DIFF/${changes}.tmp"
      COMMAND "${CMAKE_COMMAND}" "-E" "copy_if_different" "${feedback_source_dir}/DIFF/${changes}.tmp" "${feedback_source_dir}/DIFF/${changes}.diff"
      COMMAND "${CMAKE_COMMAND}" "-E" "echo" "namespace { using none = int; }" ">" "${feedback_source_dir}/DIFF/${changes}.cpp"
      WORKING_DIRECTORY "${worktree}"
      DEPENDS Git::Git "${repository}/.git/HEAD" "${repository}/.git/index"
      BYPRODUCTS "${feedback_source_dir}/DIFF/${changes}.diff"
      )
    target_sources (DIFF PRIVATE "${feedback_source_dir}/DIFF/${changes}.cpp" "${feedback_source_dir}/DIFF/${changes}.diff")

#    add_custom_command (
#      OUTPUT "${feedback_source_dir}/staged.diff"
#      COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "--staged" "@" ">" "${feedback_source_dir}/staged.diff"
#      WORKING_DIRECTORY "${worktree}"
#      DEPENDS Git::Git ${refs} "${repository}/.git/HEAD" "${repository}/.git/index"
#      )

#    add_custom_command (
#      OUTPUT "${feedback_source_dir}/staged_or_committed.diff"
#      COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "--staged" "@{push}" ">" "${feedback_source_dir}/staged_or_committed.diff"
#      WORKING_DIRECTORY "${worktree}"
#      DEPENDS Git::Git ${refs} "${repository}/.git/HEAD" "${repository}/.git/index"
#      )

#    add_custom_command (
#      OUTPUT "${feedback_source_dir}/committed.diff"
#      COMMAND "$<TARGET_FILE:Git::Git>" "log" "--unified=0" "--branches" "--not" "--remotes" "--format=format:" ">" "${feedback_source_dir}/committed.diff"
#      WORKING_DIRECTORY "${worktree}"
#      DEPENDS Git::Git ${refs} "${repository}/.git/HEAD" "${repository}/.git/index"
#      )

#    add_custom_command (
#      OUTPUT "${feedback_source_dir}/all.git.diff"
#      COMMAND "$<TARGET_FILE:Git::Git>" "diff" "--unified=0" "--output=${feedback_source_dir}/all.git.diff" "@"
#      COMMAND "${CMAKE_COMMAND}" "-E" "copy_if_different" "${feedback_source_dir}/all.git.diff" "${feedback_source_dir}/all.diff"
#      WORKING_DIRECTORY "${worktree}"
#      DEPENDS Git::Git ${refs} "${repository}/.git/HEAD" "${repository}/.git/index"
#      BYPRODUCTS "${feedback_source_dir}/all.diff"
#      )

    Feedback_MarkInternalTargets (DIFF)
  endif ()

  WriteWorkflow ("${feedback_source_dir}/${feedback_target}/workflow.json" "${workflow}")

  add_library ("${feedback_target}" STATIC EXCLUDE_FROM_ALL)
  target_sources ("${feedback_target}" PRIVATE "${rules}" "${feedback_source_dir}/${feedback_target}/workflow.json")

  foreach (target IN LISTS relevant_targets)
    CreateFeedbackSourcesForTarget ("${feedback_target}" "${rules}" "${feedback_source_dir}/${feedback_target}/workflow.json" "${changes}" "${target}")
    add_dependencies ("${target}" "${feedback_target}")
  endforeach()

  target_compile_options("${feedback_target}" PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wno-error>)
  target_compile_options("${feedback_target}" PRIVATE $<$<CXX_COMPILER_ID:MSVC>:-WX->)

  Feedback_MarkInternalTargets ("${feedback_target}")
endfunction ()

function (WriteWorkflow filename workflow)
  unset (content)
  unset (separator)

  string (APPEND content "{\n")

  foreach (type IN LISTS "FEEDBACK_WORKFLOW_${workflow}_TYPES")
    string (APPEND content "${separator}")
    set (separator ",\n")

    string (APPEND content "  \"${type}\": {\n")
    string (APPEND content "    \"check\": \"${FEEDBACK_WORKFLOW_${workflow}_CHECK_${type}}\",\n")
    string (APPEND content "    \"response\": \"${FEEDBACK_WORKFLOW_${workflow}_RESPONSE_${type}}\"\n")
    string (APPEND content "  }")
  endforeach()

  string (APPEND content "\n}")

  WriteFileIfDifferent ("${filename}" "${content}")
endfunction ()

function (CreateFeedbackSourceForSources filename rules workflow diff)
endfunction ()

function (CreateFeedbackSourcesForTarget feedback_target rules workflow diff target)
  GetFeedbackSourceDir (feedback_source_dir)
  RelevantSourcesFromTarget (relevant_sources "${target}")

  if (NOT TARGET "ALL_SOURCES_${target}")
    add_library("ALL_SOURCES_${target}" INTERFACE)

    add_custom_command (
      OUTPUT "${feedback_source_dir}/ALL_SOURCES/${target}.cpp"
      COMMAND "${CMAKE_COMMAND}" "-E" "echo" "namespace { using none = int; }" ">" "${feedback_source_dir}/ALL_SOURCES/${target}.cpp"
      DEPENDS ${relevant_sources}
      )
    target_sources ("ALL_SOURCES_${target}" INTERFACE "${feedback_source_dir}/ALL_SOURCES/${target}.cpp")
    target_link_libraries("DIFF" PRIVATE "ALL_SOURCES_${target}")
  endif ()

  if (NOT TARGET "DIFF_${target}")
    add_library("DIFF_${target}" INTERFACE)

    add_custom_command (
      OUTPUT "${feedback_source_dir}/DIFF/${target}/${diff}.diff"
      COMMAND "${CMAKE_COMMAND}" "-E" "copy_if_different" "${feedback_source_dir}/DIFF/${diff}.diff" "${feedback_source_dir}/DIFF/${target}/${diff}.diff"
      DEPENDS "DIFF" ${relevant_sources} # sic! no dependency to "${feedback_source_dir}/DIFF/${diff}.diff"
      )
    target_sources ("DIFF_${target}" INTERFACE "${feedback_source_dir}/DIFF/${target}/${diff}.diff")
  endif ()

  set (index 0)
  while (relevant_sources)
    math (EXPR index "${index} + 1")
    set (feedback_source "${feedback_source_dir}/${feedback_target}/${target}_${index}")

    RemoveFirstElementsFromList (sources 2048 relevant_sources)

    WriteFileList ("${feedback_source}.sources.txt" ${sources})

    add_custom_command (
      OUTPUT "${feedback_source}.cpp"
      COMMAND "$<TARGET_FILE:generator>" "--output=${feedback_source}.cpp" "--diff=${feedback_source_dir}/DIFF/${target}/${diff}.diff" "${rules}" "${workflow}" "${feedback_source}.sources.txt"
      DEPENDS generator "DIFF_${target}" "${feedback_source_dir}/DIFF/${target}/${diff}.diff" "${rules}" "${workflow}" "${feedback_source}.sources.txt"
      )
    target_sources ("${feedback_target}" PRIVATE "${feedback_source}.cpp")
  endwhile ()

  target_link_libraries("${feedback_target}" PRIVATE "DIFF_${target}")
endfunction ()
