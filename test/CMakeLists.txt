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

target_sources(pf_test PRIVATE
  # Unit tests
  test_diag.cpp
  test_cmdev.cpp
  test_cmdmc.cpp
  test_cmina.cpp
  test_cmio.cpp
  test_cmpbe.cpp
  test_cmrdr.cpp
  test_cmrpc.cpp
  test_cmrs.cpp
  test_cmsm.cpp
  test_cmsu.cpp
  test_cmwdr.cpp
  test_alarm.cpp
  test_cpm.cpp
  test_dcp.cpp
  test_lldp.cpp
  test_pnetapi.cpp
  test_ppm.cpp
  test_ptcp.cpp
  test_scheduler.cpp
  test_eth.cpp
  test_osal.cpp

  # Mocks
  mocks.h
  mocks.cpp

  # Testrunner
  pf_test.cpp
  )

# Rebuild units to be tested with UNIT_TEST flag set. This is used to
# mock external dependencies.
target_sources(pf_test PRIVATE
  # Units to be tested
  ${PROFINET_SOURCE_DIR}/src/device/pf_block_reader.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_block_writer.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_fspm.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_diag.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmdev.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmdmc.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmina.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmio.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmpbe.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmrdr.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmrpc.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmrs.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmsm.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmsu.c
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmwrr.c
  ${PROFINET_SOURCE_DIR}/src/device/pnet_api.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_alarm.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_cpm.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_dcp.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_ppm.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_ptcp.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_scheduler.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_eth.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_lldp.c
  )

get_target_property(PROFINET_OPTIONS profinet COMPILE_OPTIONS)
target_compile_options(pf_test PRIVATE
  -DUNIT_TEST
  ${PROFINET_OPTIONS}
  )

target_include_directories(pf_test
  PRIVATE
  ${PROFINET_SOURCE_DIR}/src
  ${PROFINET_SOURCE_DIR}/src/common
  ${PROFINET_SOURCE_DIR}/src/device
  ${PROFINET_BINARY_DIR}/src
  )

# Link against profinet target to get the cmake properties for that
# target, such as include paths and libraries.
target_link_libraries(pf_test
  PRIVATE
  profinet
  )
