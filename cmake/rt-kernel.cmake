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

target_include_directories(profinet
  PRIVATE
  src/ports/rt-kernel
  )

target_sources(profinet
  PRIVATE
  src/ports/rt-kernel/pnal.c
  src/ports/rt-kernel/pnal_eth.c
  src/ports/rt-kernel/pnal_udp.c
  src/ports/rt-kernel/pnal_snmp.c
  src/ports/rt-kernel/lldp-mib.c
  src/ports/rt-kernel/lldp-ext-pno-mib.c
  src/ports/rt-kernel/lldp-ext-dot3-mib.c
  src/ports/rt-kernel/rowindex.c
  src/ports/rt-kernel/dwmac1000.c
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
  sample_app
  src/ports/rt-kernel
  )

target_sources(pn_dev
  PRIVATE
  sample_app/sampleapp_common.c
  src/ports/rt-kernel/sampleapp_main.c
  )

if (BUILD_TESTING)
  target_sources(pf_test
    PRIVATE
    ${PROFINET_SOURCE_DIR}/src/ports/rt-kernel/pnal.c
    )
  target_include_directories(pf_test
    PRIVATE
    src/ports/rt-kernel
    )
endif()
