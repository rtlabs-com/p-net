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

set(APP_SOURCES
  ${PROFINET_SOURCE_DIR}/sample_app/main_linux.c)
set(OSAL_SOURCES
  ${PROFINET_SOURCE_DIR}/src/osal/linux/osal.c
  ${PROFINET_SOURCE_DIR}/src/osal/linux/osal_eth.c
  ${PROFINET_SOURCE_DIR}/src/osal/linux/osal_udp.c
  )
set(OSAL_INCLUDES
  ${PROFINET_SOURCE_DIR}/src/osal/linux
  )
set(OSAL_LIBS
  "dl"
  "pthread"
  "rt"
  )

set(GOOGLE_TEST_INDIVIDUAL TRUE)

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter -Werror")
set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})

set(CMAKE_C_FLAGS_COVERAGE "-fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_FLAGS_COVERAGE ${CMAKE_C_FLAGS_COVERAGE})

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections")
