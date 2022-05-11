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

cmake_minimum_required (VERSION 3.14)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools")
project (PROFINET VERSION 0.2.0)

# Default settings if this is the main project
if (CMAKE_PROJECT_NAME STREQUAL PROFINET)
  include(CTest)

  # Make option visible in ccmake, cmake-gui
  option (BUILD_SHARED_LIBS "Build shared library" OFF)

  # Default to release build with debug info
  if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
  endif (NOT CMAKE_BUILD_TYPE)

  # Default to installing in build directory
  if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${PROFINET_BINARY_DIR}/install
      CACHE PATH "Default install path" FORCE)
  endif()

  message(STATUS "Current build type is: ${CMAKE_BUILD_TYPE}")
  message(STATUS "Current install path is: ${CMAKE_INSTALL_PREFIX}")
  message(STATUS "Building for ${CMAKE_SYSTEM_NAME}")
endif()

include(AddOsal)
include(GenerateExportHeader)
include(CMakeDependentOption)
include(GetGitRevision)

# Always use standard .o suffix
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

# Options. See options.h.in for more information.

option (PNET_OPTION_FAST_STARTUP "" ON)
option (PNET_OPTION_PARAMETER_SERVER "" ON)
option (PNET_OPTION_IR "" ON)
option (PNET_OPTION_SR "" ON)
option (PNET_OPTION_REDUNDANCY "" ON)
option (PNET_OPTION_AR_VENDOR_BLOCKS "" ON)
option (PNET_OPTION_RS "" ON)
option (PNET_OPTION_MC_CR "" ON)
option (PNET_OPTION_SRL "" OFF)
option (PNET_OPTION_SNMP "" OFF)
option (PNET_OPTION_DRIVER_ENABLE "Enable drivers. Specific driver must be enabled." OFF )

# TODO: this should be handled in cc.h
option (PNET_USE_ATOMICS "Enable use of atomic operations (stdatomic.h)" OFF)

set(PNET_MAX_AR                 2
  CACHE STRING "Number of connections. Must be > 0. 'Automated RT Tester' uses 2, but only 1 connection AR is supported.")
set(PNET_MAX_API                1
  CACHE STRING "Number of Application Processes. Must be > 0")
set(PNET_MAX_CR                 2
  CACHE STRING "Per AR. At least 2 (1 input and 1 output). If unsure, use 2.")
set(PNET_MAX_SLOTS            5
  CACHE STRING "Per API. Should be > 1 to allow at least one I/O module")
set(PNET_MAX_SUBSLOTS         3
  CACHE STRING "Per slot (DAP requires 2 + PNET_MAX_PHYSICAL_PORTS)")
set(PNET_MAX_DFP_IOCR           2
  CACHE STRING "Allowed values are 0 (zero) or 2")
set(PNET_MAX_PHYSICAL_PORTS          1
  CACHE STRING "Max number of physical ports")
set(PNET_MAX_LOG_BOOK_ENTRIES   16
  CACHE STRING "")
set(PNET_MAX_ALARMS             6
  CACHE STRING "Per AR and queue. One queue for high and one for low priority alarms. 'Automated RT Tester' uses 6.")
set(PNET_MAX_ALARM_PAYLOAD_DATA_SIZE 28
  CACHE STRING "Min 24 or PNET_MAX_DIAG_MANUF_DATA_SIZE + 12. Max is 1408")
set(PNET_MAX_DIAG_ITEMS         200
  CACHE STRING "Total, per device. Max is 65534 items")
set(PNET_MAX_DIAG_MANUF_DATA_SIZE 16
  CACHE STRING "Min 5 for tests. Max is 1396")
set(PNET_MAX_MC_CR              1
  CACHE STRING "Per AR")
set(PNET_MAX_AR_VENDOR_BLOCKS   1
  CACHE STRING "Must be > 0")
set(PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH 512
  CACHE STRING "")
set(PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH 0
  CACHE STRING "or 512 (bytes")
set(PNET_MAX_SESSION_BUFFER_SIZE 4500
  CACHE STRING "Max fragmented RPC request/response length. Max value 65535")
set(PNET_MAX_DIRECTORYPATH_SIZE 240
  CACHE STRING "Max size of directory path, including termination")
set(PNET_MAX_FILENAME_SIZE 30
  CACHE STRING "Max size of filename, including termination")
set(PNET_MAX_PORT_DESCRIPTION_SIZE 60
  CACHE STRING "Max size of port description, including termination")

set(LOG_STATE_VALUES "ON;OFF")
set(LOG_LEVEL_VALUES "DEBUG;INFO;WARNING;ERROR;FATAL")

set(LOG_LEVEL FATAL CACHE STRING "default log level")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS ${LOG_LEVEL_VALUES})

set(PF_ETH_LOG ON CACHE STRING "pf_eth log")
set_property(CACHE PF_ETH_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PF_LLDP_LOG ON CACHE STRING "pf_lldp log")
set_property(CACHE PF_LLDP_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PF_SNMP_LOG ON CACHE STRING "pf_snmp log")
set_property(CACHE PF_SNMP_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

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

set(PF_PNAL_LOG ON CACHE STRING "PNAL log")
set_property(CACHE PF_PNAL_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

set(PNET_LOG ON CACHE STRING "PNET log")
set_property(CACHE PNET_LOG PROPERTY STRINGS ${LOG_STATE_VALUES})

# Generate version numbers
configure_file (
  pnet_version.h.in
  ${PROFINET_BINARY_DIR}/include/pnet_version.h
  )

# Generate config options
configure_file (
  options.h.in
  ${PROFINET_BINARY_DIR}/src/options.h
  )
configure_file (
  pnet_options.h.in
  ${PROFINET_BINARY_DIR}/include/pnet_options.h
  )

# Add platform-dependent targets early, so they can be configured by
# platform
add_library(profinet "")
add_executable(pn_dev "")

if (PNET_OPTION_DRIVER_ENABLE)
  include(${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/drivers.cmake)
endif ()

if (CMAKE_PROJECT_NAME STREQUAL PROFINET AND BUILD_TESTING)
  add_executable(pf_test "")
endif()

# Platform configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CMAKE_SYSTEM_NAME}.cmake)

generate_export_header(profinet
  BASE_NAME pnet
  EXPORT_FILE_NAME ${PROFINET_BINARY_DIR}/include/pnet_export.h
  )

set_target_properties (profinet pn_dev
  PROPERTIES
  C_STANDARD 99
  )

target_compile_features(profinet PUBLIC c_std_99)

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

target_link_libraries(profinet PUBLIC osal)

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
  ${PROFINET_BINARY_DIR}/include/pnet_options.h
  ${PROFINET_BINARY_DIR}/include/pnet_version.h
  DESTINATION include
  )

add_subdirectory (src)
add_subdirectory (samples/pn_dev)

if (CMAKE_PROJECT_NAME STREQUAL PROFINET AND BUILD_TESTING)
  add_subdirectory (test)
  include(AddGoogleTest)
  add_gtest(pf_test)
endif()

# Doxygen configuration
cmake_policy(SET CMP0057 NEW)
find_package(Doxygen)
if (CMAKE_PROJECT_NAME STREQUAL PROFINET AND DOXYGEN_FOUND)
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
  set(DOXYGEN_RECURSIVE YES)
  set(DOXYGEN_GENERATE_XML YES)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  doxygen_add_docs(docs
    README.md
    include
    src
    )
endif()

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${PROFINET_SOURCE_DIR}/LICENSE.md")
set (CPACK_PACKAGE_CONTACT info.profinet@rt-labs.com)
include (CPack)
