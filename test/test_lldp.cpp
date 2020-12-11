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
 * @brief Integration test that a LLDP frame is sent at init.
 *
 */

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

#define LOCAL_PORT 1

const char * chassis_id_test_sample_1 =
   "\x53\x43\x41\x4c\x41\x4e\x43\x45\x20\x58\x2d\x32\x30\x30\x20"
   "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x36\x47\x4b\x35\x20"
   "\x32\x30\x34\x2d\x30\x42\x41\x30\x30\x2d\x32\x42\x41\x33\x20\x20"
   "\x53\x56\x50\x4c\x36\x31\x34\x37\x39\x32\x30\x20\x20\x20\x20\x20"
   "\x20\x20\x20\x20\x20\x39\x20\x56\x20\x20\x35\x20\x20\x34\x20\x20"
   "\x32";

const char * port_id_test_sample_1 =
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

   memset (&man_address, 0xff, sizeof (man_address));

   pf_lldp_get_management_address (net, &man_address);
   EXPECT_EQ (man_address.subtype, 1); /* IPv4 */
   EXPECT_EQ (man_address.value[0], 192);
   EXPECT_EQ (man_address.value[1], 168);
   EXPECT_EQ (man_address.value[2], 1);
   EXPECT_EQ (man_address.value[3], 171);
   EXPECT_EQ (man_address.len, 4u);
}

TEST_F (LldpTest, LldpGetManagementPortIndex)
{
   pf_lldp_management_port_index_t man_port_index;

   memset (&man_port_index, 0xff, sizeof (man_port_index));

   pf_lldp_get_management_port_index (net, &man_port_index);
   EXPECT_EQ (man_port_index.subtype, 2); /* ifIndex */
   EXPECT_EQ (man_port_index.index, 1);   /* TODO */
}

TEST_F (LldpTest, LldpGetSignalDelays)
{
   pf_lldp_signal_delay_t delays;

   memset (&delays, 0xff, sizeof (delays));

   pf_lldp_get_signal_delays (net, LOCAL_PORT, &delays);
   EXPECT_EQ (delays.port_tx_delay_ns, 0u);          /* Not measured */
   EXPECT_EQ (delays.port_rx_delay_ns, 0u);          /* Not measured */
   EXPECT_EQ (delays.line_propagation_delay_ns, 0u); /* Not measured */
}

TEST_F (LldpTest, LldpGetLinkStatus)
{
   pf_lldp_link_status_t link_status;

   memset (&link_status, 0xff, sizeof (link_status));

   pf_lldp_get_link_status (net, LOCAL_PORT, &link_status);
   EXPECT_EQ (link_status.auto_neg_supported, true);
   EXPECT_EQ (link_status.auto_neg_enabled, true);
   EXPECT_EQ (
      link_status.auto_neg_advertised_cap,
      PNET_LLDP_AUTONEG_CAP_100BaseTX_HALF_DUPLEX |
         PNET_LLDP_AUTONEG_CAP_100BaseTX_FULL_DUPLEX);
   EXPECT_EQ (link_status.oper_mau_type, PNET_MAU_COPPER_100BaseTX_FULL_DUPLEX);
}

TEST_F (LldpTest, LldpGetPeerTimestamp)
{
   uint32_t timestamp_10ms;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_timestamp (net, LOCAL_PORT, &timestamp_10ms);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
}

TEST_F (LldpTest, LldpGetPeerChassisId)
{
   pf_lldp_chassis_id_t chassis_id;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_chassis_id (net, LOCAL_PORT, &chassis_id);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
}

TEST_F (LldpTest, LldpGetPeerPortId)
{
   pf_lldp_port_id_t port_id;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_port_id (net, LOCAL_PORT, &port_id);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
}

TEST_F (LldpTest, LldpGetPeerPortDescription)
{
   pf_lldp_port_description_t port_desc;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_port_description (net, LOCAL_PORT, &port_desc);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
}

TEST_F (LldpTest, LldpGetPeerManagementAddress)
{
   pf_lldp_management_address_t man_address;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_management_address (net, LOCAL_PORT, &man_address);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
}

TEST_F (LldpTest, LldpGetPeerManagementPortIndex)
{
   pf_lldp_management_port_index_t man_port_index;
   int error;

   /* Nothing received */
   error =
      pf_lldp_get_peer_management_port_index (net, LOCAL_PORT, &man_port_index);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
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
   pf_lldp_signal_delay_t delays;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_signal_delays (net, LOCAL_PORT, &delays);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
}

TEST_F (LldpTest, LldpGetPeerLinkStatus)
{
   pf_lldp_link_status_t link_status;
   int error;

   /* Nothing received */
   error = pf_lldp_get_peer_link_status (net, LOCAL_PORT, &link_status);
   EXPECT_EQ (error, -1);

   /* TODO: Add test case for scenario where LLDP packet has been received */
}
