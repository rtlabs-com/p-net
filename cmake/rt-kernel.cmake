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
  ${PROFINET_SOURCE_DIR}/sample_app/main_rtk.c)
set(OSAL_SOURCES
  ${PROFINET_SOURCE_DIR}/src/osal/rt-kernel/stubs.c
  ${PROFINET_SOURCE_DIR}/src/osal/rt-kernel/osal.c
  ${PROFINET_SOURCE_DIR}/src/osal/rt-kernel/osal_eth.c
  ${PROFINET_SOURCE_DIR}/src/osal/rt-kernel/osal_udp.c
  ${PROFINET_SOURCE_DIR}/src/osal/rt-kernel/dwmac1000.c
  )
set(OSAL_INCLUDES
  ${PROFINET_SOURCE_DIR}/src/osal/rt-kernel
  ${RTK}/include
  ${RTK}/include/kern
  ${RTK}/include/arch/${ARCH}
  ${RTK}/include/drivers
  ${RTK}/lwip/src/include
  )
