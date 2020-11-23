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

# Find Net-SNMP

find_path(NetSNMP_INCLUDE_DIR net-snmp-includes.h
  PATH_SUFFIXES net-snmp
  )
find_library(NetSNMP_LIBRARY netsnmp)
mark_as_advanced(NetSNMP_INCLUDE_DIR NetSNMP_LIBRARY)

find_package_handle_standard_args(NetSNMP
  REQUIRED_VARS NetSNMP_LIBRARY NetSNMP_INCLUDE_DIR
  )

if (NetSNMP_FOUND AND NOT TARGET NetSNMP::NetSNMP)
  add_library(NetSNMP::NetSNMP UNKNOWN IMPORTED)
  set_target_properties(NetSNMP::NetSNMP PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${NetSNMP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${NetSNMP_INCLUDE_DIRS}"
    )
endif()
