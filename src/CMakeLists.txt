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

# NOTE: add headers to make them show up in an IDE
# NOTE: Use full path for the <$<BOOL:${PNET_OPTION_SNMP}> expression
#       to work with certain cmake versions
target_sources (profinet PRIVATE
  ${PROFINET_SOURCE_DIR}/include/pnet_api.h
  pf_includes.h
  pf_types.h
  device/pnet_api.c
  device/pf_block_reader.c
  device/pf_block_writer.c
  device/pf_fspm.c
  device/pf_diag.c
  device/pf_cmdev.c
  device/pf_cmdmc.c
  device/pf_cmina.c
  device/pf_cmio.c
  device/pf_cmpbe.c
  device/pf_cmrdr.c
  device/pf_cmrpc.c
  device/pf_cmrpc_epm.c
  device/pf_cmrpc_helpers.c
  device/pf_cmrs.c
  device/pf_cmsm.c
  device/pf_cmsu.c
  device/pf_cmwrr.c
  device/pf_pdport.c
  device/pf_port.c
  device/pf_block_reader.h
  device/pf_block_writer.h
  device/pf_fspm.h
  device/pf_diag.h
  device/pf_cmdev.h
  device/pf_cmdmc.h
  device/pf_cmina.h
  device/pf_cmio.h
  device/pf_cmpbe.h
  device/pf_cmrdr.h
  device/pf_cmrpc.h
  device/pf_cmrpc_epm.h
  device/pf_cmrpc_helpers.h
  device/pf_cmrs.h
  device/pf_cmsm.h
  device/pf_cmsu.h
  device/pf_cmwrr.h
  device/pf_pdport.h
  device/pf_port.h
  common/pf_alarm.c
  common/pf_bg_worker.c
  common/pf_cpm.c
  common/pf_cpm_driver_sw.c
  common/pf_dcp.c
  common/pf_ppm.c
  common/pf_ppm_driver_sw.c
  common/pf_ptcp.c
  common/pf_scheduler.c
  common/pf_eth.c
  common/pf_file.c
  common/pf_lldp.c
  $<$<BOOL:${PNET_OPTION_SNMP}>:${PROFINET_SOURCE_DIR}/src/common/pf_snmp.c>
  common/pf_udp.c
  common/pf_alarm.h
  common/pf_bg_worker.h
  common/pf_cpm.h
  common/pf_cpm_driver_sw.h
  common/pf_dcp.h
  common/pf_file.h
  common/pf_ppm.h
  common/pf_ppm_driver_sw.h
  common/pf_ptcp.h
  common/pf_scheduler.h
  common/pf_eth.h
  common/pf_lldp.h
  common/pf_snmp.h
  common/pf_udp.h
  )
