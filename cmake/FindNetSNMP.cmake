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
