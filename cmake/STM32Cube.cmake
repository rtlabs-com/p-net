#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2021 rt-labs AB, Sweden.
#
# This software is dual-licensed under GPLv3 and a commercial
# license. See the file LICENSE.md distributed with this software for
# full license information.
#*******************************************************************/

target_include_directories(profinet
  PRIVATE
  src/ports/STM32Cube
  )

target_sources(profinet
  PRIVATE
  src/ports/STM32Cube/pnal.c
  src/ports/STM32Cube/pnal_eth.c
  src/ports/STM32Cube/pnal_udp.c
  )

target_compile_options(profinet
  PRIVATE
  -Wall
  -Wextra
  -Werror
  -Wno-unused-parameter
  )

target_include_directories(pn_dev
  PRIVATE
  sample_app
  src/ports/STM32Cube
  )

if (EXISTS ${PROFINET_SOURCE_DIR}/src/ports/STM32Cube/sampleapp_${BOARD}.c)
  set(BOARD_SOURCE sampleapp_${BOARD}.c)
else()
  set(BOARD_SOURCE sampleapp_board.c)
endif()

target_sources(pn_dev
  PRIVATE
  sample_app/sampleapp_common.c
  sample_app/app_utils.c
  sample_app/app_log.c
  sample_app/app_gsdml.c
  sample_app/app_data.c
  src/ports/STM32Cube/sampleapp_main.c
  src/ports/STM32Cube/${BOARD_SOURCE}
  )

target_compile_options(pn_dev
  PRIVATE
  -Wall
  -Wextra
  -Werror
  -Wno-unused-parameter
  )

target_link_libraries(pn_dev PRIVATE cube-bsp)

install (FILES
  src/ports/STM32Cube/pnal_config.h
  DESTINATION include
  )

if (BUILD_TESTING)
  target_include_directories(pf_test
    PRIVATE
    src/ports/STM32Cube
    )
  target_link_libraries(pf_test PRIVATE cube-bsp)
endif()

generate_bin(pn_dev)
generate_bin(pf_test)
