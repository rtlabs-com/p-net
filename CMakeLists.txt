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

cmake_minimum_required (VERSION 3.11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
project (PROFINET VERSION 0.1.0)
include(CTest)

# Set required standard level
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

# Always use standard .o suffix
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

# Options
include(CMakeDependentOption)

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
  set(CMAKE_INSTALL_PREFIX ${PROFINET_BINARY_DIR}/install)
endif()

# Generate version numbers
configure_file (
  ${PROFINET_SOURCE_DIR}/version.h.in
  ${PROFINET_BINARY_DIR}/src/version.h
  )
# Generate config options
configure_file (
  ${PROFINET_SOURCE_DIR}/options.h.in
  ${PROFINET_BINARY_DIR}/src/options.h
  )

# Include paths
include_directories(
  ${PROFINET_BINARY_DIR}/include
  ${PROFINET_BINARY_DIR}/src
  ${PROFINET_SOURCE_DIR}/include
  ${PROFINET_SOURCE_DIR}/src
  ${PROFINET_SOURCE_DIR}/src/common
  ${PROFINET_SOURCE_DIR}/src/device
  )

# Platform flags, OSAL sources
include(${CMAKE_SYSTEM_NAME})

# Source paths
add_subdirectory (src)
add_subdirectory (test)
add_subdirectory (sample_app)

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${PROFINET_SOURCE_DIR}/LICENSE.md")
set (CPACK_PACKAGE_CONTACT info.profinet@rt-labs.com)
include (CPack)
