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

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

class CminaUnitTest : public PnetUnitTest
{
};

TEST_F (CminaUnitTest, CmdevCheckIsStationNameValid)
{
   EXPECT_TRUE (pf_cmina_is_stationname_valid ("", 0));
   EXPECT_TRUE (pf_cmina_is_stationname_valid ("abc", 3));
   EXPECT_TRUE (pf_cmina_is_stationname_valid ("a1.2.3.4", 8));
   EXPECT_TRUE (
      pf_cmina_is_stationname_valid ("device-1.machine-1.plant-1.vendor", 33));
   EXPECT_TRUE (pf_cmina_is_stationname_valid (
      "xn--mhle1-kva.xn--lmhle1-vxa4c.plant.com",
      40));

   EXPECT_TRUE (pf_cmina_is_stationname_valid ("port-xyz", 8));
   EXPECT_TRUE (pf_cmina_is_stationname_valid ("port-xyz-abcde", 14));
   EXPECT_TRUE (pf_cmina_is_stationname_valid (
      "abcdefghijklmnopqrstuvwxyz-abcdefghijklmnopqrstuvwxyz1234567890",
      63));
   EXPECT_TRUE (pf_cmina_is_stationname_valid (
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa."
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb."
      "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc."
      "dddddddddddddddddddddddddddddddddddddddddddddddd",
      240));

   EXPECT_FALSE (pf_cmina_is_stationname_valid (
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa."
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb."
      "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc."
      "dddddddddddddddddddddddddddddddddddddddddddddddd",
      240));
   EXPECT_FALSE (pf_cmina_is_stationname_valid (
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa."
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb."
      "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc."
      "ddddddddddddddddddddddddddddddddddddddddddddddddd",
      241));
   EXPECT_FALSE (pf_cmina_is_stationname_valid ("name_1", 6));
   EXPECT_FALSE (pf_cmina_is_stationname_valid ("name/1", 6));
   EXPECT_FALSE (pf_cmina_is_stationname_valid ("-name", 5));
   EXPECT_FALSE (pf_cmina_is_stationname_valid ("name-", 5));
   EXPECT_FALSE (pf_cmina_is_stationname_valid ("name.-name", 10));
   EXPECT_FALSE (pf_cmina_is_stationname_valid ("name.name-", 10));
   EXPECT_FALSE (pf_cmina_is_stationname_valid (
      "looooooooooooooooooooooooooooooooooooooooooooooooooooooonglabelname",
      67));

   EXPECT_FALSE (pf_cmina_is_stationname_valid ("1.2.3.4", 7));
   EXPECT_FALSE (pf_cmina_is_stationname_valid ("port-123", 8));
   EXPECT_FALSE (pf_cmina_is_stationname_valid ("port-123-98765", 14));
}

TEST_F (CminaUnitTest, CmdevCheckIsNetmaskValid)
{
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (0, 0, 0, 0)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 255)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 254)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 252)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 248)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 240)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 224)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 192)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 128)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 255, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 254, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 252, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 248, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 240, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 224, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 192, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 128, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 255, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 254, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 252, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 248, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 240, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 224, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 192, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 128, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 000, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (254, 000, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (252, 000, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (248, 000, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (240, 000, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (224, 000, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (192, 000, 000, 000)));
   EXPECT_TRUE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (128, 000, 000, 000)));

   EXPECT_FALSE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 0, 255, 255)));
   EXPECT_FALSE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (0, 255, 255, 255)));
   EXPECT_FALSE (pf_cmina_is_netmask_valid (PNAL_MAKEU32 (255, 254, 255, 0)));
}

TEST_F (CminaUnitTest, CmdevCheckIsIPaddressValid)
{
   /* 0.0.0.0 /32 Mandatory 0.0.0.0 up to 0.0.0.0
      Special case: No IPsuite assigned in conjunction with
      SubnetMask and StandardGateway set to zero */
   EXPECT_TRUE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (0, 0, 0, 0),
      PNAL_MAKEU32 (0, 0, 0, 0)));

   /* 0.0.0.0 /8 Invalid 0.0.0.0 up to 0.255.255.255
      Reserved according to IETF RFC 6890 */
   EXPECT_FALSE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (255, 0, 0, 0),
      PNAL_MAKEU32 (0, 255, 1, 1)));

   /* 127.0.0.0 /8 Invalid 127.0.0.0 up to 127.255.255.255
      Reserved according to IETF RFC 6890 loopback address */
   EXPECT_FALSE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (255, 0, 0, 0),
      PNAL_MAKEU32 (127, 0, 0, 1)));

   /* 224.0.0.0 /4 Invalid 224.0.0.0 up to 239.255.255.255
      Reserved according to IETF RFC 6890; IPv4 multicast address assignments */
   EXPECT_FALSE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (0xF0, 0, 0, 0),
      PNAL_MAKEU32 (224, 0, 0, 34)));

   /* 240.0.0.0 /4 Invalid 240.0.0.0 up to 255.255.255.255
      Reserved according to IETF RFC 6890; reserved for future addressing */
   EXPECT_FALSE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (0xF0, 0, 0, 0),
      PNAL_MAKEU32 (240, 0, 0, 34)));

   /*  Invalid — Subnet part of the IPAddress is “0” */
   EXPECT_FALSE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (255, 255, 0, 0),
      PNAL_MAKEU32 (0, 0, 1, 10)));

   /* Invalid Host part of the IPAddress is a series of consecutive “1”
     (subnet broadcast address)
     IPAddress may be accepted but should be invalid.*/
   EXPECT_FALSE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (255, 255, 0, 0),
      PNAL_MAKEU32 (192, 168, 255, 255)));

   /* Invalid — Host part of the IPAddress is a series of consecutive “0”
      (subnet address)
      IPAddress may be accepted but should be invalid. */
   EXPECT_FALSE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (255, 255, 0, 0),
      PNAL_MAKEU32 (192, 168, 0, 0)));

   /* Other Mandatory — IP address assigned */
   EXPECT_TRUE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (255, 255, 0, 0),
      PNAL_MAKEU32 (192, 168, 1, 1)));

   EXPECT_TRUE (pf_cmina_is_ipaddress_valid (
      PNAL_MAKEU32 (255, 255, 255, 0),
      PNAL_MAKEU32 (10, 10, 0, 35)));
}

TEST_F (CminaUnitTest, CmdevCheckIsGatewayValid)
{
   EXPECT_TRUE (pf_cmina_is_gateway_valid (
      PNAL_MAKEU32 (192, 168, 1, 4),
      PNAL_MAKEU32 (255, 255, 255, 0),
      PNAL_MAKEU32 (192, 168, 1, 1)));

   EXPECT_TRUE (pf_cmina_is_gateway_valid (
      PNAL_MAKEU32 (192, 168, 1, 4),
      PNAL_MAKEU32 (255, 255, 255, 0),
      PNAL_MAKEU32 (0, 0, 0, 0)));

   EXPECT_FALSE (pf_cmina_is_gateway_valid (
      PNAL_MAKEU32 (192, 168, 1, 4),
      PNAL_MAKEU32 (255, 255, 255, 0),
      PNAL_MAKEU32 (192, 169, 1, 1)));

   EXPECT_FALSE (pf_cmina_is_gateway_valid (
      PNAL_MAKEU32 (192, 168, 1, 4),
      PNAL_MAKEU32 (255, 255, 255, 0),
      PNAL_MAKEU32 (192, 168, 0, 1)));
}
