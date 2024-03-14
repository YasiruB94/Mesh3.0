# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
set(ESPNOW_PATH $ENV{MDF_PATH}/esp-now-patch)
set(EXTRA_COMPONENT_DIRS ${ESPNOW_PATH}/components)
include($ENV{MDF_PATH}/project.cmake)

# code to store current git branch, commit and commit time in firmware
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE COMMIT_BRANCH )
execute_process(
    COMMAND git show --format="%h" --no-patch
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE COMMIT_HASH )
execute_process(
    COMMAND git show --format="%cI" --no-patch
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE COMMIT_TIME )
string(REGEX REPLACE "\n$" "" COMMIT_BRANCH "${COMMIT_BRANCH}")
string(REGEX REPLACE "\"" "" COMMIT_HASH "${COMMIT_HASH}")
string(REGEX REPLACE "\n$" "" COMMIT_HASH "${COMMIT_HASH}")
string(REGEX REPLACE "\"" "" COMMIT_TIME "${COMMIT_TIME}")
string(REGEX REPLACE "\n$" "" COMMIT_TIME "${COMMIT_TIME}")
add_definitions(-DGIT_BRANCH="${COMMIT_BRANCH}")
add_definitions(-DGIT_HASH="${COMMIT_HASH}")
add_definitions(-DGIT_TIME="${COMMIT_TIME}")

project(Mesh2_0)

