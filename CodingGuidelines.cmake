cmake_policy (VERSION 3.7.0)

include ("${CMAKE_CURRENT_LIST_DIR}/all_targets_in_directory.cmake")

set (DISABLE_CODING_GUIDELINES TRUE CACHE BOOL "Disable coding guidelines")

function (GetCodingGuidelinesSourceDir source_dir_variable)
  set (${source_dir_variable} "${CMAKE_BINARY_DIR}/CodingGuidelines" PARENT_SCOPE)
endfunction ()

function (GetCodingGuidelinesScript script_variable)
  GetCodingGuidelinesSourceDir (source_dir)
  set (${script_variable} "${source_dir}/CodingGuidelines.cmake" PARENT_SCOPE)
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

function (GetRepositoryPath repository_variable)
  get_filename_component(repository "${CMAKE_SOURCE_DIR}" ABSOLUTE)

  while (NOT IS_DIRECTORY "${repository}/.git" AND NOT IS_DIRECTORY "${repository}/.svn")
    get_filename_component (new_repository "${repository}" DIRECTORY)
    if (new_repository STREQUAL repository)
      message (FATAL_ERROR "'${CMAKE_SOURCE_DIR}' is not under version control!")
    endif ()
    set (repository "${new_repository}")
  endwhile ()

  set (${repository_variable} "${repository}" PARENT_SCOPE)
endfunction ()

function (StatusOfGitRepositoryEntries entries_variable repository)
  execute_process (
    COMMAND "@GIT_EXECUTABLE@" status --porcelain=v1 ${ARGN}
    WORKING_DIRECTORY "${repository}"
    OUTPUT_VARIABLE status
    )

  string (REGEX REPLACE "\\\\" "/" status "${status}")
  string (REGEX REPLACE "[\r\n]+$" "" status "${status}")
  string (REGEX REPLACE "[\r\n]+" ";" status "${status}")

  foreach (entry IN LISTS status)
    if (entry MATCHES "^(A.|.A) (.+)$")
      string (REGEX REPLACE "^...(.+)$" "\\1" file "${entry}")
      get_filename_component (file "${file}" ABSOLUTE BASE_DIR "${repository}")
      list (APPEND entries_added "${file}")
    elseif (entry MATCHES "^(D.|.D) (.+)$")
      string (REGEX REPLACE "^...(.+)$" "\\1" file "${entry}")
      get_filename_component (file "${file}" ABSOLUTE BASE_DIR "${repository}")
      list (APPEND entries_deleted "${file}")
    elseif (entry MATCHES "^(M.|.M) (.+)$")
      string (REGEX REPLACE "^...(.+)$" "\\1" file "${entry}")
      get_filename_component (file "${file}" ABSOLUTE BASE_DIR "${repository}")
      list (APPEND entries_modified "${file}")
    elseif (entry MATCHES "^[?][?] (.+)$")
      string (REGEX REPLACE "^...(.+)$" "\\1" file "${entry}")
      get_filename_component (file "${file}" ABSOLUTE BASE_DIR "${repository}")
      list (APPEND entries_unversioned "${file}")
    endif ()
  endforeach ()

  set (${entries_variable}_added ${entries_added} PARENT_SCOPE)
  set (${entries_variable}_deleted ${entries_deleted} PARENT_SCOPE)
  set (${entries_variable}_modified ${entries_modified} PARENT_SCOPE)
  set (${entries_variable}_unversioned ${entries_unversioned} PARENT_SCOPE)
endfunction ()

function (StatusOfSubversionRepositoryEntries entries_variable repository)
  execute_process (
    COMMAND "@Subversion_SVN_EXECUTABLE@" status ${ARGN}
    WORKING_DIRECTORY "${repository}"
    OUTPUT_VARIABLE status
    )

  string (REGEX REPLACE "\\\\" "/" status "${status}")
  string (REGEX REPLACE "[\r\n]+$" "" status "${status}")
  string (REGEX REPLACE "[\r\n]+" ";" status "${status}")

  foreach (entry IN LISTS status)
    if (entry MATCHES "^[A][^ ]*[ ]+(.+)$")
      string (REGEX REPLACE "^[A][^ ]*[ ]+(.+)$" "\\1" file "${entry}")
      get_filename_component (file "${file}" ABSOLUTE BASE_DIR "${repository}")
      list (APPEND entries_added "${file}")
    elseif (entry MATCHES "^[D][^ ]*[ ]+(.+)$")
      string (REGEX REPLACE "^[D][^ ]*[ ]+(.+)$" "\\1" file "${entry}")
      get_filename_component (file "${file}" ABSOLUTE BASE_DIR "${repository}")
      list (APPEND entries_deleted "${file}")
    elseif (entry MATCHES "^[MR][^ ]*[ ]+(.+)$")
      string (REGEX REPLACE "^[MR][^ ]*[ ]+(.+)$" "\\1" file "${entry}")
      get_filename_component (file "${file}" ABSOLUTE BASE_DIR "${repository}")
      list (APPEND entries_modified "${file}")
    elseif (entry MATCHES "^[?][^ ]*[ ]+(.+)$")
      string (REGEX REPLACE "^[?][^ ]*[ ]+(.+)$" "\\1" file "${entry}")
      get_filename_component (file "${file}" ABSOLUTE BASE_DIR "${repository}")
      list (APPEND entries_unversioned "${file}")
    endif ()
  endforeach ()

  set (${entries_variable}_added ${entries_added} PARENT_SCOPE)
  set (${entries_variable}_deleted ${entries_deleted} PARENT_SCOPE)
  set (${entries_variable}_modified ${entries_modified} PARENT_SCOPE)
  set (${entries_variable}_unversioned ${entries_unversioned} PARENT_SCOPE)
endfunction ()

function (StatusOfRepositoryEntries entries_variable repository)
  if (IS_DIRECTORY "${repository}/.git")
    StatusOfGitRepositoryEntries (entries "${repository}" ${ARGN})
  elseif (IS_DIRECTORY "${repository}/.svn")
    StatusOfSubversionRepositoryEntries (entries "${repository}" ${ARGN})
  endif ()

  set (${entries_variable}_added ${entries_added} PARENT_SCOPE)
  set (${entries_variable}_deleted ${entries_deleted} PARENT_SCOPE)
  set (${entries_variable}_modified ${entries_modified} PARENT_SCOPE)
  set (${entries_variable}_unversioned ${entries_unversioned} PARENT_SCOPE)
endfunction ()

function (AddedOrModifiedRepositoryEntries modified_entries_variable repository)
  StatusOfRepositoryEntries (entries "${repository}" ${ARGN})
  set (${modified_entries_variable} ${entries_added} ${entries_modified} ${entries_unversioned} PARENT_SCOPE)
endfunction ()

function (WriteAddedOrModifiedFileList)
  AddedOrModifiedRepositoryEntries (added_or_modified_entries "${PARAMETER_REPOSITORY}" .)
  WriteFileList ("${PARAMETER_OUTPUT}" ${added_or_modified_entries})
endfunction ()

function (RelevantTargets relevant_targets_variable)
  foreach (target IN LISTS ARGN)
    get_target_property (type ${target} TYPE)
    if (" STATIC_LIBRARY MODULE_LIBRARY SHARED_LIBRARY EXECUTABLE " MATCHES " ${type} ")
      get_target_property (exclude_from_coding_guidelines ${target} EXCLUDE_FROM_CODING_GUIDELINES)
      if (NOT exclude_from_coding_guidelines)
        list (APPEND relevant_targets ${target})
      endif ()
    endif ()
  endforeach ()

  set (${relevant_targets_variable} "${relevant_targets}" PARENT_SCOPE)
endfunction ()

function (AllTargetsInDirectory targets_variable directory)
  get_directory_property(targets DIRECTORY ${directory} BUILDSYSTEM_TARGETS)
  get_directory_property(subdirectories DIRECTORY ${directory} SUBDIRECTORIES)

  foreach (subdirectory IN LISTS subdirectories)
    AllTargetsInDirectory (subdirectory_targets ${subdirectory})
    list (APPEND targets ${subdirectory_targets})
  endforeach ()

  set (${targets_variable} ${targets} PARENT_SCOPE)
endfunction ()

function (ConfigureCodingGuidelinesOptions)
  set (CODING_GUIDELINES_FILES_TO_CHECK ADDED_OR_MODIFIED_FILES CACHE STRING "files to check")
  mark_as_advanced (CODING_GUIDELINES_FILES_TO_CHECK)
  set_property (CACHE CODING_GUIDELINES_FILES_TO_CHECK PROPERTY STRINGS ALL_VERSIONED_FILES ADDED_OR_MODIFIED_FILES CUSTOM_FILE_LIST)

  set (CODING_GUIDELINES_FILE_LIST "" CACHE FILEPATH "file list")
  mark_as_advanced (CODING_GUIDELINES_FILE_LIST)
endfunction ()

function (GetAddedOrModifiedFileListPath added_or_modified_file_list_variable)
  if (CODING_GUIDELINES_FILES_TO_CHECK STREQUAL "CUSTOM_FILE_LIST")
    set (${added_or_modified_file_list_variable} "${CODING_GUIDELINES_FILE_LIST}" PARENT_SCOPE)
    return ()
  endif ()

  GetCodingGuidelinesSourceDir (source_dir)
  set (${added_or_modified_file_list_variable} "${source_dir}/added_or_modified_file_list.txt" PARENT_SCOPE)
endfunction ()

function (RelevantSourcesFromTargetList relevant_sources_variable)
  foreach (target IN LISTS ARGN)
    RelevantSourcesFromTarget (sources "${target}")
    list (APPEND relevant_sources ${sources})
  endforeach ()

  set (${relevant_sources_variable} ${relevant_sources} PARENT_SCOPE)
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

function (ConfigureCodingGuidelinesTargetFromTargets coding_guideline_target json_filename)
  CreateCodingGuidelinesSourcesFromTargets (coding_guidelines_sources "${json_filename}" ${ARGN})

  add_library ("${coding_guideline_target}" SHARED EXCLUDE_FROM_ALL "${json_filename}" ${coding_guidelines_sources})
  target_compile_options("${coding_guideline_target}" PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wno-error>)
  set_target_properties ("${coding_guideline_target}" PROPERTIES FOLDER "[coding guidelines]" EXCLUDE_FROM_CODING_GUIDELINES true)
  
  foreach (target IN LISTS ARGN)
    add_dependencies ("${target}" "${coding_guideline_target}")
  endforeach()
endfunction ()

function (ConfigureCodingGuidelinesForTargets json_filename)
  GetCodingGuidelinesSourceDir (source_dir)
  set (cmake_lists_file "${source_dir}/CMakeLists.txt")
  
  if (NOT CMAKE_CURRENT_LIST_FILE STREQUAL cmake_lists_file)
    get_filename_component (json_filename "${json_filename}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    file (WRITE "${cmake_lists_file}" "ConfigureCodingGuidelinesForTargets (\"${json_filename}\" ${ARGN})")
    add_subdirectory ("${source_dir}" "${source_dir}Build")
  
    return ()
  endif ()

  message (STATUS "Configuring coding guidelines")

  ConfigureCodingGuidelinesOptions ()
  ConfigureCodingGuidelinesTargetFromTargets (ALL_CODING_GUIDELINES "${json_filename}" ${ARGN})

  if (CODING_GUIDELINES_FILES_TO_CHECK STREQUAL "ADDED_OR_MODIFIED_FILES")
    GetRepositoryPath (repository)
    GetAddedOrModifiedFileListPath (added_or_modified_file_list)
    GetCodingGuidelinesSourceDir (source_dir)
    GetCodingGuidelinesScript (script)
    RelevantSourcesFromTargetList(all_relevant_sources ${ARGN})

    add_custom_command (
      OUTPUT "${source_dir}/added_or_modified.cpp"
      COMMAND "${CMAKE_COMMAND}" -D "SCRIPT_FUNCTION=WriteAddedOrModifiedFileList" -D "PARAMETER_REPOSITORY=${repository}" -D "PARAMETER_OUTPUT=${added_or_modified_file_list}" -P "${script}"
      COMMAND "${CMAKE_COMMAND}" -E copy "${source_dir}/dummy.cpp" "${source_dir}/added_or_modified.cpp"
      DEPENDS "${script}" ${all_relevant_sources}
      BYPRODUCTS "${added_or_modified_file_list}"
      )

    add_library (ADDED_OR_MODIFIED_FILE_LIST MODULE EXCLUDE_FROM_ALL "${source_dir}/added_or_modified.cpp")
    set_target_properties (ADDED_OR_MODIFIED_FILE_LIST PROPERTIES FOLDER "[coding guidelines]" EXCLUDE_FROM_CODING_GUIDELINES true)

    add_dependencies(ALL_CODING_GUIDELINES ADDED_OR_MODIFIED_FILE_LIST)
  endif ()

  message (STATUS "Configuration done")
endfunction ()

function (CreateCodingGuidelinesSourceForSources filename json_filename)
  WriteFileList ("${filename}.sources.txt" ${ARGN})

  if (CODING_GUIDELINES_FILES_TO_CHECK STREQUAL "ALL_VERSIONED_FILES")
    add_custom_command (
      OUTPUT "${filename}"
      COMMAND "$<TARGET_FILE:CodingGuidelinesChecker>" "-o=${filename}" "${json_filename}" "${filename}.sources.txt"
      DEPENDS CodingGuidelinesChecker "${json_filename}" ${ARGN}
      )
  elseif (CODING_GUIDELINES_FILES_TO_CHECK STREQUAL "ADDED_OR_MODIFIED_FILES")
    GetAddedOrModifiedFileListPath (added_or_modified_file_list)
    add_custom_command (
      OUTPUT "${filename}"
      COMMAND "$<TARGET_FILE:CodingGuidelinesChecker>" "-f=${added_or_modified_file_list}" "-o=${filename}" "${json_filename}" "${filename}.sources.txt"
      DEPENDS CodingGuidelinesChecker "${added_or_modified_file_list}" "${json_filename}" ${ARGN}
      )
  endif ()
endfunction ()

function (CreateCodingGuidelinesSourcesForTarget coding_guidelines_sources_variable json_filename target)
  RelevantSourcesFromTarget (relevant_sources "${target}")
  GetCodingGuidelinesSourceDir (source_dir)

  set (index 0)
  while (relevant_sources)
    math (EXPR index "${index} + 1")
    set (coding_guidelines_source "${source_dir}/${target}_part_${index}.cpp")
    list (APPEND coding_guidelines_sources "${coding_guidelines_source}")

    RemoveFirstElementsFromList (sources 128 relevant_sources)
    CreateCodingGuidelinesSourceForSources ("${coding_guidelines_source}" "${json_filename}" ${sources})
  endwhile ()

  set (${coding_guidelines_sources_variable} ${coding_guidelines_sources} PARENT_SCOPE)
endfunction ()

function (CreateCodingGuidelinesSourcesFromTargets all_sources_variable json_filename)
  foreach (target IN LISTS ARGN)
    CreateCodingGuidelinesSourcesForTarget (sources "${json_filename}" "${target}")
    list (APPEND all_sources ${sources})
  endforeach()

  set (${all_sources_variable} ${all_sources} PARENT_SCOPE)
endfunction ()

function (ConfigureCodingGuidelinesForAllTargets json_filename)
  if (DISABLE_CODING_GUIDELINES)
    return()
  endif()
  AllTargetsInDirectory (all_targets "${CMAKE_SOURCE_DIR}")
  RelevantTargets (relevant_targets ${all_targets})
  ConfigureCodingGuidelinesForTargets ("${json_filename}" ${relevant_targets})
endfunction ()

function (ConfigureScript)
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/coding_guidelines/checker" "${CMAKE_CURRENT_BINARY_DIR}coding_guidelines/checker")
  target_compile_options (CodingGuidelinesChecker BEFORE PRIVATE $<$<CONFIG:Debug>:-O2> $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:-Ob2>)

  define_property(TARGET PROPERTY EXCLUDE_FROM_CODING_GUIDELINES
                 BRIEF_DOCS "exclude target from coding guidelines"
                 FULL_DOCS "exclude target from coding guidelines")

  AllTargetsInDirectory (targets "${CMAKE_CURRENT_LIST_DIR}/coding_guidelines/checker")
  RelevantTargets (targets ${targets})

  set_target_properties (${targets} PROPERTIES
    FOLDER "[coding guidelines]"
    EXCLUDE_FROM_CODING_GUIDELINES true
    )

  GetCodingGuidelinesSourceDir (source_dir)
  configure_file ("${CMAKE_CURRENT_LIST_DIR}/coding_guidelines/dummy.cpp" "${source_dir}/dummy.cpp" COPYONLY)

  GetCodingGuidelinesScript (script)
  if ("${CMAKE_CURRENT_LIST_FILE}" IS_NEWER_THAN "${script}")
    file (REMOVE "${script}")
  endif ()

  find_package (Git)
  find_package (Subversion)

  if (NOT Git_FOUND AND NOT Subversion_FOUND)
    message (FATAL_ERROR "SCM client not found!")
  endif()

  configure_file ("${CMAKE_CURRENT_LIST_FILE}" "${script}" @ONLY ESCAPE_QUOTES)
endfunction ()

if (DISABLE_CODING_GUIDELINES)
  return()
endif()

if (SCRIPT_FUNCTION STREQUAL "WriteAddedOrModifiedFileList")
  WriteAddedOrModifiedFileList ()
  return ()
elseif (SCRIPT_FUNCTION)
  message (FATAL_ERROR "Unknown script function")
endif ()

ConfigureScript ()
