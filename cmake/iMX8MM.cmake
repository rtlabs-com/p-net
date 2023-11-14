#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2021 rt-labs AB, Sweden.
# Copyright 2023 NXP
#
# This software is dual-licensed under GPLv3 and a commercial
# license. See the file LICENSE.md distributed with this software for
# full license information.
#*******************************************************************/

target_include_directories(profinet
  PRIVATE
  src/ports/iMX8M
  )

target_sources(profinet
  PRIVATE
  src/ports/iMX8M/pnal.c
  src/ports/iMX8M/pnal_eth.c
  src/ports/iMX8M/pnal_udp.c
  )

target_include_directories(pn_dev
  PRIVATE
  samples/pn_dev
  src/ports/iMX8M
  )

target_sources(pn_dev
  PRIVATE
  samples/pn_dev/sampleapp_common.c
  samples/pn_dev/app_utils.c
  samples/pn_dev/app_log.c
  samples/pn_dev/app_gsdml.c
  samples/pn_dev/app_data.c
  src/ports/iMX8M/sampleapp_main.c
  src/ports/iMX8M/sampleapp_imx8mmevk.c
  )

target_link_libraries(pn_dev PRIVATE mcuxsdk-bsp)

install (FILES
  src/ports/iMX8M/pnal_config.h
  DESTINATION include
  )

generate_bin(pn_dev)

if (BUILD_TESTING)
  target_include_directories(pf_test
    PRIVATE
    src/ports/iMX8M
    )
  target_link_libraries(pf_test PRIVATE mcuxsdk-bsp)
  generate_bin(pf_test)
endif()
