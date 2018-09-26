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

include(GenerateExportHeader)

message (STATUS "Building for ${CMAKE_SYSTEM_NAME}")

# NOTE: add headers to make them show up in an IDE
add_library (profinet
  ${PROFINET_SOURCE_DIR}/include/pnet_api.h
  pf_includes.h
  pf_types.h
  ${PROFINET_SOURCE_DIR}/src/device/pnet_api.c
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
  ${PROFINET_SOURCE_DIR}/src/device/pf_block_reader.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_block_writer.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_fspm.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_diag.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmdev.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmdmc.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmina.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmio.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmpbe.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmrdr.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmrpc.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmrs.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmsm.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmsu.h
  ${PROFINET_SOURCE_DIR}/src/device/pf_cmwrr.h
  ${PROFINET_SOURCE_DIR}/src/common/pf_alarm.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_cpm.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_dcp.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_ppm.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_ptcp.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_scheduler.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_eth.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_lldp.c
  ${PROFINET_SOURCE_DIR}/src/common/pf_alarm.h
  ${PROFINET_SOURCE_DIR}/src/common/pf_cpm.h
  ${PROFINET_SOURCE_DIR}/src/common/pf_dcp.h
  ${PROFINET_SOURCE_DIR}/src/common/pf_ppm.h
  ${PROFINET_SOURCE_DIR}/src/common/pf_ptcp.h
  ${PROFINET_SOURCE_DIR}/src/common/pf_scheduler.h
  ${PROFINET_SOURCE_DIR}/src/common/pf_eth.h
  ${PROFINET_SOURCE_DIR}/src/common/pf_lldp.h
  ${OSAL_SOURCES}
  )
generate_export_header(profinet
  BASE_NAME pnet
  EXPORT_FILE_NAME ${PROFINET_BINARY_DIR}/include/pnet_export.h
  )

include_directories(${OSAL_INCLUDES})

target_link_libraries(profinet ${OSAL_LIBS})

install (TARGETS profinet DESTINATION bin)
install (FILES
  ${PROFINET_SOURCE_DIR}/include/pnet_api.h
  ${PROFINET_BINARY_DIR}/include/pnet_export.h
  DESTINATION include)

# Doxygen configuration
cmake_policy(SET CMP0057 NEW)
find_package(Doxygen)
if (DOXYGEN_FOUND)
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
  set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)
  set(DOXYGEN_STRIP_CODE_COMMENTS NO)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
  set(DOXYGEN_PREDEFINED PNET_EXPORT)
  set(DOXYGEN_EXPAND_AS_DEFINED PNET_EXPORT)
  set(DOXYGEN_COLLABORATION_GRAPH NO)
  set(DOXYGEN_INCLUDE_GRAPH NO)
  set(DOXYGEN_INCLUDED_BY_GRAPH NO)
  set(DOXYGEN_RECURSIVE NO)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  doxygen_add_docs(docs
    ${PROFINET_SOURCE_DIR}/README.md
    ${PROFINET_SOURCE_DIR}/include
    ${PROFINET_SOURCE_DIR}/src
    )
endif (DOXYGEN_FOUND)
