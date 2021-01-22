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


#npcap: https://nmap.org/npcap/
cmake_minimum_required(VERSION 3.18)
set(PCAP_DIR ${CMAKE_BINARY_DIR}/ext/pcap)
set(PCAP_ZIP ${CMAKE_BINARY_DIR}/ext/npcap.zip)
if(NOT EXISTS ${PCAP_ZIP})
  message("Downloading NPcap SDK")
  file(DOWNLOAD 
    https://nmap.org/npcap/dist/npcap-sdk-1.06.zip 
    ${PCAP_ZIP}
    SHA1=e7935f0623fc79adaf8d81b5fbf491c89a9dc022
    SHOW_PROGRESS
  )
  file(ARCHIVE_EXTRACT 
    INPUT ${PCAP_ZIP}
    DESTINATION ${PCAP_DIR}
  )
endif()


#remove Visual Studio unsafe warnings.
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

target_include_directories(profinet
  PRIVATE
  src/ports/windows
  ${PCAP_DIR}/Include
  )

target_sources(profinet
  PRIVATE
  src/ports/windows/pnal.c
  src/ports/windows/pnal_eth.c
  src/ports/windows/pnal_udp.c
  src/ports/windows/pcap_helper.c
  )

target_compile_options(profinet
  PRIVATE
  )

if (${CMAKE_CL_64})
  find_library(NPCAP_LIBRARY wpcap HINT ${PCAP_DIR}/Lib/x64)
else()
  find_library(NPCAP_LIBRARY wpcap HINT ${PCAP_DIR}/Lib)
endif()
  
target_link_libraries(profinet
  PUBLIC
  ws2_32
  IPHLPAPI
  ${NPCAP_LIBRARY}
  )

target_include_directories(pn_dev
  PRIVATE
  sample_app
  src/ports/windows
  )

target_sources(pn_dev
  PRIVATE
  sample_app/sampleapp_common.c
  src/ports/windows/sampleapp_main.c
  )

