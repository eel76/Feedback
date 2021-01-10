cmake_minimum_required(VERSION 3.15)

include(FetchContent)

# FIXME: Repository URL eintragen

FetchContent_Declare (Feedback SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(Feedback)
