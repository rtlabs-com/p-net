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

if (PNET_OPTION_SNMP)
  find_package(NetSNMP REQUIRED)
  find_package(NetSNMPAgent REQUIRED)
endif()

target_include_directories(profinet
  PRIVATE
  src/ports/linux
  )

target_sources(profinet
  PRIVATE
  src/ports/linux/pnal.c
  src/ports/linux/pnal_eth.c
  src/ports/linux/pnal_udp.c
  src/ports/linux/pnal_filetools.c
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/pnal_snmp.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/system_mib.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpLocalSystemData.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpLocPortTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpConfigManAddrTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpLocManAddrTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpRemTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpRemManAddrTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpXdot3LocPortTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpXdot3RemPortTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpXPnoLocTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpXPnoRemTable.c>
  )

target_compile_options(profinet
  PRIVATE
  -Wall
  -Wextra
  -Werror
  -Wno-unused-parameter
  -ffunction-sections
  -fdata-sections
  INTERFACE
  $<$<CONFIG:Coverage>:--coverage>
  )

target_link_libraries(profinet
  PUBLIC
  $<$<BOOL:${PNET_OPTION_SNMP}>:NetSNMP::NetSNMPAgent>
  $<$<BOOL:${PNET_OPTION_SNMP}>:NetSNMP::NetSNMP>
  INTERFACE
  $<$<CONFIG:Coverage>:--coverage>
  )

target_include_directories(pn_dev
  PRIVATE
  sample_app
  src/ports/linux
  )

target_sources(pn_dev
  PRIVATE
  sample_app/sampleapp_common.c
  sample_app/app_utils.c
  sample_app/app_log.c
  sample_app/app_gsdml.c
  sample_app/app_data.c
  src/ports/linux/sampleapp_main.c
  )

target_compile_options(pn_dev
  PRIVATE
  -Wall
  -Wextra
  -Werror
  -Wno-unused-parameter
  -ffunction-sections
  -fdata-sections
  )

target_link_options(pn_dev
   PRIVATE
   -Wl,--gc-sections
)

install (FILES
  src/ports/linux/pnal_config.h
  DESTINATION include
  )

file(COPY
  src/ports/linux/set_network_parameters
  src/ports/linux/set_profinet_leds
  src/ports/linux/set_profinet_leds.raspberrypi
  DESTINATION
  ${PROFINET_BINARY_DIR}/
  )

if (BUILD_TESTING)
  set(GOOGLE_TEST_INDIVIDUAL TRUE)
  target_include_directories(pf_test
    PRIVATE
    src/ports/linux
    )
endif()
