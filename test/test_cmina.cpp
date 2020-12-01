/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

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
