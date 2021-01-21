#********************************************************************
# This software is dual-licensed under GPLv3 and a commercial
# license. See the file LICENSE.md distributed with this software for
# full license information.
#*******************************************************************/

#remove Visual Studio unsafe warnings.
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

target_include_directories(profinet
  PRIVATE
  src/ports/windows
  src/ports/windows/pcap/Include
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

#npcap: https://nmap.org/npcap/
find_library(NPCAP_LIBRARY wpcap HINT src/ports/windows/pcap/Lib/x64)
  
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

