#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2020 rt-labs AB, Sweden.
#
# This software is dual-licensed under GPLv3 and a commercial
# license. See the file LICENSE.md distributed with this software for
# full license information.
#*******************************************************************/

include(FindPackageHandleStandardArgs)

# Find Net-SNMP agent

find_path(NetSNMPAgent_INCLUDE_DIR net-snmp-agent-includes.h
  PATH_SUFFIXES net-snmp/agent
  )
find_library(NetSNMPAgent_LIBRARY netsnmpagent)
mark_as_advanced(NetSNMPAgent_INCLUDE_DIR NetSNMPAgent_LIBRARY)

find_package_handle_standard_args(NetSNMPAgent
  REQUIRED_VARS NetSNMPAgent_LIBRARY NetSNMPAgent_INCLUDE_DIR
  )

if (NetSNMPAgent_FOUND AND NOT TARGET NetSNMP::NetSNMPAgent)
  add_library(NetSNMP::NetSNMPAgent UNKNOWN IMPORTED)
  set_target_properties(NetSNMP::NetSNMPAgent PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${NetSNMPAgent_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${NetSNMPAgent_INCLUDE_DIRS}"
    )
endif()
