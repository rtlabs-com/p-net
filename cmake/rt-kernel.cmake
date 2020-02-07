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

# Fix inclusion order of assert.h and log.h by including our
# definition before anything else. FIXME: these files should be
# renamed.
include_directories(BEFORE
  src/osal/rt-kernel/
  src
  )

target_include_directories(profinet
  PRIVATE
  src/osal/rt-kernel
  )

target_sources(profinet
  PRIVATE
  src/osal/rt-kernel/osal.c
  src/osal/rt-kernel/osal_eth.c
  src/osal/rt-kernel/osal_udp.c
  src/osal/rt-kernel/dwmac1000.c
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
  src/osal/rt-kernel
  )

target_sources(pn_dev
  PRIVATE
  sample_app/main_rtk.c
  )

if (BUILD_TESTING)
  target_sources(pf_test
    PRIVATE
    ${PROFINET_SOURCE_DIR}/src/osal/rt-kernel/osal.c
    ${PROFINET_SOURCE_DIR}/src/osal/rt-kernel/stubs.c
    )
  target_include_directories(pf_test
    PRIVATE
    src/osal/rt-kernel
    )
endif()
