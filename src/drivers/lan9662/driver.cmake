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

cmake_dependent_option (
  PNET_OPTION_DRIVER_LAN9662
  "Enable LAN9662 HW Offload driver" OFF
  "PNET_OPTION_DRIVER_ENABLE" OFF )

if (PNET_OPTION_DRIVER_LAN9662)

option(PNET_OPTION_LAN9662_SHOW_RTE_INFO "Show RTE config log message" OFF)
set(PNET_LAN9662_MAX_FRAMES 2  CACHE STRING "Max active CPM and PPM instances")
set(PNET_LAN9662_MAX_IDS    16 CACHE STRING "Max subslots for a frame")

set(PNET_LAN9662_VCAM_BASE  2  CACHE STRING "LAN9662 VCAP base index")
set(PNET_LAN9662_RTP_BASE   4  CACHE STRING "LAN9662 RTP base index")
set(PNET_LAN9662_WAL_BASE   6  CACHE STRING "LAN9662 write action list base index")
set(PNET_LAN9662_RAL_BASE   8  CACHE STRING "LAN9662 read action list base index")

message(STATUS "LAN9662 pnet_driver_options.h configuration")
configure_file (
  src/drivers/lan9662/pnet_driver_options.h.in
  include/pnet_driver_options.h
)

message(STATUS "Add LAN9662 targets and configurations")

if(TARGET mera)
  message(STATUS "LAN9662 mera lib found")
  get_target_property(MERA_INCLUDES mera INCLUDE_DIRECTORIES)
else()
  message(STATUS "LAN9662 mera lib not found - fetch repo")
  include(src/drivers/lan9662/add_mera_lib.cmake)
  get_target_property(MERA_INCLUDES mera INCLUDE_DIRECTORIES)
endif()

add_executable(pn_lan9662 "")
add_executable(pn_shm_tool "")

set_target_properties (pn_lan9662 pn_shm_tool
  PROPERTIES
  C_STANDARD 99
  )

add_subdirectory (samples/pn_dev_lan9662)
add_subdirectory (samples/pn_shm_tool)

target_sources(profinet
  PRIVATE
  ${PROFINET_SOURCE_DIR}/src/drivers/lan9662/src/pf_cpm_driver_lan9662.c
  ${PROFINET_SOURCE_DIR}/src/drivers/lan9662/src/pf_lan9662_mera.c
  ${PROFINET_SOURCE_DIR}/src/drivers/lan9662/src/pf_mera_trace.c
  ${PROFINET_SOURCE_DIR}/src/drivers/lan9662/src/pf_mera_trace.c
  ${PROFINET_SOURCE_DIR}/src/drivers/lan9662/src/pf_rte_uio.c
  ${PROFINET_SOURCE_DIR}/src/drivers/lan9662/src/pf_sram_uio.c
  ${PROFINET_SOURCE_DIR}/src/drivers/lan9662/src/pf_ppm_driver_lan9662.c
  )

target_include_directories(profinet
  PRIVATE
  src/drivers/lan9662
  src/drivers/lan9662/include
  ${MERA_INCLUDES}
  )

target_link_libraries(profinet
  PUBLIC
  mera
  )

install (
  TARGETS mera
  EXPORT ProfinetConfig
  DESTINATION lib
  )

target_include_directories(pn_dev
  PRIVATE
  src/drivers/lan9662
  src/drivers/lan9662/include
  )

target_include_directories(pn_lan9662
  PRIVATE
  samples/pn_dev_lan9662
  src/drivers/lan9662
  src/drivers/lan9662/include
  src/ports/linux
  )

target_sources(pn_lan9662
  PRIVATE
  samples/pn_dev_lan9662/sampleapp_common.c
  samples/pn_dev_lan9662/app_utils.c
  samples/pn_dev_lan9662/app_log.c
  samples/pn_dev_lan9662/app_gsdml.c
  samples/pn_dev_lan9662/app_data.c
  samples/pn_dev_lan9662/app_shm.c
  src/ports/linux/sampleapp_main.c
  )

target_compile_options(pn_lan9662
  PRIVATE
  ${APP_COMPILER_FLAGS}
  )

target_link_options(pn_lan9662
   PRIVATE
   -Wl,--gc-sections
  )

target_sources(pn_shm_tool
  PRIVATE
  samples/pn_shm_tool/pn_shm_tool.c
  )

target_compile_options(pn_shm_tool
  PRIVATE
  ${APP_COMPILER_FLAGS}
  )

target_link_options(pn_shm_tool
   PRIVATE
   -Wl,--gc-sections
  )

target_link_libraries (pn_shm_tool PUBLIC rt pthread)

file(COPY
  src/drivers/lan9662/include/driver_config.h
  src/drivers/lan9662/include/pnet_lan9662_api.h
  DESTINATION include
  )

file(COPY
  src/ports/linux/set_network_parameters
  src/drivers/lan9662/set_profinet_leds
  DESTINATION
  ${PROFINET_BINARY_DIR}/
  )

endif()