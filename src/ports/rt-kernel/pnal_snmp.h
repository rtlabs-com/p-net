/*
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
*/

/**
 * @file
 * @brief SNMP server for rt-kernel
 *
 * Uses the SNMP server implementation supplied by lwip.
 * Access to Profinet stack is done through the platform-independent internal
 * API, pf_snmp.h. Supported MIBs:
 * - MIB-II,
 * - LLDP-MIB,
 * - LLDP-EXT-DOT3-MIB,
 * - LLDP-EXT-PNO-MIB.
 *
 * Note that the rt-kernel tree needs to be patched to support SNMP etc.
 * See supplied patch file.
 */

#ifndef PNAL_SNMP_H
#define PNAL_SNMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pf_snmp.h"

/**
 * SNMP server state
 */
typedef struct pnal_snmp
{
   /* The p-net stack instance.
    *
    * Used for accessing variables in the stack, such as LLDP variables.
    */
   pnet_t * net;

   pf_snmp_system_name_t sysname;
   pf_snmp_system_contact_t syscontact;
   pf_snmp_system_location_t syslocation;
   pf_snmp_system_description_t sysdescr;
} pnal_snmp_t;

/** Global variable containing SNMP server state */
extern pnal_snmp_t pnal_snmp;

#ifdef __cplusplus
}
#endif

#endif /* PNAL_SNMP_H */
