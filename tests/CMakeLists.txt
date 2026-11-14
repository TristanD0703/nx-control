cmake_minimum_required(VERSION 3.22)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default single-config generators to a debuggable build.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel
    )
endif()

project(nintendo_switch_controller_tests C)

option(ENABLE_SANITIZERS "Enable ASan and UBSan for host tests" ON)
option(ENABLE_COVERAGE "Enable coverage instrumentation and reporting" OFF)

if(ENABLE_SANITIZERS AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "ASan and UBSan require a Clang or GCC compiler")
endif()

if(ENABLE_COVERAGE AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "Coverage reporting requires a Clang or GCC compiler")
endif()

include(CTest)
include(FetchContent)
enable_testing()

find_package(Ruby 3.0 REQUIRED COMPONENTS Interpreter)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.7.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    cmock
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/CMock.git
    GIT_TAG v2.7.0
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
)

FetchContent_MakeAvailable(unity cmock)

add_library(cmock STATIC "${cmock_SOURCE_DIR}/src/cmock.c")
target_include_directories(cmock PUBLIC "${cmock_SOURCE_DIR}/src")
target_link_libraries(cmock PUBLIC unity)

include(cmake/CMock.cmake)

add_subdirectory(host)
