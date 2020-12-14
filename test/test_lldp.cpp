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

/**
 * @file
 * @brief Integration test that a LLDP frame is sent at init.
 *
 */

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

#define LOCAL_PORT 1

#define MAX_LLDP_PAYLOAD_SIZE 1500

#define LLDP_TYPE_END                0
#define LLDP_TYPE_CHASSIS_ID         1
#define LLDP_TYPE_PORT_ID            2
#define LLDP_TYPE_TTL                3
#define LLDP_TYPE_PORT_DESCRIPTION   4
#define LLDP_TYPE_MANAGEMENT_ADDRESS 8
#define LLDP_TYPE_ORG_SPEC           127

#define TLV_BYTE_0(len, type) (((len) >> 8) | ((type) << 1))
#define TLV_BYTE_1(len)       (((len) >> 0) & 0xff)

static const char * chassis_id_test_sample_1 =
   "\x53\x43\x41\x4c\x41\x4e\x43\x45\x20\x58\x2d\x32\x30\x30\x20"
   "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x36\x47\x4b\x35\x20"
   "\x32\x30\x34\x2d\x30\x42\x41\x30\x30\x2d\x32\x42\x41\x33\x20\x20"
   "\x53\x56\x50\x4c\x36\x31\x34\x37\x39\x32\x30\x20\x20\x20\x20\x20"
   "\x20\x20\x20\x20\x20\x39\x20\x56\x20\x20\x35\x20\x20\x34\x20\x20"
   "\x32";

static const char * port_id_test_sample_1 =
   "\x70\x6f\x72\x74\x2d\x30\x30\x33\x2e\x61\x62\x63\x64\x65\x66\x67"
   "\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77"
   "\x78\x79\x7a\x2d\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c"
   "\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x31\x32"
   "\x33\x34\x35\x36\x37\x38\x39\x30\x2e\x61\x62\x63\x64\x65\x66\x67"
   "\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77"
   "\x78\x79\x7a\x2d\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c"
   "\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x31\x32"
   "\x2e\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
   "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x2d\x61\x62\x63\x64"
   "\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74"
   "\x75\x76\x77\x78\x79\x7a\x31\x32\x33\x2e\x61\x62\x63\x64\x65\x66"
   "\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76"
   "\x77\x78\x79\x7a\x2d\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b"
   "\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x31"
   "\x32\x33\x34\x35\x36\x37\x38\x39\x30";

static pf_lldp_peer_info_t fake_peer_info (void)
{
   pf_lldp_peer_info_t peer;

   memset (&peer, 0, sizeof (peer));

   snprintf (
      peer.chassis_id.string,
      sizeof (peer.chassis_id.string),
      "%s",
      "Chassis ID received from remote device");
   peer.chassis_id.len = strlen (peer.chassis_id.string);
   peer.chassis_id.subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   peer.chassis_id.is_valid = true;

   snprintf (
      peer.port_id.string,
      sizeof (peer.port_id.string),
      "%s",
      "Port ID received from remote device");
   peer.port_id.len = strlen (peer.port_id.string);
   peer.port_id.subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   peer.port_id.is_valid = true;

   snprintf (
      peer.port_description.string,
      sizeof (peer.port_description.string),
      "%s",
      "Port Description");
   peer.port_description.len = strlen (peer.port_description.string);
   peer.port_description.is_valid = true;

   peer.management_address.subtype = 1; /* IPv4 */
   peer.management_address.value[0] = 192;
   peer.management_address.value[1] = 168;
   peer.management_address.value[2] = 10;
   peer.management_address.value[3] = 102;
   peer.management_address.len = 4;
   peer.management_address.interface_number.subtype = 2; /* ifIndex */
   peer.management_address.interface_number.value = 1;
   peer.management_address.is_valid = true;

   peer.phy_config.is_autonegotiation_supported = true;
   peer.phy_config.is_autonegotiation_enabled = true;
   peer.phy_config.autonegotiation_advertised_capabilities =
      PNET_LLDP_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
      PNET_LLDP_AUTONEG_CAP_100BaseTX_FULL_DUPLEX;
   peer.phy_config.operational_mau_type = PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX;
   peer.phy_config.is_valid = true;

   peer.port_delay.cable_delay_local = 1;
   peer.port_delay.rx_delay_local = 2;
   peer.port_delay.tx_delay_local = 3;
   peer.port_delay.rx_delay_remote = 4;
   peer.port_delay.tx_delay_remote = 5;
   peer.port_delay.is_valid = true;

   return peer;
}

static size_t construct_packet_chassis_id (
   uint8_t packet[],
   const pf_lldp_chassis_id_t * id)
{
   size_t size = 0;

   if (id->is_valid)
   {
      packet[0] = TLV_BYTE_0 (1 + id->len, LLDP_TYPE_CHASSIS_ID);
      packet[1] = TLV_BYTE_1 (1 + id->len);
      packet[2] = id->subtype;
      memcpy (&packet[3], id->string, id->len);
      size = 3 + id->len;
   }

   return size;
}

static size_t construct_packet_port_id (
   uint8_t packet[],
   const pf_lldp_port_id_t * id)
{
   size_t size = 0;

   if (id->is_valid)
   {
      packet[0] = TLV_BYTE_0 (1 + id->len, LLDP_TYPE_PORT_ID);
      packet[1] = TLV_BYTE_1 (1 + id->len);
      packet[2] = id->subtype;
      memcpy (&packet[3], id->string, id->len);
      size = 3 + id->len;
   }

   return size;
}

static size_t construct_packet_port_description (
   uint8_t packet[],
   const pf_lldp_port_description_t * description)
{
   size_t size = 0;

   if (description->is_valid)
   {
      packet[0] = TLV_BYTE_0 (description->len, LLDP_TYPE_PORT_DESCRIPTION);
      packet[1] = TLV_BYTE_1 (description->len);
      memcpy (&packet[2], description->string, description->len);
      size = 2 + description->len;
   }

   return size;
}

static size_t construct_packet_management_address (
   uint8_t packet[],
   const pf_lldp_management_address_t * address)
{
   size_t size = 0;

   if (address->is_valid)
   {
      uint32_t big_endian;

      packet[0] =
         TLV_BYTE_0 (2 + address->len + 6, LLDP_TYPE_MANAGEMENT_ADDRESS);
      packet[1] = TLV_BYTE_1 (2 + address->len + 6);
      packet[2] = 1 + address->len;
      packet[3] = address->subtype;
      memcpy (&packet[4], address->value, address->len);
      size += 4 + address->len;

      big_endian = CC_TO_BE32 (address->interface_number.value);
      packet[0 + size] = address->interface_number.subtype;
      memcpy (&packet[1 + size], &big_endian, 4);
      packet[5 + size] = 0; /* OID string length */
      size += 6;
   }

   return size;
}

static size_t construct_packet_link_status (
   uint8_t packet[],
   const pf_lldp_link_status_t * status)
{
   size_t size = 0;

   if (status->is_valid)
   {
      packet[0] = TLV_BYTE_0 (9, LLDP_TYPE_ORG_SPEC);
      packet[1] = TLV_BYTE_1 (9);
      packet[2] = 0x00; /* 802.3 OUI */
      packet[3] = 0x12; /* 802.3 OUI */
      packet[4] = 0x0F; /* 802.3 OUI */
      packet[5] = 1;    /* subtype */
      packet[6] = ((uint8_t)status->is_autonegotiation_supported << 0) |
                  ((uint8_t)status->is_autonegotiation_enabled << 1);
      packet[7] = status->autonegotiation_advertised_capabilities >> 8;
      packet[8] = status->autonegotiation_advertised_capabilities >> 0;
      packet[9] = status->operational_mau_type >> 8;
      packet[10] = status->operational_mau_type >> 0;
      size = 11;
   }

   return size;
}

static size_t construct_packet_signal_delay (
   uint8_t packet[],
   const pf_lldp_signal_delay_t * delay)
{
   size_t size = 0;
   uint32_t bigendian;

   if (delay->is_valid)
   {
      packet[0] = TLV_BYTE_0 (24, LLDP_TYPE_ORG_SPEC);
      packet[1] = TLV_BYTE_1 (24);
      packet[2] = 0x00; /* Profinet OUI */
      packet[3] = 0x0E; /* Profinet OUI */
      packet[4] = 0xCF; /* Profinet OUI */
      packet[5] = 1;    /* subtype */
      bigendian = CC_TO_BE32 (delay->rx_delay_local);
      memcpy (&packet[6], &bigendian, 4);
      bigendian = CC_TO_BE32 (delay->rx_delay_remote);
      memcpy (&packet[10], &bigendian, 4);
      bigendian = CC_TO_BE32 (delay->tx_delay_local);
      memcpy (&packet[14], &bigendian, 4);
      bigendian = CC_TO_BE32 (delay->tx_delay_remote);
      memcpy (&packet[18], &bigendian, 4);
      bigendian = CC_TO_BE32 (delay->cable_delay_local);
      memcpy (&packet[22], &bigendian, 4);
      size = 26;
   }

   return size;
}

static size_t construct_packet_end_marker (uint8_t packet[])
{
   size_t size;

   packet[0] = TLV_BYTE_0 (0, LLDP_TYPE_END);
   packet[1] = TLV_BYTE_1 (0);
   size = 2;

   return size;
}

static size_t construct_packet (
   uint8_t packet[MAX_LLDP_PAYLOAD_SIZE],
   const pf_lldp_peer_info_t * data)
{
   memset (packet, 0xff, MAX_LLDP_PAYLOAD_SIZE);
   size_t size = 0;

   size += construct_packet_chassis_id (&packet[size], &data->chassis_id);
   size += construct_packet_port_id (&packet[size], &data->port_id);
   size += construct_packet_port_description (
      &packet[size],
      &data->port_description);
   size += construct_packet_management_address (
      &packet[size],
      &data->management_address);
   size += construct_packet_link_status (&packet[size], &data->phy_config);
   size += construct_packet_signal_delay (&packet[size], &data->port_delay);
   size += construct_packet_end_marker (&packet[size]);

   return size;
}

class LldpTest : public PnetIntegrationTestBase
{
 protected:
   virtual void SetUp() override
   {
      mock_init();
      cfg_init();
      appdata_init();
      available_modules_and_submodules_init();

      callcounter_reset();

      pnet_init_only (
         net,
         TEST_INTERFACE_NAME,
         TICK_INTERVAL_US,
         &pnet_default_cfg);

      /* Do not clear mock or callcounters here - we need to verify send at init
       * from LLDP */
   };
};

TEST_F (LldpTest, LldpInitTest)
{
   /* Verify that LLDP has sent a frame at init */
   EXPECT_EQ (appdata.call_counters.state_calls, 0);
   EXPECT_EQ (appdata.call_counters.connect_calls, 0);
   EXPECT_EQ (appdata.call_counters.release_calls, 0);
   EXPECT_EQ (appdata.call_counters.dcontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.ccontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.read_calls, 0);
   EXPECT_EQ (appdata.call_counters.write_calls, 0);
   EXPECT_EQ (mock_os_data.eth_send_count, 1);
}

TEST_F (LldpTest, LldpGenerateAliasName)
{
   char alias[256];
   int err;

   err = pf_lldp_generate_alias_name (NULL, "dut", alias, 96);
   EXPECT_EQ (err, -1);

   err = pf_lldp_generate_alias_name ("port-001", NULL, alias, 96);
   EXPECT_EQ (err, -1);

   err = pf_lldp_generate_alias_name ("port-001", "dut", NULL, 96);
   EXPECT_EQ (err, -1);

   /* Test legacy PN v2.2
    * PortId string does not include a "."
    * Alias = PortId.StationName
    */
   err = pf_lldp_generate_alias_name ("port-001", "dut", alias, 13);
   EXPECT_EQ (err, 0);
   EXPECT_EQ (strcmp (alias, "port-001.dut"), 0);
   memset (alias, 0, sizeof (alias));

   err = pf_lldp_generate_alias_name ("port-001", "dut", alias, 12);
   EXPECT_EQ (err, -1);

   /* Test PN v2.3+
    * PortId string does include a "."
    * Alias = PortId
    */
   err = pf_lldp_generate_alias_name ("port-001.dut", "tester", alias, 13);
   EXPECT_EQ (err, 0);
   EXPECT_EQ (strcmp (alias, "port-001.dut"), 0);
   memset (alias, 0, sizeof (alias));

   err = pf_lldp_generate_alias_name ("port-001.dut", "tester", alias, 12);
   EXPECT_EQ (err, -1);

   /* Test data from rt-tester v2.3 style */
   err = pf_lldp_generate_alias_name (
      port_id_test_sample_1,
      chassis_id_test_sample_1,
      alias,
      256);
   EXPECT_EQ (err, 0);
   EXPECT_EQ (strcmp (alias, port_id_test_sample_1), 0);
   memset (alias, 0, sizeof (alias));
}

TEST_F (LldpTest, LldpParsePacket)
{
   uint8_t packet[MAX_LLDP_PAYLOAD_SIZE];
   int error;
   size_t size;
   pf_lldp_peer_info_t actual;
   const pf_lldp_peer_info_t expected = fake_peer_info();

   size = construct_packet (packet, &expected);
   memset (&actual, 0xff, sizeof (actual));

   error = pf_lldp_parse_packet (packet, size, &actual);

   EXPECT_EQ (error, 0);

   EXPECT_EQ (actual.chassis_id.is_valid, expected.chassis_id.is_valid);
   EXPECT_EQ (actual.chassis_id.subtype, expected.chassis_id.subtype);
   EXPECT_EQ (actual.chassis_id.len, expected.chassis_id.len);
   EXPECT_STREQ (actual.chassis_id.string, expected.chassis_id.string);

   EXPECT_EQ (actual.port_id.is_valid, expected.port_id.is_valid);
   EXPECT_EQ (actual.port_id.subtype, expected.port_id.subtype);
   EXPECT_EQ (actual.port_id.len, expected.port_id.len);
   EXPECT_STREQ (actual.port_id.string, expected.port_id.string);

   EXPECT_EQ (
      actual.port_description.is_valid,
      expected.port_description.is_valid);
   EXPECT_EQ (actual.port_description.len, expected.port_description.len);
   EXPECT_STREQ (
      actual.port_description.string,
      expected.port_description.string);

   EXPECT_EQ (
      actual.management_address.is_valid,
      expected.management_address.is_valid);
   EXPECT_EQ (
      actual.management_address.subtype,
      expected.management_address.subtype);
   EXPECT_EQ (actual.management_address.len, expected.management_address.len);
   EXPECT_EQ (
      memcmp (
         actual.management_address.value,
         expected.management_address.value,
         4),
      0);
   EXPECT_EQ (
      actual.management_address.interface_number.subtype,
      expected.management_address.interface_number.subtype);
   EXPECT_EQ (
      actual.management_address.interface_number.value,
      expected.management_address.interface_number.value);

   EXPECT_EQ (actual.phy_config.is_valid, expected.phy_config.is_valid);
   EXPECT_EQ (
      actual.phy_config.is_autonegotiation_supported,
      expected.phy_config.is_autonegotiation_supported);
   EXPECT_EQ (
      actual.phy_config.is_autonegotiation_enabled,
      expected.phy_config.is_autonegotiation_enabled);
   EXPECT_EQ (
      actual.phy_config.autonegotiation_advertised_capabilities,
      expected.phy_config.autonegotiation_advertised_capabilities);
   EXPECT_EQ (
      actual.phy_config.operational_mau_type,
      expected.phy_config.operational_mau_type);

   EXPECT_EQ (actual.port_delay.is_valid, expected.port_delay.is_valid);
   EXPECT_EQ (
      actual.port_delay.rx_delay_local,
      expected.port_delay.rx_delay_local);
   EXPECT_EQ (
      actual.port_delay.rx_delay_remote,
      expected.port_delay.rx_delay_remote);
   EXPECT_EQ (
      actual.port_delay.tx_delay_local,
      expected.port_delay.tx_delay_local);
   EXPECT_EQ (
      actual.port_delay.tx_delay_remote,
      expected.port_delay.tx_delay_remote);
   EXPECT_EQ (
      actual.port_delay.cable_delay_local,
      expected.port_delay.cable_delay_local);

   /* Finally check that everything is as expected.
    * While this is the only check really needed, the checks above will make
    * it easier to find out exactly what went wrong.
    */
   EXPECT_EQ (memcmp (&actual, &expected, sizeof (expected)), 0);
}

TEST_F (LldpTest, LldpParsePacketWithTooBigChassisId)
{
   uint8_t packet[MAX_LLDP_PAYLOAD_SIZE];
   int error;
   size_t size;
   pf_lldp_peer_info_t actual;
   pf_lldp_peer_info_t peer = fake_peer_info();
   const size_t max_len = sizeof (peer.chassis_id.string) - 1;

   peer.chassis_id.len = max_len + 1;
   size = construct_packet (packet, &peer);
   memset (&actual, 0xff, sizeof (actual));

   error = pf_lldp_parse_packet (packet, size, &actual);

   EXPECT_EQ (error, 0);
   EXPECT_EQ (actual.chassis_id.is_valid, false);
}

TEST_F (LldpTest, LldpParsePacketWithTooBigPortId)
{
   uint8_t packet[MAX_LLDP_PAYLOAD_SIZE];
   int error;
   size_t size;
   pf_lldp_peer_info_t actual;
   pf_lldp_peer_info_t peer = fake_peer_info();
   const size_t max_len = sizeof (peer.port_id.string) - 1;

   peer.port_id.len = max_len + 1;
   size = construct_packet (packet, &peer);
   memset (&actual, 0xff, sizeof (actual));

   error = pf_lldp_parse_packet (packet, size, &actual);

   EXPECT_EQ (error, 0);
   EXPECT_EQ (actual.port_id.is_valid, false);
}

TEST_F (LldpTest, LldpParsePacketWithTooBigPortDescription)
{
   uint8_t packet[MAX_LLDP_PAYLOAD_SIZE];
   int error;
   size_t size;
   pf_lldp_peer_info_t actual;
   pf_lldp_peer_info_t peer = fake_peer_info();
   const size_t max_len = sizeof (peer.port_description.string) - 1;

   peer.port_description.len = max_len + 1;
   size = construct_packet (packet, &peer);
   memset (&actual, 0xff, sizeof (actual));

   error = pf_lldp_parse_packet (packet, size, &actual);

   EXPECT_EQ (error, 0);
   EXPECT_EQ (actual.port_description.is_valid, false);
}

TEST_F (LldpTest, LldpParsePacketWithTooBigManagementAddress)
{
   uint8_t packet[MAX_LLDP_PAYLOAD_SIZE];
   int error;
   size_t size;
   pf_lldp_peer_info_t actual;
   pf_lldp_peer_info_t peer = fake_peer_info();
   const size_t max_len = sizeof (peer.management_address.value);

   peer.management_address.len = max_len + 1;
   size = construct_packet (packet, &peer);
   memset (&actual, 0xff, sizeof (actual));

   error = pf_lldp_parse_packet (packet, size, &actual);

   EXPECT_EQ (error, 0);
   EXPECT_EQ (actual.management_address.is_valid, false);
}

TEST_F (LldpTest, LldpGetChassisId)
{
   pf_lldp_chassis_id_t chassis_id;

   memset (&chassis_id, 0xff, sizeof (chassis_id));

   /* Currently, the MAC address is used as Chassis ID */
   pf_lldp_get_chassis_id (net, &chassis_id);
   EXPECT_EQ (chassis_id.subtype, PF_LLDP_SUBTYPE_MAC);
   EXPECT_EQ (chassis_id.string[0], 0x12);
   EXPECT_EQ (chassis_id.string[1], 0x34);
   EXPECT_EQ (chassis_id.string[2], 0x00);
   EXPECT_EQ (chassis_id.string[3], 0x78);
   EXPECT_EQ (chassis_id.string[4], (char)0x90);
   EXPECT_EQ (chassis_id.string[5], (char)0xab);
   EXPECT_EQ (chassis_id.len, 6u);

   /* TODO: Add test case for locally assigned Chassis ID */
}

TEST_F (LldpTest, LldpGetPortId)
{
   pf_lldp_port_id_t port_id;

   memset (&port_id, 0xff, sizeof (port_id));

   pf_lldp_get_port_id (net, LOCAL_PORT, &port_id);
   EXPECT_EQ (port_id.subtype, PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED);
   EXPECT_STREQ (port_id.string, "port-001");
   EXPECT_EQ (port_id.len, strlen ("port-001"));
}

TEST_F (LldpTest, LldpGetPortDescription)
{
   pf_lldp_port_description_t port_desc;

   memset (&port_desc, 0xff, sizeof (port_desc));

   pf_lldp_get_port_description (net, LOCAL_PORT, &port_desc);
   EXPECT_STREQ (port_desc.string, TEST_INTERFACE_NAME);
   EXPECT_EQ (port_desc.len, strlen (TEST_INTERFACE_NAME));
}

TEST_F (LldpTest, LldpGetManagementAddress)
{
   pf_lldp_management_address_t man_address;

   mock_os_data.interface_index = 3;
   memset (&man_address, 0xff, sizeof (man_address));

   pf_lldp_get_management_address (net, &man_address);
   EXPECT_EQ (man_address.subtype, 1); /* IPv4 */
   EXPECT_EQ (man_address.value[0], 192);
   EXPECT_EQ (man_address.value[1], 168);
   EXPECT_EQ (man_address.value[2], 1);
   EXPECT_EQ (man_address.value[3], 171);
   EXPECT_EQ (man_address.len, 4u);
   EXPECT_EQ (man_address.interface_number.subtype, 2); /* ifIndex */
   EXPECT_EQ (man_address.interface_number.value, 3);
}

TEST_F (LldpTest, LldpGetSignalDelays)
{
   pf_lldp_signal_delay_t delays;

   memset (&delays, 0xff, sizeof (delays));

   pf_lldp_get_signal_delays (net, LOCAL_PORT, &delays);
   EXPECT_EQ (delays.tx_delay_local, 0u);    /* Not measured */
   EXPECT_EQ (delays.rx_delay_local, 0u);    /* Not measured */
   EXPECT_EQ (delays.cable_delay_local, 0u); /* Not measured */
}

TEST_F (LldpTest, LldpGetLinkStatus)
{
   pf_lldp_link_status_t link_status;

   memset (&link_status, 0xff, sizeof (link_status));

   pf_lldp_get_link_status (net, LOCAL_PORT, &link_status);
   EXPECT_EQ (link_status.is_autonegotiation_supported, true);
   EXPECT_EQ (link_status.is_autonegotiation_enabled, true);
   EXPECT_EQ (
      link_status.autonegotiation_advertised_capabilities,
      PNET_LLDP_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
         PNET_LLDP_AUTONEG_CAP_100BaseTX_FULL_DUPLEX);
   EXPECT_EQ (
      link_status.operational_mau_type,
      PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX);
}

TEST_F (LldpTest, LldpGetPeerTimestamp)
{
   uint32_t actual;
   const uint32_t expected = 0x87654321;
   int error;
   pf_lldp_peer_info_t received_data;

   /* Nothing received */
   error = pf_lldp_get_peer_timestamp (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, -1);

   /* Receive fake data at fake timestamp */
   mock_os_data.system_uptime_10ms = expected;
   received_data = fake_peer_info();
   pf_lldp_store_peer_info (net, LOCAL_PORT, &received_data);

   error = pf_lldp_get_peer_timestamp (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (actual, expected);
}

TEST_F (LldpTest, LldpGetPeerChassisId)
{
   const pf_lldp_peer_info_t received = fake_peer_info();
   const pf_lldp_chassis_id_t expected = received.chassis_id;
   pf_lldp_chassis_id_t actual;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_chassis_id (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, -1);

   /* Receive fake data */
   pf_lldp_store_peer_info (net, LOCAL_PORT, &received);

   error = pf_lldp_get_peer_chassis_id (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (memcmp (&actual, &expected, sizeof (actual)), 0);
}

TEST_F (LldpTest, LldpGetPeerPortId)
{
   const pf_lldp_peer_info_t received = fake_peer_info();
   const pf_lldp_port_id_t expected = received.port_id;
   pf_lldp_port_id_t actual;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_port_id (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, -1);

   /* Receive fake data */
   pf_lldp_store_peer_info (net, LOCAL_PORT, &received);

   error = pf_lldp_get_peer_port_id (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (memcmp (&actual, &expected, sizeof (actual)), 0);
}

TEST_F (LldpTest, LldpGetPeerPortDescription)
{
   const pf_lldp_peer_info_t received = fake_peer_info();
   const pf_lldp_port_description_t expected = received.port_description;
   pf_lldp_port_description_t actual;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_port_description (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, -1);

   /* Receive fake data */
   pf_lldp_store_peer_info (net, LOCAL_PORT, &received);

   error = pf_lldp_get_peer_port_description (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (memcmp (&actual, &expected, sizeof (actual)), 0);
}

TEST_F (LldpTest, LldpGetPeerManagementAddress)
{
   const pf_lldp_peer_info_t received = fake_peer_info();
   const pf_lldp_management_address_t expected = received.management_address;
   pf_lldp_management_address_t actual;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_management_address (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, -1);

   /* Receive fake data */
   pf_lldp_store_peer_info (net, LOCAL_PORT, &received);

   error = pf_lldp_get_peer_management_address (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (memcmp (&actual, &expected, sizeof (actual)), 0);
}

TEST_F (LldpTest, LldpGetPeerStationName)
{
   pf_lldp_station_name_t station_name;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_station_name (net, LOCAL_PORT, &station_name);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
}

TEST_F (LldpTest, LldpGetPeerSignalDelays)
{
   const pf_lldp_peer_info_t received = fake_peer_info();
   const pf_lldp_signal_delay_t expected = received.port_delay;
   pf_lldp_signal_delay_t actual;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_signal_delays (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, -1);

   /* Receive fake data */
   pf_lldp_store_peer_info (net, LOCAL_PORT, &received);

   error = pf_lldp_get_peer_signal_delays (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (memcmp (&actual, &expected, sizeof (actual)), 0);
}

TEST_F (LldpTest, LldpGetPeerLinkStatus)
{
   const pf_lldp_peer_info_t received = fake_peer_info();
   const pf_lldp_link_status_t expected = received.phy_config;
   pf_lldp_link_status_t actual;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_link_status (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, -1);

   /* Receive fake data */
   pf_lldp_store_peer_info (net, LOCAL_PORT, &received);

   error = pf_lldp_get_peer_link_status (net, LOCAL_PORT, &actual);
   EXPECT_EQ (error, 0);
   EXPECT_EQ (memcmp (&actual, &expected, sizeof (actual)), 0);
}
