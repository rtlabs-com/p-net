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
  )

target_sources(profinet
  PRIVATE
  src/ports/windows/pnal.c
  src/ports/windows/pnal_eth.c
  src/ports/windows/pnal_udp.c
  )

target_compile_options(profinet
  PRIVATE
  )
  
target_link_libraries(profinet
  PUBLIC
  wsock32 
  ws2_32
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

