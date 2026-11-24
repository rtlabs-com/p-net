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

TEST_F (SnmpTest, SnmpGetLocationGivenError)
{
   pf_snmp_system_location_t actual;

   mock_file_data.is_load_failing = true;
   memset (&actual, 0xff, sizeof (actual));
   pf_snmp_data_init (net);

   pf_snmp_get_system_location (net, &actual);

   EXPECT_STREQ (actual.string, "");
   EXPECT_EQ (strlen (actual.string), 0u);
}

TEST_F (SnmpTest, SnmpSetLocationGivenLargeString)
{
   const pf_snmp_system_location_t written = {"1234567890123456789012345"};
   int error;

   error = pf_snmp_set_system_location (net, &written);

   EXPECT_EQ (error, 0);
   EXPECT_STREQ (mock_file_data.filename, PF_FILENAME_SNMP_SYSLOCATION);
   EXPECT_EQ (mock_file_data.size, sizeof (written));
   EXPECT_STREQ ((char *)mock_file_data.object, written.string);
}

TEST_F (SnmpTest, SnmpSetLocationGivenSmallString)
{
   const pf_snmp_system_location_t written = {"small"};
   int error;

   error = pf_snmp_set_system_location (net, &written);

   EXPECT_EQ (error, 0);
   EXPECT_STREQ (mock_file_data.filename, PF_FILENAME_SNMP_SYSLOCATION);
   EXPECT_EQ (mock_file_data.size, sizeof (written));

   EXPECT_STREQ ((char *)mock_file_data.object, written.string);
}

TEST_F (SnmpTest, SnmpSetLocationGivenError)
{
   const pf_snmp_system_location_t written = {"1234567890123456789012345"};
   uint8_t file_data[sizeof (mock_file_data.object)];
   memcpy (file_data, mock_file_data.object, sizeof (mock_file_data.object));

   mock_file_data.is_save_failing = true;

   pf_snmp_set_system_location (net, &written);
   EXPECT_EQ (
      memcmp (
         mock_file_data.object,
         file_data,
         sizeof (mock_file_data.object)), 0);
}

#if PNET_OPTION_SECURITY_LEVEL_1

TEST_F (SnmpTest, SnmpAdjustValidateCommunityString)
{
   int error;

   error = pf_snmp_validate_community_name ("private");
   EXPECT_EQ (error, 0);

   error = pf_snmp_validate_community_name ("public");
   EXPECT_EQ (error, 0);

   /* Empty string is valid */
   error = pf_snmp_validate_community_name ("");
   EXPECT_EQ (error, 0);

   /* NULL is is invalid */
   error = pf_snmp_validate_community_name (NULL);
   EXPECT_EQ (error, -1);

   /* 240 characters is valid */
   error = pf_snmp_validate_community_name (
      "123456789012345678901234567890123456789012345678901234567890123456789012"
      "345678901234567890123456789012345678901234567890123456789012345678901234"
      "567890123456789012345678901234567890123456789012345678901234567890123456"
      "789012345678901234567890");
   EXPECT_EQ (error, 0);

   /* 241 characters is invalid */
   error = pf_snmp_validate_community_name (
      "123456789012345678901234567890123456789012345678901234567890123456789012"
      "345678901234567890123456789012345678901234567890123456789012345678901234"
      "567890123456789012345678901234567890123456789012345678901234567890123456"
      "7890123456789012345678901");
   EXPECT_EQ (error, -1);

   error = pf_snmp_validate_community_name ("test*test");
   EXPECT_EQ (error, -1);

   error = pf_snmp_validate_community_name (".public");
   EXPECT_EQ (error, -1);

   error = pf_snmp_validate_community_name ("public.");
   EXPECT_EQ (error, -1);

   error = pf_snmp_validate_community_name ("pub.lic");
   EXPECT_EQ (error, 0);

   error = pf_snmp_validate_community_name ("-public");
   EXPECT_EQ (error, -1);

   error = pf_snmp_validate_community_name ("public-");
   EXPECT_EQ (error, -1);

   error = pf_snmp_validate_community_name ("pub-lic");
   EXPECT_EQ (error, 0);
}

TEST_F (SnmpTest, SnmpAdjustValidateConfig)
{
   int error;
   const pf_snmp_config_t disable = {
      .control_mode = PNAL_SNMP_CONTROL_MODE_DISABLED,
      .ro_name = {""},
      .rw_name = {""},
      .snmp_adjust_active = true};

   error = pf_snmp_validate_config (&disable);
   EXPECT_EQ (error, 0);

   const pf_snmp_config_t readonly = {
      .control_mode = PNAL_SNMP_CONTROL_MODE_READONLY,
      .ro_name = {"readonly"},
      .rw_name = {""},
      .snmp_adjust_active = true};

   error = pf_snmp_validate_config (&readonly);
   EXPECT_EQ (error, 0);

   const pf_snmp_config_t readwrite = {
      .control_mode = PNAL_SNMP_CONTROL_MODE_READWRITE,
      .ro_name = {"read"},
      .rw_name = {"write"},
      .snmp_adjust_active = true};

   error = pf_snmp_validate_config (&readwrite);
   EXPECT_EQ (error, 0);

   /* Empty community string shall be used when setting disabled */
   const pf_snmp_config_t disable_invalid = {
      .control_mode = PNAL_SNMP_CONTROL_MODE_DISABLED,
      .ro_name = {"public"},
      .rw_name = {"private"},
      .snmp_adjust_active = true};

   error = pf_snmp_validate_config (&disable_invalid);
   EXPECT_EQ (error, -1);

   /* Empty write community string shall be used when setting readonly */
   const pf_snmp_config_t readonly_invalid = {
      .control_mode = PNAL_SNMP_CONTROL_MODE_READONLY,
      .ro_name = {"public"},
      .rw_name = {"private"},
      .snmp_adjust_active = true};

   error = pf_snmp_validate_config (&readonly_invalid);
   EXPECT_EQ (error, -1);

   /* Invalid mode */
   const pf_snmp_config_t mode_invalid = {
      .control_mode = (pnal_snmp_control_mode_t)3,
      .ro_name = {"public"},
      .rw_name = {"private"},
      .snmp_adjust_active = true};

   error = pf_snmp_validate_config (&mode_invalid);
   EXPECT_EQ (error, -1);
}

TEST_F (SnmpTest, SnmpAdjust)
{
   int status;
   pnet_result_t res = {{0, 0, 0, 0}, 0, 0};
   pf_iod_write_request_t write_req = {
      .sequence_number = 1,
      .ar_uuid = {0, 0, 0, 0, 0, {0}},
      .api = 0,
      .slot_number = 0,
      .subslot_number = 0x8000,
      .padding = {0},
      .index = 0x8200,
      .record_data_length = 32,
      .rw_padding = {0}};

   /* Valid configuration */
   /* mode = readwrite, ro = "readonly1", rw = "readwrite1" */
   uint8_t snmp_adjust_1[] = {0x03, 0x00, 0x00, 0x1c, 0x01, 0x00, 0x00, 0x02,
                              0x09, 0x72, 0x65, 0x61, 0x64, 0x6f, 0x6e, 0x6c,
                              0x79, 0x31, 0x0a, 0x72, 0x65, 0x61, 0x64, 0x77,
                              0x72, 0x69, 0x74, 0x65, 0x31, 0x00, 0x00, 0x00};

   /* P-Net frame parser will consume block header before calling handler.
    * This gives offset 6 in code below
    */
   status = pf_cim_snmp_adjust (
      net,
      &write_req,
      &snmp_adjust_1[6],
      sizeof (snmp_adjust_1) - 6,
      &res);

   EXPECT_EQ (status, 0);
   EXPECT_STREQ (mock_snmp_config.ro_name.string, "readonly1");
   EXPECT_STREQ (mock_snmp_config.rw_name.string, "readwrite1");
   EXPECT_EQ (mock_snmp_config.control_mode, PNAL_SNMP_CONTROL_MODE_READWRITE);

   /* Invalid frame - wrong control mode byte [7] 0x02 => 0x03 */
   /* Additionally modify strings so they are different from the valid case */
   uint8_t snmp_adjust_2[] = {0x03, 0x00, 0x00, 0x1c, 0x01, 0x00, 0x00, 0x03,
                              0x09, 'R',  0x65, 0x61, 0x64, 0x6f, 0x6e, 0x6c,
                              0x79, 0x31, 0x0a, 'T',  0x65, 0x61, 0x64, 0x77,
                              0x72, 0x69, 0x74, 0x65, 0x31, 0x00, 0x00, 0x00};

   status = pf_cim_snmp_adjust (
      net,
      &write_req,
      &snmp_adjust_2[6],
      sizeof (snmp_adjust_2) - 6,
      &res);

   EXPECT_EQ (status, -1);
   /*Check that the configuration has not been modified*/
   EXPECT_STREQ (mock_snmp_config.ro_name.string, "readonly1");
   EXPECT_STREQ (mock_snmp_config.rw_name.string, "readwrite1");
   EXPECT_EQ (mock_snmp_config.control_mode, PNAL_SNMP_CONTROL_MODE_READWRITE);

   /* Invalid frame - community string length = 0xFF */
   uint8_t snmp_adjust_3[] = {0x03, 0x00, 0x00, 0x1c, 0x01, 0x00, 0x00, 0x02,
                              0xFF, 0x72, 0x65, 0x61, 0x64, 0x6f, 0x6e, 0x6c,
                              0x79, 0x31, 0x0a, 0x72, 0x65, 0x61, 0x64, 0x77,
                              0x72, 0x69, 0x74, 0x65, 0x31, 0x00, 0x00, 0x00};

   status = pf_cim_snmp_adjust (
      net,
      &write_req,
      &snmp_adjust_3[6],
      sizeof (snmp_adjust_3) - 6,
      &res);

   EXPECT_EQ (status, -1);
   /*Check that the configuration has not been modified*/
   EXPECT_STREQ (mock_snmp_config.ro_name.string, "readonly1");
   EXPECT_STREQ (mock_snmp_config.rw_name.string, "readwrite1");
   EXPECT_EQ (mock_snmp_config.control_mode, PNAL_SNMP_CONTROL_MODE_READWRITE);
}

#endif /* PNET_OPTION_SECURITY_LEVEL_1 */
