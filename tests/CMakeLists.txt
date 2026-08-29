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

if(ENABLE_SANITIZERS AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "ASan and UBSan require a Clang or GCC compiler")
endif()

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
