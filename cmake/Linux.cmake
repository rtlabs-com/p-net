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

option (USE_SCHED_FIFO
  "Use SCHED_FIFO policy. May require extra privileges to run"
  OFF)

if (USE_SCHED_FIFO)
  add_compile_definitions(USE_SCHED_FIFO)
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
  )

target_compile_options(profinet
  PRIVATE
  -Wall
  -Wextra
  -Werror
  -Wno-unused-parameter
  INTERFACE
  $<$<CONFIG:Coverage>:--coverage>
  )

target_link_libraries(profinet
  PUBLIC
  pthread
  rt
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
  src/ports/linux/sampleapp_main.c
  )

file(COPY
  src/ports/linux/set_network_parameters
  sample_app/set_profinet_leds_linux
  sample_app/set_profinet_leds_linux.raspberrypi
  DESTINATION
  ${PROFINET_BINARY_DIR}/
  )

if (BUILD_TESTING)
  set(GOOGLE_TEST_INDIVIDUAL TRUE)
  target_sources(pf_test
    PRIVATE
    ${PROFINET_SOURCE_DIR}/src/ports/linux/pnal.c
    )
  target_include_directories(pf_test
    PRIVATE
    src/ports/linux
    )
endif()
