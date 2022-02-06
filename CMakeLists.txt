cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

project(af-ini VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

# Allow package managers to do their magic
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

# Toggle integration/unit tests
option(AF_INI_UNIT_TESTS "Build unit tests" OFF)
option(AF_INI_INTEGRATION_TESTS "Build integration tests" OFF)

set(LIBRARY_NAME ${PROJECT_NAME})
add_library(${LIBRARY_NAME} INTERFACE)
add_library(${LIBRARY_NAME}::${LIBRARY_NAME} ALIAS ${LIBRARY_NAME})
target_include_directories(${LIBRARY_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(${LIBRARY_NAME} INTERFACE "cxx_std_17")

# Tests
if(AF_INI_UNIT_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()

# Configure installation files
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Useful paths
set(${LIBRARY_NAME}_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(${LIBRARY_NAME}_INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY_NAME})
set(${LIBRARY_NAME}_GENERATED_PATH "${CMAKE_CURRENT_BINARY_DIR}/generated")

write_basic_package_version_file(${${LIBRARY_NAME}_GENERATED_PATH}/${LIBRARY_NAME}-config-version.cmake
    COMPATIBILITY ExactVersion
)

configure_package_config_file(${${LIBRARY_NAME}_CMAKE_DIR}/${LIBRARY_NAME}-config.cmake.in
    ${${LIBRARY_NAME}_GENERATED_PATH}/${LIBRARY_NAME}-config.cmake
    INSTALL_DESTINATION ${${LIBRARY_NAME}_INSTALL_CONFIGDIR}
)

configure_file(${${LIBRARY_NAME}_CMAKE_DIR}/version.hpp.in
    ${${LIBRARY_NAME}_GENERATED_PATH}/include/${LIBRARY_NAME}/version.hpp @ONLY
)

configure_file(${${LIBRARY_NAME}_CMAKE_DIR}/${LIBRARY_NAME}.pc.in
    ${${LIBRARY_NAME}_GENERATED_PATH}/${LIBRARY_NAME}.pc @ONLY)

# Imstall actual files
install(
    DIRECTORY 
        include/${LIBRARY_NAME}
        ${${LIBRARY_NAME}_GENERATED_PATH}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    TARGETS ${LIBRARY_NAME}
    EXPORT ${LIBRARY_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    FILES
        ${${LIBRARY_NAME}_GENERATED_PATH}/${LIBRARY_NAME}-config.cmake
        ${${LIBRARY_NAME}_GENERATED_PATH}/${LIBRARY_NAME}-config-version.cmake
    DESTINATION ${${LIBRARY_NAME}_INSTALL_CONFIGDIR}
)

install(
    FILES 
        ${${LIBRARY_NAME}_GENERATED_PATH}/${LIBRARY_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig/
)

install(
    EXPORT ${LIBRARY_NAME}-targets
    NAMESPACE ${LIBRARY_NAME}::
    DESTINATION ${${LIBRARY_NAME}_INSTALL_CONFIGDIR}
    COMPONENT dev
)
