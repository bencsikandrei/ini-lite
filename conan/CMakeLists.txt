cmake_minimum_required(VERSION 3.22)

project(af-ini-lite-conan)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Build type [Debug, RelWithDebInfo, Release, MinSizeRel]"
        FORCE)
    message(STATUS "No build type was specified, will default to ${CMAKE_BUILD_TYPE}")
endif()

# This CMakeList.txt is a wrapper around the main CMakeList.txt in the root
# directory. It aims to setup a full conan environment to ease the project
# build.
include(conan.cmake)

find_program(CONAN_CMD conan REQUIRED)

if(CONAN_CMD)
    conan_check(VERSION 0.0.0)
endif()

conan_add_remote(NAME conancenter
    INDEX 0
    URL https://center.conan.io)

conan_cmake_autodetect(CONAN_SETTINGS
    # we want release versions of third-parties
    BUILD_TYPE Release)

conan_cmake_install(PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}
    SETTINGS ${CONAN_SETTINGS}
    BUILD missing
    INSTALL_FOLDER "${CMAKE_BINARY_DIR}"
    # allow conan to use the environment for building missing packages
    ENV CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER})

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

enable_testing()

add_subdirectory(${PROJECT_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/af-ini-lite)
