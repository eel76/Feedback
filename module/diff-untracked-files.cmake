cmake_policy (VERSION 3.15)

execute_process (COMMAND "@GIT_EXECUTABLE@" "ls-files" "--others" "--exclude-standard" WORKING_DIRECTORY "@WORKING_DIRECTORY@" OUTPUT_VARIABLE sources)

STRING (REGEX REPLACE ";" "\\\\;" sources "${sources}")
STRING (REGEX REPLACE "\n\n+" "\n" sources "${sources}")
STRING (REGEX REPLACE "^\n|\n$" "" sources "${sources}")
STRING (REGEX REPLACE "\n" ";" sources "${sources}")

if (EXISTS "@UNTRACKED_FILES_DIFF@")
  file (REMOVE "@UNTRACKED_FILES_DIFF@")
endif ()

file (TOUCH "@UNTRACKED_FILES_DIFF@")

foreach (source IN LISTS sources)
  execute_process (COMMAND "@GIT_EXECUTABLE@" "diff" "--unified=0" "--no-index" "/dev/null" "${source}" WORKING_DIRECTORY "@WORKING_DIRECTORY@" OUTPUT_VARIABLE output)
  file (APPEND "@UNTRACKED_FILES_DIFF@" "${output}")
endforeach ()
