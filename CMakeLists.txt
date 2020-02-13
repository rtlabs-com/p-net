#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2018 rt-labs AB, Sweden.
#
# This software is dual-licensed under GPLv3 and a commercial
# license. See the file LICENSE.md distributed with this software for
# full license information.
#*******************************************************************/

cmake_minimum_required (VERSION 3.13)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
project (PROFINET VERSION 0.1.0)

include(CTest)
include(GenerateExportHeader)
include(CMakeDependentOption)

# Set required standard level
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

# Always use standard .o suffix
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

# Options

option (BUILD_SHARED_LIBS "Build shared library" OFF)

set(LOG_STATE_VALUES "ON;OFF")
set(LOG_LEVEL_VALUES "DEBUG;INFO;WARNING;ERROR")

set(LOG_LEVEL ERROR CACHE STRING "default log level")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS ${LOG_LEVEL_VALUES})

set(PF_ETH_LOG ON CACHE STRING "pf_eth log")
set_property(CACHE PF_ETH_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PF_CPM_LOG ON CACHE STRING "pf_cpm log")
set_property(CACHE PF_CPM_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PF_PPM_LOG ON CACHE STRING "pf_ppm log")
set_property(CACHE PF_PPM_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PF_DCP_LOG ON CACHE STRING "pf_dcp log")
set_property(CACHE PF_DCP_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PF_RPC_LOG ON CACHE STRING "pf_rpc log")
set_property(CACHE PF_RPC_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PF_ALARM_LOG ON CACHE STRING "pf_alarm log")
set_property(CACHE PF_ALARM_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PF_AL_BUF_LOG ON CACHE STRING "pf_alarm_buffer log")
set_property(CACHE PF_AL_BUF_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PNET_LOG ON CACHE STRING "PNET log")
set_property(CACHE PNET_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

# Default to release build with debug info
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif (NOT CMAKE_BUILD_TYPE)
message (STATUS "Current build type is: ${CMAKE_BUILD_TYPE}")

# Default to installing in build directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${PROFINET_BINARY_DIR}/install
    CACHE PATH "Default install path" FORCE)
endif()
message(STATUS "Current install path is: ${CMAKE_INSTALL_PREFIX}")

# Generate version numbers
configure_file (
  version.h.in
  ${PROFINET_BINARY_DIR}/src/version.h
  )
# Generate config options
configure_file (
  options.h.in
  ${PROFINET_BINARY_DIR}/src/options.h
  )

# Add platform-dependent targets early, so they can be configured by
# platform
add_library(profinet "")
add_executable(pn_dev "")

if (BUILD_TESTING)
  add_executable(pf_test "")
endif()

# Platform configuration
include(${CMAKE_SYSTEM_NAME})
message (STATUS "Building for ${CMAKE_SYSTEM_NAME}")

generate_export_header(profinet
  BASE_NAME pnet
  EXPORT_FILE_NAME ${PROFINET_BINARY_DIR}/include/pnet_export.h
  )

target_include_directories(profinet
  PUBLIC
  $<BUILD_INTERFACE:${PROFINET_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROFINET_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  ${PROFINET_BINARY_DIR}/src
  src
  src/common
  src/device
  )

install (
  TARGETS profinet
  EXPORT ProfinetConfig
  DESTINATION lib
  )

install(
  EXPORT ProfinetConfig
  DESTINATION cmake
  )

install (FILES
  include/pnet_api.h
  ${PROFINET_BINARY_DIR}/include/pnet_export.h
  DESTINATION include
  )

add_subdirectory (src)
add_subdirectory (sample_app)

if (BUILD_TESTING)
  add_subdirectory (test)
  include(AddGoogleTest)
  add_gtest(pf_test)
endif()

# Doxygen configuration
cmake_policy(SET CMP0057 NEW)
find_package(Doxygen)
if (DOXYGEN_FOUND)
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
  set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)
  set(DOXYGEN_STRIP_CODE_COMMENTS NO)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
  set(DOXYGEN_PREDEFINED PNET_EXPORT)
  set(DOXYGEN_EXPAND_AS_DEFINED PNET_EXPORT)
  set(DOXYGEN_COLLABORATION_GRAPH NO)
  set(DOXYGEN_INCLUDE_GRAPH NO)
  set(DOXYGEN_INCLUDED_BY_GRAPH NO)
  set(DOXYGEN_RECURSIVE NO)
  set(DOXYGEN_GENERATE_XML YES)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  doxygen_add_docs(docs
    README.md
    include
    src
    )
endif (DOXYGEN_FOUND)

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${PROFINET_SOURCE_DIR}/LICENSE.md")
set (CPACK_PACKAGE_CONTACT info.profinet@rt-labs.com)
include (CPack)
