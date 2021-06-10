/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

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
   mock_lldp_data.link_status.is_autonegotiation_supported = true;
   mock_lldp_data.link_status.is_autonegotiation_enabled = true;
   mock_lldp_data.link_status.autonegotiation_advertised_capabilities = 0xF00F;
   mock_lldp_data.link_status.operational_mau_type =
      PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX;

   pf_snmp_get_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (status.auto_neg_supported, 1); /* true */
   EXPECT_EQ (status.auto_neg_enabled, 1);   /* true */
   EXPECT_EQ (status.auto_neg_advertised_cap[0], 0xF0);
   EXPECT_EQ (status.auto_neg_advertised_cap[1], 0x0F);
   EXPECT_EQ (status.oper_mau_type, PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX);

   memset (&status, 0xff, sizeof (status));
   mock_lldp_data.link_status.is_autonegotiation_supported = true;
   mock_lldp_data.link_status.is_autonegotiation_enabled = false;
   mock_lldp_data.link_status.autonegotiation_advertised_capabilities =
      BIT (5 + 0) | BIT (3 + 0) | BIT (6 + 8) | BIT (0 + 8);
   mock_lldp_data.link_status.operational_mau_type =
      PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX;

   pf_snmp_get_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (status.auto_neg_supported, 1); /* true */
   EXPECT_EQ (status.auto_neg_enabled, 2);   /* false */
   EXPECT_EQ (status.auto_neg_advertised_cap[0], BIT (2) | BIT (4));
   EXPECT_EQ (status.auto_neg_advertised_cap[1], BIT (1) | BIT (7));
   EXPECT_EQ (status.oper_mau_type, PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX);
}

TEST_F (SnmpTest, SnmpGetPeerLinkStatus)
{
   pf_snmp_link_status_t status;
   int error;

   memset (&status, 0xff, sizeof (status));
   mock_lldp_data.peer_link_status.is_autonegotiation_supported = true;
   mock_lldp_data.peer_link_status.is_autonegotiation_enabled = true;
   mock_lldp_data.peer_link_status.autonegotiation_advertised_capabilities =
      0xF00F;
   mock_lldp_data.peer_link_status.operational_mau_type =
      PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX;
   mock_lldp_data.error = 0;

   error = pf_snmp_get_peer_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (status.auto_neg_supported, 1); /* true */
   EXPECT_EQ (status.auto_neg_enabled, 1);   /* true */
   EXPECT_EQ (status.auto_neg_advertised_cap[0], 0xF0);
   EXPECT_EQ (status.auto_neg_advertised_cap[1], 0x0F);
   EXPECT_EQ (status.oper_mau_type, PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX);

   memset (&status, 0xff, sizeof (status));
   mock_lldp_data.peer_link_status.is_autonegotiation_supported = true;
   mock_lldp_data.peer_link_status.is_autonegotiation_enabled = false;
   mock_lldp_data.peer_link_status.autonegotiation_advertised_capabilities =
      BIT (5 + 0) | BIT (3 + 0) | BIT (6 + 8) | BIT (0 + 8);
   mock_lldp_data.peer_link_status.operational_mau_type =
      PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX;
   mock_lldp_data.error = 0;

   error = pf_snmp_get_peer_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (status.auto_neg_supported, 1); /* true */
   EXPECT_EQ (status.auto_neg_enabled, 2);   /* false */
   EXPECT_EQ (status.auto_neg_advertised_cap[0], BIT (2) | BIT (4));
   EXPECT_EQ (status.auto_neg_advertised_cap[1], BIT (1) | BIT (7));
   EXPECT_EQ (status.oper_mau_type, PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX);

   mock_lldp_data.error = -1;
   error = pf_snmp_get_peer_link_status (net, LOCAL_PORT, &status);
   EXPECT_EQ (error, -1);
}

TEST_F (SnmpTest, SnmpGetLocationGivenLargeString)
{
   const pf_snmp_system_location_t stored = {"1234567890123456789012 large"};
   const char * expected = stored.string;
   pf_snmp_system_location_t actual;

   memset (&actual, 0xff, sizeof (actual));
   mock_pf_file_save (NULL, PF_FILENAME_SNMP_SYSLOCATION, &stored, sizeof (stored));
   pf_snmp_data_init (net);

   pf_snmp_get_system_location (net, &actual);

   EXPECT_STREQ (actual.string, expected);
}

TEST_F (SnmpTest, SnmpGetLocationGivenSmallString)
{
   const pf_snmp_system_location_t stored = {"small"};
   const char * expected = stored.string;
   pf_snmp_system_location_t actual;

   memset (&actual, 0xcd, sizeof (actual));
   mock_pf_file_save (NULL, PF_FILENAME_SNMP_SYSLOCATION, &stored, sizeof (stored));
   pf_snmp_data_init (net);

   pf_snmp_get_system_location (net, &actual);

   EXPECT_STREQ (actual.string, expected);
}

/* Note that the 22 bytes device location in I&M1 will be used if loading of
 * sysLocation file fails (e.g. if does not exit).
 */
TEST_F (SnmpTest, SnmpGetLocationGivenError)
{
   const char expected[] = "IM_Tag_Location in I&M";
   pf_snmp_system_location_t actual;

   mock_file_data.is_load_failing = true;
   memset (&actual, 0xff, sizeof (actual));
   mock_pf_fspm_save_im_location (net, expected);
   pf_snmp_data_init (net);

   pf_snmp_get_system_location (net, &actual);

   EXPECT_STREQ (actual.string, expected);
   EXPECT_EQ (strlen (actual.string), 22u);
}

/* Note that the first 22 bytes is also written to device location in I&M1 */
TEST_F (SnmpTest, SnmpSetLocationGivenLargeString)
{
   const pf_snmp_system_location_t written = {"1234567890123456789012345"};
   int error;

   error = pf_snmp_set_system_location (net, &written);

   EXPECT_EQ (error, 0);
   EXPECT_STREQ (mock_file_data.filename, PF_FILENAME_SNMP_SYSLOCATION);
   EXPECT_EQ (mock_file_data.size, sizeof (written));
   EXPECT_EQ (memcmp (mock_file_data.object, &written, sizeof (written)), 0);
   EXPECT_STREQ (mock_fspm_data.im_location, "1234567890123456789012");
}

/* Note that the first 22 bytes is also written to device location in I&M1 */
TEST_F (SnmpTest, SnmpSetLocationGivenSmallString)
{
   const pf_snmp_system_location_t written = {"small"};
   int error;

   error = pf_snmp_set_system_location (net, &written);

   EXPECT_EQ (error, 0);
   EXPECT_STREQ (mock_file_data.filename, PF_FILENAME_SNMP_SYSLOCATION);
   EXPECT_EQ (mock_file_data.size, sizeof (written));
   EXPECT_EQ (memcmp (mock_file_data.object, &written, sizeof (written)), 0);
   EXPECT_STREQ (mock_fspm_data.im_location, "small                 ");
}

/* Note that the first 22 bytes is also written to device location in I&M1 */
TEST_F (SnmpTest, SnmpSetLocationGivenError)
{
   const pf_snmp_system_location_t written = {"1234567890123456789012345"};
   int error;

   mock_file_data.is_save_failing = true;

   error = pf_snmp_set_system_location (net, &written);

   EXPECT_EQ (error, -1);
   EXPECT_STREQ (mock_fspm_data.im_location, "1234567890123456789012");
}
