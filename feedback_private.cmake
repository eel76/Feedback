cmake_policy (VERSION 3.7.0)

define_property (TARGET PROPERTY EXCLUDE_FROM_FEEDBACK
                 BRIEF_DOCS "exclude target from any feedback"
                 FULL_DOCS "exclude target from any feedback")

function (Feedback_Exclude)
  set_target_properties (${ARGN} PROPERTIES EXCLUDE_FROM_FEEDBACK true)
endfunction ()

function (Feedback_IsExcluded is_excluded_variable target)
  get_target_property (exclude_from_feedback "${target}" EXCLUDE_FROM_FEEDBACK)
  set (${is_excluded_variable} "${exclude_from_feedback}" PARENT_SCOPE)
endfunction ()

function (Feedback_InternalTargets)
  Feedback_Exclude (${ARGN})
  GroupTargetsInFolder (feedback ${ARGN})
endfunction ()

function (GetFeedbackSourceDir source_dir_variable)
  set (${source_dir_variable} "${CMAKE_BINARY_DIR}/FeedbackSourceDir" PARENT_SCOPE)
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

function (Feedback_RemoveExcludedTargets included_targets_variable)
  foreach (target IN LISTS ARGN)
    Feedback_IsExcluded (is_excluded "${target}")
    if (NOT is_excluded)
      list (APPEND included_targets "${target}")
    endif ()
  endforeach ()

  set (${included_targets_variable} "${included_targets}" PARENT_SCOPE)
endfunction ()

#function (ConfigureFeedbackOptions)
#  # set (FEEDBACK_FILES_TO_CHECK ADDED_OR_MODIFIED_FILES CACHE STRING "files to check")
#  set (FEEDBACK_FILES_TO_CHECK ALL_VERSIONED_FILES CACHE STRING "files to check")
#  mark_as_advanced (FEEDBACK_FILES_TO_CHECK)
#  set_property (CACHE FEEDBACK_FILES_TO_CHECK PROPERTY STRINGS ALL_VERSIONED_FILES ADDED_OR_MODIFIED_FILES CUSTOM_FILE_LIST)

#  set (FEEDBACK_FILE_LIST "" CACHE FILEPATH "file list")
#  mark_as_advanced (FEEDBACK_FILE_LIST)
#endfunction ()

#function (GetAddedOrModifiedFileListPath added_or_modified_file_list_variable)
#  if (FEEDBACK_FILES_TO_CHECK STREQUAL "CUSTOM_FILE_LIST")
#    set (${added_or_modified_file_list_variable} "${FEEDBACK_FILE_LIST}" PARENT_SCOPE)
#    return ()
#  endif ()

#  GetFeedbackSourceDir (source_dir)
#  set (${added_or_modified_file_list_variable} "${source_dir}/added_or_modified_file_list.txt" PARENT_SCOPE)
#endfunction ()

#function (RelevantSourcesFromTargetList relevant_sources_variable)
#  foreach (target IN LISTS ARGN)
#    RelevantSourcesFromTarget (sources "${target}")
#    list (APPEND relevant_sources ${sources})
#  endforeach ()

#  set (${relevant_sources_variable} ${relevant_sources} PARENT_SCOPE)
#endfunction ()

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

function (GroupTargetsInFolder folder)
  foreach (target IN LISTS ARGN)
    get_target_property(current_folder ${target} FOLDER)
    if (current_folder)
      set_target_properties (${target} PROPERTIES FOLDER "${folder}/${current_folder}")
    else ()
      set_target_properties (${target} PROPERTIES FOLDER "${folder}")
    endif ()
  endforeach()
endfunction ()

function (ConfigureFeedbackTargetFromTargets type feedback_target json_filename)
  Feedback_RemoveExcludedTargets (ARGN ${ARGN})
  CreateFeedbackSourcesFromTargets (feedback_sources "${type}" "${feedback_target}" "${json_filename}" ${ARGN})

  add_library ("${feedback_target}" SHARED EXCLUDE_FROM_ALL "${json_filename}" ${feedback_sources})
  target_compile_options(${feedback_target} PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wno-error>)
  target_compile_options(${feedback_target} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:-WX->)

  Feedback_Exclude ("${feedback_target}")
  GroupTargetsInFolder (feedback "${feedback_target}")
  
  foreach (target IN LISTS ARGN)
    add_dependencies ("${target}" "${feedback_target}")
  endforeach()
endfunction ()

#function (ConfigureFeedbackForTargets json_filename)
  #GetFeedbackSourceDir (source_dir)
  #set (cmake_lists_file "${source_dir}/CMakeLists.txt")
  
  #if (NOT CMAKE_CURRENT_LIST_FILE STREQUAL cmake_lists_file)
#  get_filename_component (json_filename "${json_filename}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
  #  file (WRITE "${cmake_lists_file}" "ConfigureFeedbackForTargets (\"${json_filename}\" ${ARGN})")
  #  add_subdirectory ("${source_dir}" "${source_dir}-Build")
  
  #  return ()
  #endif ()

#  message (STATUS "Configuring feedback")

#  ConfigureFeedbackOptions ()
#  ConfigureFeedbackTargetFromTargets (ALL_FEEDBACK "${json_filename}" ${ARGN})

#  if (FEEDBACK_FILES_TO_CHECK STREQUAL "ADDED_OR_MODIFIED_FILES")
#    GetRepositoryPath (repository)
#    GetAddedOrModifiedFileListPath (added_or_modified_file_list)
#    GetFeedbackSourceDir (source_dir)
#    GetFeedbackScript (script)
#    RelevantSourcesFromTargetList(all_relevant_sources ${ARGN})

#    add_custom_command (
#      OUTPUT "${source_dir}/added_or_modified.cpp"
#      COMMAND "${CMAKE_COMMAND}" -D "SCRIPT_FUNCTION=WriteAddedOrModifiedFileList" -D "PARAMETER_REPOSITORY=${repository}" -D "PARAMETER_OUTPUT=${added_or_modified_file_list}" -P "${script}"
#      COMMAND "${CMAKE_COMMAND}" -E copy "${source_dir}/dummy.cpp" "${source_dir}/added_or_modified.cpp"
#      DEPENDS "${script}" ${all_relevant_sources}
#      BYPRODUCTS "${added_or_modified_file_list}"
#      )

#    add_library (ADDED_OR_MODIFIED_FILE_LIST MODULE EXCLUDE_FROM_ALL "${source_dir}/added_or_modified.cpp")
#    ExcludeFromFeedback (ADDED_OR_MODIFIED_FILE_LIST)
#    GroupInFeedbackFolder (ADDED_OR_MODIFIED_FILE_LIST)

#    add_dependencies(ALL_FEEDBACK ADDED_OR_MODIFIED_FILE_LIST)
#  endif ()

#  message (STATUS "Configuration done")
#endfunction ()

function (CreateFeedbackSourceForSources type filename json_filename)
  WriteFileList ("${filename}.sources.txt" ${ARGN})

#  if (FEEDBACK_FILES_TO_CHECK STREQUAL "ALL_VERSIONED_FILES")
    add_custom_command (
      OUTPUT "${filename}"
      COMMAND "$<TARGET_FILE:generator>" "-o=${filename}" "${json_filename}" "${filename}.sources.txt"
      DEPENDS generator "${json_filename}" ${ARGN}
      )
#  elseif (FEEDBACK_FILES_TO_CHECK STREQUAL "ADDED_OR_MODIFIED_FILES")
#    GetAddedOrModifiedFileListPath (added_or_modified_file_list)
#    add_custom_command (
#      OUTPUT "${filename}"
#      COMMAND "$<TARGET_FILE:generator>" "-f=${added_or_modified_file_list}" "-o=${filename}" "${json_filename}" "${filename}.sources.txt"
#      DEPENDS generator "${added_or_modified_file_list}" "${json_filename}" ${ARGN}
#      )
#  endif ()
endfunction ()

function (CreateFeedbackSourcesForTarget feedback_sources_variable type feedback_target json_filename target)
  RelevantSourcesFromTarget (relevant_sources "${target}")
  GetFeedbackSourceDir (source_dir)

  set (index 0)
  while (relevant_sources)
    math (EXPR index "${index} + 1")
    set (feedback_source "${source_dir}/${feedback_target}_${target}_${index}.cpp")
    list (APPEND feedback_sources "${feedback_source}")

    RemoveFirstElementsFromList (sources 128 relevant_sources)
    CreateFeedbackSourceForSources ("${type}" "${feedback_source}" "${json_filename}" ${sources})
  endwhile ()

  set (${feedback_sources_variable} ${feedback_sources} PARENT_SCOPE)
endfunction ()

function (CreateFeedbackSourcesFromTargets all_sources_variable type feedback_target json_filename)
  foreach (target IN LISTS ARGN)
    CreateFeedbackSourcesForTarget (sources "${type}" "${feedback_target}" "${json_filename}" "${target}")
    list (APPEND all_sources ${sources})
  endforeach()

  set (${all_sources_variable} ${all_sources} PARENT_SCOPE)
endfunction ()
