#[[
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
]]#*/

option (USE_SCHED_FIFO
  "Use SCHED_FIFO policy. May require extra privileges to run"
  OFF)

if (USE_SCHED_FIFO)
  add_compile_definitions(USE_SCHED_FIFO)
endif()

if (PNET_OPTION_SNMP)
  find_package(NetSNMP REQUIRED)
  find_package(NetSNMPAgent REQUIRED)
endif()

set(PNET_SNMP_PRIO 1
  CACHE STRING "SNMP thread priority")
set(PNET_SNMP_STACK_SIZE 256*1024
  CACHE STRING "SNMP thread stack size")

# Generate PNAL options
configure_file (
  src/ports/linux/pnal_options.h.in
  ${PROFINET_BINARY_DIR}/src/ports/linux/pnal_options.h
  )

target_include_directories(profinet
  PRIVATE
  src/ports/linux
  ${PROFINET_BINARY_DIR}/src/ports/linux
  )

target_sources(profinet
  PRIVATE
  src/ports/linux/pnal.c
  src/ports/linux/pnal_eth.c
  src/ports/linux/pnal_udp.c
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/pnal_snmp.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/system_mib.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpLocalSystemData.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpLocPortTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpConfigManAddrTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpLocManAddrTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpRemTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpRemManAddrTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpXdot3LocPortTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpXdot3RemPortTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpXPnoLocTable.c>
  $<$<BOOL:${PNET_OPTION_SNMP}>:src/ports/linux/mib/lldpXPnoRemTable.c>
  )

target_compile_options(profinet
  PRIVATE
  -Wall
  -Wextra
  -Werror
  -Wno-unused-parameter
  INTERFACE
  $<$<CONFIG:Coverage>:--coverage>
  )

target_link_libraries(profinet
  PUBLIC
  pthread
  rt
  $<$<BOOL:${PNET_OPTION_SNMP}>:NetSNMP::NetSNMPAgent>
  $<$<BOOL:${PNET_OPTION_SNMP}>:NetSNMP::NetSNMP>
  INTERFACE
  $<$<CONFIG:Coverage>:--coverage>
  )

target_include_directories(pn_dev
  PRIVATE
  sample_app
  src/ports/linux
  )

target_sources(pn_dev
  PRIVATE
  sample_app/sampleapp_common.c
  src/ports/linux/sampleapp_main.c
  )

file(COPY
  src/ports/linux/set_network_parameters
  sample_app/set_profinet_leds_linux
  sample_app/set_profinet_leds_linux.raspberrypi
  DESTINATION
  ${PROFINET_BINARY_DIR}/
  )

if (BUILD_TESTING)
  set(GOOGLE_TEST_INDIVIDUAL TRUE)
  target_sources(pf_test
    PRIVATE
    ${PROFINET_SOURCE_DIR}/src/ports/linux/pnal.c
    )
  target_include_directories(pf_test
    PRIVATE
    src/ports/linux
    )
endif()
