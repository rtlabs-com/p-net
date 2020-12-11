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
 * @brief Helper functions for CMRPC
 *
 * Located in a separate file to enabling mocking during tests.
 *
 */

#include "pf_includes.h"

/**
 * @internal
 * Generate an UUID, inspired by version 1, variant 1
 *
 * See RFC 4122
 *
 * @param timestamp        In:   Some kind of timestamp
 * @param session_number   In:   Session number
 * @param mac_address      In:   MAC address
 * @param p_uuid           Out:  The generated UUID
 */
void pf_generate_uuid (
   uint32_t timestamp,
   uint32_t session_number,
   pnet_ethaddr_t mac_address,
   pf_uuid_t * p_uuid)
{
   p_uuid->data1 = timestamp;
   p_uuid->data2 = session_number >> 16;
   p_uuid->data3 = (session_number >> 8) & 0x00ff;
   p_uuid->data3 |= 1 << 12; /* UUID version 1 */
   p_uuid->data4[0] = 0x80;  /* UUID variant 1 */
   p_uuid->data4[1] = session_number & 0xff;
   p_uuid->data4[2] = mac_address.addr[0];
   p_uuid->data4[3] = mac_address.addr[1];
   p_uuid->data4[4] = mac_address.addr[2];
   p_uuid->data4[5] = mac_address.addr[3];
   p_uuid->data4[6] = mac_address.addr[4];
   p_uuid->data4[7] = mac_address.addr[5];
}
