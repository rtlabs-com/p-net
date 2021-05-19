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

#define MAX_ETH_FRAME_SIZE   1514u
#define MAX_ETH_PAYLOAD_SIZE 1500u

#define LLDP_TYPE_END                0
#define LLDP_TYPE_CHASSIS_ID         1
#define LLDP_TYPE_PORT_ID            2
#define LLDP_TYPE_TTL                3
#define LLDP_TYPE_PORT_DESCRIPTION   4
#define LLDP_TYPE_MANAGEMENT_ADDRESS 8
#define LLDP_TYPE_ORG_SPEC           127

#define TLV_BYTE_0(len, type) (((len) >> 8) | ((type) << 1))
#define TLV_BYTE_1(len)       (((len) >> 0) & 0xff)
#define TLV_TYPE(tlv)         ((tlv)[0] >> 1)
#define TLV_LEN(tlv)          ((size_t) (((tlv)[0] << 8) + (tlv)[1]) & 0x1FF)

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

static pf_lldp_chassis_id_t fake_chassis_id (void)
{
   pf_lldp_chassis_id_t chassis_id;

   memset (&chassis_id, 0, sizeof (chassis_id));
   snprintf (
      chassis_id.string,
      sizeof (chassis_id.string),
      "%s",
      "Chassis ID received from remote device");
   chassis_id.len = strlen (chassis_id.string);
   chassis_id.subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   chassis_id.is_valid = true;
   return chassis_id;
}

static pf_lldp_port_id_t fake_port_id (void)
{
   pf_lldp_port_id_t port_id;

   memset (&port_id, 0, sizeof (port_id));
   snprintf (
      port_id.string,
      sizeof (port_id.string),
      "%s",
      "Port ID received from remote device");
   port_id.len = strlen (port_id.string);
   port_id.subtype = PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED;
   port_id.is_valid = true;
   return port_id;
}

static uint16_t fake_ttl (void)
{
   return 12345;
}

static pf_lldp_port_description_t fake_port_description (void)
{
   pf_lldp_port_description_t port_description;

   memset (&port_description, 0, sizeof (port_description));
   snprintf (
      port_description.string,
      sizeof (port_description.string),
      "%s",
      "Port Description");
   port_description.len = strlen (port_description.string);
   port_description.is_valid = true;
   return port_description;
}

static pf_lldp_management_address_t fake_ipv4_management_address (void)
{
   pf_lldp_management_address_t management_address;

   memset (&management_address, 0, sizeof (management_address));
   management_address.subtype = 1; /* IPv4 */
   management_address.value[0] = 192;
   management_address.value[1] = 168;
   management_address.value[2] = 10;
   management_address.value[3] = 102;
   management_address.len = 4;
   management_address.interface_number.subtype = 2; /* ifIndex */
   management_address.interface_number.value = 1;
   management_address.is_valid = true;
   return management_address;
}

static pf_lldp_management_address_t fake_ipv6_management_address (void)
{
   pf_lldp_management_address_t management_address;

   memset (&management_address, 0, sizeof (management_address));
   management_address.subtype = 2; /* IPv6 */
   management_address.value[0] = 0x20;
   management_address.value[1] = 0x01;
   management_address.value[2] = 0x0d;
   management_address.value[3] = 0xd8;
   management_address.value[4] = 0x85;
   management_address.value[5] = 0xa3;
   management_address.value[6] = 0x00;
   management_address.value[7] = 0x00;
   management_address.value[8] = 0x00;
   management_address.value[9] = 0x00;
   management_address.value[10] = 0x8a;
   management_address.value[11] = 0x2e;
   management_address.value[12] = 0x03;
   management_address.value[13] = 0x70;
   management_address.value[14] = 0x73;
   management_address.value[15] = 0x34;
   management_address.len = 16;
   management_address.interface_number.subtype = 2; /* ifIndex */
   management_address.interface_number.value = 1;
   management_address.is_valid = true;
   return management_address;
}

static pf_lldp_link_status_t fake_phy_config (void)
{
   pf_lldp_link_status_t phy_config;

   memset (&phy_config, 0, sizeof (phy_config));
   phy_config.is_autonegotiation_supported = true;
   phy_config.is_autonegotiation_enabled = true;
   phy_config.autonegotiation_advertised_capabilities =
      PNAL_ETH_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
      PNAL_ETH_AUTONEG_CAP_100BaseTX_FULL_DUPLEX;
   phy_config.operational_mau_type = PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX;
   phy_config.is_valid = true;
   return phy_config;
}

static pnet_ethaddr_t fake_mac_address (void)
{
   pnet_ethaddr_t mac_address;

   memset (&mac_address, 0, sizeof (mac_address));
   mac_address.addr[0] = 0xab;
   mac_address.addr[0] = 0xcd;
   mac_address.addr[0] = 0xef;
   mac_address.addr[0] = 0x01;
   mac_address.addr[0] = 0x23;
   mac_address.addr[0] = 0x45;
   return mac_address;
}

static pf_lldp_signal_delay_t fake_port_delay (void)
{
   pf_lldp_signal_delay_t port_delay;

   memset (&port_delay, 0, sizeof (port_delay));
   port_delay.cable_delay_local = 1;
   port_delay.rx_delay_local = 2;
   port_delay.tx_delay_local = 3;
   port_delay.rx_delay_remote = 4;
   port_delay.tx_delay_remote = 5;
   port_delay.is_valid = true;
   return port_delay;
}

static pf_lldp_peer_info_t fake_peer_info (void)
{
   pf_lldp_peer_info_t peer;

   memset (&peer, 0, sizeof (peer));
   peer.chassis_id = fake_chassis_id();
   peer.port_id = fake_port_id();
   peer.ttl = fake_ttl();
   peer.port_description = fake_port_description();
   peer.management_address = fake_ipv4_management_address();
   peer.phy_config = fake_phy_config();
   peer.mac_address = fake_mac_address();
   peer.port_delay = fake_port_delay();
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

static size_t construct_packet_ttl (uint8_t packet[], uint16_t ttl)
{
   size_t size = 0;

   packet[0] = TLV_BYTE_0 (sizeof (ttl), LLDP_TYPE_TTL);
   packet[1] = TLV_BYTE_1 (sizeof (ttl));
   packet[2] = ttl >> 8;
   packet[3] = ttl >> 0;
   size = 4;

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

static size_t construct_packet_chassis_mac (
   uint8_t packet[],
   const pnet_ethaddr_t * mac_address)
{
   size_t size = 0;

   packet[0] = TLV_BYTE_0 (10, LLDP_TYPE_ORG_SPEC);
   packet[1] = TLV_BYTE_1 (10);
   packet[2] = 0x00; /* Profinet OUI */
   packet[3] = 0x0E; /* Profinet OUI */
   packet[4] = 0xCF; /* Profinet OUI */
   packet[5] = 5;    /* subtype */
   memcpy (&packet[6], mac_address->addr, 6);
   size = 12;

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
   uint8_t packet[MAX_ETH_PAYLOAD_SIZE],
   const pf_lldp_peer_info_t * data)
{
   memset (packet, 0xff, MAX_ETH_PAYLOAD_SIZE);
   size_t size = 0;

   size += construct_packet_chassis_id (&packet[size], &data->chassis_id);
   size += construct_packet_port_id (&packet[size], &data->port_id);
   size += construct_packet_ttl (&packet[size], data->ttl);
   size += construct_packet_port_description (
      &packet[size],
      &data->port_description);
   size += construct_packet_management_address (
      &packet[size],
      &data->management_address);
   size += construct_packet_link_status (&packet[size], &data->phy_config);
   size += construct_packet_signal_delay (&packet[size], &data->port_delay);
   size += construct_packet_chassis_mac (&packet[size], &data->mac_address);
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

      pnet_init_only (net, &pnet_default_cfg);

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
   EXPECT_EQ (mock_os_data.eth_send_count, PNET_MAX_PHYSICAL_PORTS);
}

TEST_F (LldpTest, LldpGenerateAliasName)
{
   char alias[256]; /** Terminated string */
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
   uint8_t packet[MAX_ETH_PAYLOAD_SIZE];
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

   EXPECT_EQ (actual.ttl, expected.ttl);

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

   EXPECT_EQ (memcmp (actual.mac_address.addr, expected.mac_address.addr, 6), 0);

   /* TODO: Add check for port status */

   /* Finally check that everything is as expected.
    * While this is the only check really needed, the checks above will make
    * it easier to find out exactly what went wrong.
    */
   EXPECT_EQ (memcmp (&actual, &expected, sizeof (expected)), 0);
}

TEST_F (LldpTest, LldpParsePacketWithTooBigChassisId)
{
   uint8_t packet[MAX_ETH_PAYLOAD_SIZE];
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
   uint8_t packet[MAX_ETH_PAYLOAD_SIZE];
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
   uint8_t packet[MAX_ETH_PAYLOAD_SIZE];
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
   uint8_t packet[MAX_ETH_PAYLOAD_SIZE];
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

TEST_F (LldpTest, LldpParsePacketWithMultipleManagementAddresses)
{
   uint8_t packet[MAX_ETH_PAYLOAD_SIZE];
   int error;
   size_t size;
   pf_lldp_peer_info_t actual;
   const pf_lldp_chassis_id_t chassis_id = fake_chassis_id();
   const pf_lldp_port_id_t port_id = fake_port_id();
   const uint16_t ttl = fake_ttl();
   const pf_lldp_management_address_t ipv4 = fake_ipv4_management_address();
   const pf_lldp_management_address_t ipv6 = fake_ipv6_management_address();

   /* Construct LLDP packet with IPv4 and IPv6 addresses (in that order) */
   memset (packet, 0xff, MAX_ETH_PAYLOAD_SIZE);
   size = 0;
   size += construct_packet_chassis_id (&packet[size], &chassis_id);
   size += construct_packet_port_id (&packet[size], &port_id);
   size += construct_packet_ttl (&packet[size], ttl);
   size += construct_packet_management_address (&packet[size], &ipv4);
   size += construct_packet_management_address (&packet[size], &ipv6);
   size += construct_packet_end_marker (&packet[size]);

   memset (&actual, 0xff, sizeof (actual));

   error = pf_lldp_parse_packet (packet, size, &actual);

   /* Note that only the IPv4 address should be saved, as IPv6 addresses are
    * of no use for the p-net stack.
    */
   EXPECT_EQ (error, 0);
   EXPECT_EQ (actual.management_address.is_valid, true);
   EXPECT_EQ (actual.management_address.subtype, ipv4.subtype);
   EXPECT_EQ (actual.management_address.len, 4u);
}

TEST_F (LldpTest, LldpParsePacketWithInvalidManagementAddresses)
{
   uint8_t packet[MAX_ETH_PAYLOAD_SIZE];
   int error;
   size_t size;
   pf_lldp_peer_info_t actual;
   const pf_lldp_chassis_id_t chassis_id = fake_chassis_id();
   const pf_lldp_port_id_t port_id = fake_port_id();
   const uint16_t ttl = fake_ttl();
   pf_lldp_management_address_t address;

   address = fake_ipv4_management_address();
   address.len = 5;

   /* Construct LLDP packet */
   memset (packet, 0xff, MAX_ETH_PAYLOAD_SIZE);
   size = 0;
   size += construct_packet_chassis_id (&packet[size], &chassis_id);
   size += construct_packet_port_id (&packet[size], &port_id);
   size += construct_packet_ttl (&packet[size], ttl);
   size += construct_packet_management_address (&packet[size], &address);
   size += construct_packet_end_marker (&packet[size]);

   memset (&actual, 0xff, sizeof (actual));

   error = pf_lldp_parse_packet (packet, size, &actual);

   EXPECT_EQ (error, 0);
   EXPECT_EQ (actual.management_address.is_valid, false);
}

TEST_F (LldpTest, LldpGetChassisId)
{
   pf_lldp_chassis_id_t chassis_id;

   memset (&chassis_id, 0xff, sizeof (chassis_id));

   pf_lldp_get_chassis_id (net, &chassis_id);
   EXPECT_EQ (chassis_id.subtype, PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED);
   EXPECT_STREQ (
      chassis_id.string,
      "PNET unit tests           <orderid>            <serial nbr>         1 P "
      " 0  0  0");
   EXPECT_EQ (
      chassis_id.len,
      strlen ("PNET unit tests           <orderid>            <serial nbr>     "
              "    1 P  0  0  0"));
}

TEST_F (LldpTest, LldpGetPortId)
{
   pf_lldp_port_id_t port_id;

   memset (&port_id, 0xff, sizeof (port_id));

   pf_lldp_get_port_id (net, LOCAL_PORT, &port_id);
   EXPECT_EQ (port_id.subtype, PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED);
   EXPECT_STREQ (port_id.string, "port-001.12-34-00-78-90-AB");
   EXPECT_EQ (port_id.len, strlen ("port-001.12-34-00-78-90-AB"));
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

   mock_os_data.eth_status[LOCAL_PORT].is_autonegotiation_supported = true;
   mock_os_data.eth_status[LOCAL_PORT].is_autonegotiation_enabled = true;
   mock_os_data.eth_status[LOCAL_PORT].autonegotiation_advertised_capabilities =
      PNAL_ETH_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
      PNAL_ETH_AUTONEG_CAP_100BaseTX_FULL_DUPLEX;
   mock_os_data.eth_status[LOCAL_PORT].operational_mau_type =
      PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX;
   memset (&link_status, 0xff, sizeof (link_status));

   /* The ethernet status is updated by the background worker */
   pf_pdport_update_eth_status (net);

   pf_lldp_get_link_status (net, LOCAL_PORT, &link_status);
   EXPECT_EQ (link_status.is_autonegotiation_supported, true);
   EXPECT_EQ (link_status.is_autonegotiation_enabled, true);
   EXPECT_EQ (
      link_status.autonegotiation_advertised_capabilities,
      PNAL_ETH_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
         PNAL_ETH_AUTONEG_CAP_100BaseTX_FULL_DUPLEX);
   EXPECT_EQ (
      link_status.operational_mau_type,
      PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX);
}

TEST_F (LldpTest, LldpGetLinkStatusGivenAutonegotiationDisabled)
{
   pf_lldp_link_status_t link_status;

   mock_os_data.eth_status[LOCAL_PORT].is_autonegotiation_supported = false;
   mock_os_data.eth_status[LOCAL_PORT].is_autonegotiation_enabled = false;
   mock_os_data.eth_status[LOCAL_PORT].autonegotiation_advertised_capabilities =
      0x0000;
   mock_os_data.eth_status[LOCAL_PORT].operational_mau_type =
      PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX;
   memset (&link_status, 0xff, sizeof (link_status));

   pf_pdport_update_eth_status (net);

   pf_lldp_get_link_status (net, LOCAL_PORT, &link_status);
   EXPECT_EQ (link_status.is_autonegotiation_supported, false);
   EXPECT_EQ (link_status.is_autonegotiation_enabled, false);
   EXPECT_EQ (link_status.autonegotiation_advertised_capabilities, 0x0000);
   EXPECT_EQ (
      link_status.operational_mau_type,
      PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX);
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

TEST_F (LldpTest, LldpConstructFrame)
{
   uint8_t frame[MAX_ETH_FRAME_SIZE];
   const uint8_t * p;
   const uint8_t * tlv;
   size_t returned_size;
   size_t size;

   mock_os_data.interface_index = 3;

   returned_size = pf_lldp_construct_frame (net, LOCAL_PORT, frame);

   EXPECT_GT (returned_size, 14u);
   EXPECT_LE (returned_size, MAX_ETH_FRAME_SIZE);

   size = 0;

   /* Check destination MAC address (from LLDP standard) */
   p = &frame[size];
   EXPECT_EQ (p[0], 0x01);
   EXPECT_EQ (p[1], 0x80);
   EXPECT_EQ (p[2], 0xC2);
   EXPECT_EQ (p[3], 0x00);
   EXPECT_EQ (p[4], 0x00);
   EXPECT_EQ (p[5], 0x0E);
   size += 6;

   /* Check source MAC address (from app configuration) */
   p = &frame[size];
   EXPECT_EQ (p[0], 0x12);
   EXPECT_EQ (p[1], 0x34);
   EXPECT_EQ (p[2], 0x00);
   EXPECT_EQ (p[3], 0x78);
   EXPECT_EQ (p[4], 0x90);
   EXPECT_EQ (p[5], 0xab);
   size += 6;

   /* Check EtherType (from LLDP standard) */
   p = &frame[size];
   EXPECT_EQ (p[0], 0x88);
   EXPECT_EQ (p[1], 0xCC);
   size += 2;

   /* Check Chassis ID TLV */
   tlv = &frame[size];
   EXPECT_EQ (TLV_TYPE (tlv), LLDP_TYPE_CHASSIS_ID);
   EXPECT_GE (TLV_LEN (tlv), 2u);
   EXPECT_LE (TLV_LEN (tlv), 256u);
   size += 2 + TLV_LEN (tlv);

   /* Check Port ID TLV */
   tlv = &frame[size];
   EXPECT_EQ (TLV_TYPE (tlv), LLDP_TYPE_PORT_ID);
   EXPECT_GE (TLV_LEN (tlv), 2u);
   EXPECT_LE (TLV_LEN (tlv), 256u);
   EXPECT_EQ (tlv[2], 7u); /* Subtype */
   size += 2 + TLV_LEN (tlv);

   /* Check Time To Live TLV */
   tlv = &frame[size];
   EXPECT_EQ (TLV_TYPE (tlv), LLDP_TYPE_TTL);
   EXPECT_EQ (TLV_LEN (tlv), 2u);
   EXPECT_EQ (tlv[2], 0u);  /* MSB of TTL */
   EXPECT_EQ (tlv[3], 20u); /* LSB of TTL */
   size += 2 + TLV_LEN (tlv);

   /* Note that the order in which TLVs below are placed in the packet
    * is not specified by LLDP or Profinet (except for the End TTL).
    */

   /* Check LLDP_PNIO_PORTSTATUS TLV */
   tlv = &frame[size];
   EXPECT_EQ (TLV_TYPE (tlv), LLDP_TYPE_ORG_SPEC);
   EXPECT_EQ (TLV_LEN (tlv), 3 + 1 + 4u);
   EXPECT_EQ (tlv[2], 0x00); /* Profinet OUI */
   EXPECT_EQ (tlv[3], 0x0E); /* Profinet OUI */
   EXPECT_EQ (tlv[4], 0xCF); /* Profinet OUI */
   EXPECT_EQ (tlv[5], 0x02); /* Subtype */
   size += 2 + TLV_LEN (tlv);

   /* Check LLDP_PNIO_CHASSIS_MAC TLV */
   tlv = &frame[size];
   EXPECT_EQ (TLV_TYPE (tlv), LLDP_TYPE_ORG_SPEC);
   EXPECT_EQ (TLV_LEN (tlv), 3 + 1 + 6u);
   EXPECT_EQ (tlv[2], 0x00);  /* Profinet OUI */
   EXPECT_EQ (tlv[3], 0x0E);  /* Profinet OUI */
   EXPECT_EQ (tlv[4], 0xCF);  /* Profinet OUI */
   EXPECT_EQ (tlv[5], 0x05);  /* Subtype */
   EXPECT_EQ (tlv[6], 0x12);  /* MAC address */
   EXPECT_EQ (tlv[7], 0x34);  /* MAC address */
   EXPECT_EQ (tlv[8], 0x00);  /* MAC address */
   EXPECT_EQ (tlv[9], 0x78);  /* MAC address */
   EXPECT_EQ (tlv[10], 0x90); /* MAC address */
   EXPECT_EQ (tlv[11], 0xab); /* MAC address */
   size += 2 + TLV_LEN (tlv);

   /* Check 802.3 MAC/PHY Configuration/Status TLV */
   tlv = &frame[size];
   EXPECT_EQ (TLV_TYPE (tlv), LLDP_TYPE_ORG_SPEC);
   EXPECT_EQ (TLV_LEN (tlv), 3 + 1 + 5u);
   EXPECT_EQ (tlv[2], 0x00); /* 802.3 OUI */
   EXPECT_EQ (tlv[3], 0x12); /* 802.3 OUI */
   EXPECT_EQ (tlv[4], 0x0F); /* 802.3 OUI */
   EXPECT_EQ (tlv[5], 0x01); /* Subtype */
   size += 2 + TLV_LEN (tlv);

   /* Check Management Address TLV */
   tlv = &frame[size];
   EXPECT_EQ (TLV_TYPE (tlv), LLDP_TYPE_MANAGEMENT_ADDRESS);
   EXPECT_EQ (TLV_LEN (tlv), 1 + 5 + 5 + 1u);
   EXPECT_EQ (tlv[2], 1u + 4u); /* Size of Subtype+Address */
   EXPECT_EQ (tlv[3], 1u);      /* Subtype IPv4 */
   EXPECT_EQ (tlv[4], 192u);
   EXPECT_EQ (tlv[5], 168u);
   EXPECT_EQ (tlv[6], 1u);
   EXPECT_EQ (tlv[7], 171u);
   EXPECT_EQ (tlv[8], 2u);  /* Subtype ifIndex */
   EXPECT_EQ (tlv[9], 0u);  /* ifIndex MSB */
   EXPECT_EQ (tlv[10], 0u); /* ifIndex */
   EXPECT_EQ (tlv[11], 0u); /* ifIndex */
   EXPECT_EQ (tlv[12], 3u); /* ifIndex LSB */
   EXPECT_EQ (tlv[13], 0u); /* Size of OID */
   size += 2 + TLV_LEN (tlv);

   /* Check End Of LLDPPDU TLV */
   tlv = &frame[size];
   EXPECT_EQ (TLV_TYPE (tlv), LLDP_TYPE_END);
   EXPECT_EQ (TLV_LEN (tlv), 0u);
   size += 2 + TLV_LEN (tlv);

   EXPECT_EQ (size, returned_size);
}
