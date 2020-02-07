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
  src/osal/linux
  )

target_sources(profinet
  PRIVATE
  src/osal/linux/osal.c
  src/osal/linux/osal_eth.c
  src/osal/linux/osal_udp.c
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
  src/osal/linux
  )

target_sources(pn_dev
  PRIVATE
  sample_app/main_linux.c
  )

if (BUILD_TESTING)
  set(GOOGLE_TEST_INDIVIDUAL TRUE)
  target_sources(pf_test
    PRIVATE
    ${PROFINET_SOURCE_DIR}/src/osal/linux/osal.c
    )
  target_include_directories(pf_test
    PRIVATE
    src/osal/linux
    )
endif()
