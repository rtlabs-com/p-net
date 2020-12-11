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
 * @brief Unit test for SNMP helper functions in pf_snmp.h.
 *
 */

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

#define LOCAL_PORT 1

class SnmpTest : public PnetIntegrationTest
{
};

/* See https://tools.ietf.org/html/rfc2578#section-7.7 clause 3,
 * for encoding of OCTET STRING field ManAddress.
 */
TEST_F (SnmpTest, SnmpGetManagementAddress)
{
   pf_snmp_management_address_t address;

   memset (&address, 0xff, sizeof (address));
   mock_lldp_data.management_address.subtype = 1;
   mock_lldp_data.management_address.value[0] = 192;
   mock_lldp_data.management_address.value[1] = 168;
   mock_lldp_data.management_address.value[2] = 1;
   mock_lldp_data.management_address.value[3] = 100;
   mock_lldp_data.management_address.len = 4;

   pf_snmp_get_management_address (net, &address);
   EXPECT_EQ (address.subtype, 1);
   EXPECT_EQ (address.value[0], 4); /* len */
   EXPECT_EQ (address.value[1], 192);
   EXPECT_EQ (address.value[2], 168);
   EXPECT_EQ (address.value[3], 1);
   EXPECT_EQ (address.value[4], 100);
   EXPECT_EQ (address.len, 5u);
}

TEST_F (SnmpTest, SnmpGetPeerManagementAddress)
{
   pf_snmp_management_address_t address;
   int error;

   memset (&address, 0xff, sizeof (address));
   mock_lldp_data.peer_management_address.subtype = 1;
   mock_lldp_data.peer_management_address.value[0] = 192;
   mock_lldp_data.peer_management_address.value[1] = 168;
   mock_lldp_data.peer_management_address.value[2] = 1;
   mock_lldp_data.peer_management_address.value[3] = 101;
   mock_lldp_data.peer_management_address.len = 4;
   mock_lldp_data.error = 0;

   error = pf_snmp_get_peer_management_address (net, LOCAL_PORT, &address);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (address.subtype, 1);
   EXPECT_EQ (address.value[0], 4); /* len */
   EXPECT_EQ (address.value[1], 192);
   EXPECT_EQ (address.value[2], 168);
   EXPECT_EQ (address.value[3], 1);
   EXPECT_EQ (address.value[4], 101);
   EXPECT_EQ (address.len, 5u);

   mock_lldp_data.error = -1;
   error = pf_snmp_get_peer_management_address (net, LOCAL_PORT, &address);
   EXPECT_EQ (error, -1);
}

/* See https://tools.ietf.org/html/rfc1906 for encoding of
 * BITS field AutoNegAdvertisedCap.
 * See https://tools.ietf.org/html/rfc2579 for encoding of
 * TruthValue fields AutoNegSupported and AutoNegEnabled.
 */
TEST_F (SnmpTest, SnmpGetLinkStatus)
{
   pf_snmp_link_status_t status;

   memset (&status, 0xff, sizeof (status));
   mock_lldp_data.link_status.auto_neg_supported = true;
   mock_lldp_data.link_status.auto_neg_enabled = true;
   mock_lldp_data.link_status.auto_neg_advertised_cap = 0xF00F;
   mock_lldp_data.link_status.oper_mau_type =
      PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX;

   pf_snmp_get_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (status.auto_neg_supported, 1); /* true */
   EXPECT_EQ (status.auto_neg_enabled, 1);   /* true */
   EXPECT_EQ (status.auto_neg_advertised_cap[0], 0xF0);
   EXPECT_EQ (status.auto_neg_advertised_cap[1], 0x0F);
   EXPECT_EQ (status.oper_mau_type, PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX);

   memset (&status, 0xff, sizeof (status));
   mock_lldp_data.link_status.auto_neg_supported = true;
   mock_lldp_data.link_status.auto_neg_enabled = false;
   mock_lldp_data.link_status.auto_neg_advertised_cap =
      BIT (5 + 0) | BIT (3 + 0) | BIT (6 + 8) | BIT (0 + 8);
   mock_lldp_data.link_status.oper_mau_type =
      PNET_MAU_COPPER_100BaseTX_HALF_DUPLEX;

   pf_snmp_get_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (status.auto_neg_supported, 1); /* true */
   EXPECT_EQ (status.auto_neg_enabled, 2);   /* false */
   EXPECT_EQ (status.auto_neg_advertised_cap[0], BIT (2) | BIT (4));
   EXPECT_EQ (status.auto_neg_advertised_cap[1], BIT (1) | BIT (7));
   EXPECT_EQ (status.oper_mau_type, PNET_MAU_COPPER_100BaseTX_HALF_DUPLEX);
}

TEST_F (SnmpTest, SnmpGetPeerLinkStatus)
{
   pf_snmp_link_status_t status;
   int error;

   memset (&status, 0xff, sizeof (status));
   mock_lldp_data.peer_link_status.auto_neg_supported = true;
   mock_lldp_data.peer_link_status.auto_neg_enabled = true;
   mock_lldp_data.peer_link_status.auto_neg_advertised_cap = 0xF00F;
   mock_lldp_data.peer_link_status.oper_mau_type =
      PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX;
   mock_lldp_data.error = 0;

   error = pf_snmp_get_peer_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (status.auto_neg_supported, 1); /* true */
   EXPECT_EQ (status.auto_neg_enabled, 1);   /* true */
   EXPECT_EQ (status.auto_neg_advertised_cap[0], 0xF0);
   EXPECT_EQ (status.auto_neg_advertised_cap[1], 0x0F);
   EXPECT_EQ (status.oper_mau_type, PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX);

   memset (&status, 0xff, sizeof (status));
   mock_lldp_data.peer_link_status.auto_neg_supported = true;
   mock_lldp_data.peer_link_status.auto_neg_enabled = false;
   mock_lldp_data.peer_link_status.auto_neg_advertised_cap =
      BIT (5 + 0) | BIT (3 + 0) | BIT (6 + 8) | BIT (0 + 8);
   mock_lldp_data.peer_link_status.oper_mau_type =
      PNET_MAU_COPPER_100BaseTX_HALF_DUPLEX;
   mock_lldp_data.error = 0;

   error = pf_snmp_get_peer_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (status.auto_neg_supported, 1); /* true */
   EXPECT_EQ (status.auto_neg_enabled, 2);   /* false */
   EXPECT_EQ (status.auto_neg_advertised_cap[0], BIT (2) | BIT (4));
   EXPECT_EQ (status.auto_neg_advertised_cap[1], BIT (1) | BIT (7));
   EXPECT_EQ (status.oper_mau_type, PNET_MAU_COPPER_100BaseTX_HALF_DUPLEX);

   mock_lldp_data.error = -1;
   error = pf_snmp_get_peer_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (error, -1);
}
