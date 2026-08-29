cmake_minimum_required(VERSION 3.22)

project(nintendo_switch_controller_tests C)

include(CTest)
include(FetchContent)
enable_testing()

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.7.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(unity)

add_subdirectory(host)
