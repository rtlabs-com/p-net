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
 * @brief Testing of DCP aspects.
 *
 * For example
 *   Sending hello frame
 *   Handling of get name request
 *   Read station name
 *   Set station name
 *   Set IP address
 *   Do factory reset
 *
 */

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

class DcpTest : public PnetIntegrationTest
{
};

class DcpUnitTest : public PnetUnitTest
{
};

static uint8_t get_wrong_length_req[] = {
   // clang-format off

   // Destination MAC address. Pos 0
   0x12, 0x34, 0x00, 0x78, 0x90, 0xab,

   // Source MAC address. Pos 6
   0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,

   // Ethernet type = Profinet RT. Pos 12
   0x88, 0x92,

   // DCP set/get. Pos 14
   0xfe, 0xfd,

   // Get, request. Pos 16
   0x03, 0x00,

   // XID. Pos 20
   0x00, 0x00, 0x00, 0x01,

   // Reserved. Pos 22
   0x04, 0x01,

   // DCP data length. Wrong value. Pos 24
   0x04, 0x00,

   // Options. Pos 26
   0x00, 0x00

   // clang-format on
};

static uint8_t get_name_req[] = {
   0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfd, 0x03, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
   0x00, 0x06, 0x02, 0x02, 0x02, 0x03, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t ident_req[] = {
   0x01, 0x0e, 0xcf, 0x00, 0x00, 0x00, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfe, 0x05, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01,
   0x00, 0x10, 0x02, 0x02, 0x00, 0x0c, 0x72, 0x74, 0x2d, 0x6c, 0x61, 0x62,
   0x73, 0x2d, 0x64, 0x65, 0x6d, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t set_name_req[] = {
   0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
   0x00, 0x12, 0x02, 0x02, 0x00, 0x0e, 0x00, 0x00, 0x72, 0x74, 0x2d, 0x6c,
   0x61, 0x62, 0x73, 0x2d, 0x64, 0x65, 0x6d, 0x6f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t set_ip_req[] = {
   0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
   0x00, 0x18, 0x01, 0x02, 0x00, 0x0e, 0x00, 0x00, 0xc0, 0xa8, 0x01, 0xab,
   0xff, 0xff, 0xff, 0x00, 0xc0, 0xa8, 0x01, 0x01, 0x05, 0x02, 0x00, 0x02,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t factory_reset_req[] = {
   0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
   0x00, 0x06, 0x05, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t signal_req[] = {
   0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
   0x00, 0x06, 0x05, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

TEST_F (DcpTest, DcpHelloTest)
{
   pnal_buf_t * p_buf;
   int ret;

   mock_clear();

   ret = pf_dcp_hello_req (net);
   EXPECT_EQ (ret, 0);

   mock_clear();

   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, get_name_req, sizeof (get_name_req));
   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1); // Incoming frame is handled
   // LLDP frames and one additional frame
   EXPECT_EQ (mock_os_data.eth_send_count, PNET_MAX_PHYSICAL_PORTS + 1);

   EXPECT_EQ (appdata.call_counters.led_off_calls, 1);
   EXPECT_EQ (appdata.call_counters.led_on_calls, 0);
   EXPECT_EQ (appdata.call_counters.state_calls, 0);
   EXPECT_EQ (appdata.call_counters.connect_calls, 0);
   EXPECT_EQ (appdata.call_counters.release_calls, 0);
   EXPECT_EQ (appdata.call_counters.dcontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.ccontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.read_calls, 0);
   EXPECT_EQ (appdata.call_counters.write_calls, 0);
}

TEST_F (DcpTest, DcpGetWrongLengthTest)
{
   pnal_buf_t * p_buf;
   int ret;

   mock_clear();

   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, get_wrong_length_req, sizeof (get_wrong_length_req));
   pnal_buf_set_size (p_buf, sizeof (get_wrong_length_req));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);                         // Incoming frame is handled
   EXPECT_EQ (mock_os_data.eth_send_count, 0); // Drop malformed frame

   EXPECT_EQ (appdata.call_counters.led_off_calls, 1);
   EXPECT_EQ (appdata.call_counters.led_on_calls, 0);
   EXPECT_EQ (appdata.call_counters.state_calls, 0);
   EXPECT_EQ (appdata.call_counters.connect_calls, 0);
   EXPECT_EQ (appdata.call_counters.release_calls, 0);
   EXPECT_EQ (appdata.call_counters.dcontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.ccontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.read_calls, 0);
   EXPECT_EQ (appdata.call_counters.write_calls, 0);
}

TEST_F (DcpTest, DcpRunTest)
{
   pnal_buf_t * p_buf;
   int ret;

   TEST_TRACE ("\nGenerating mock set name request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, set_name_req, sizeof (set_name_req));
   pnal_buf_set_size (p_buf, sizeof (set_name_req));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock set IP request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, set_ip_req, sizeof (set_ip_req));
   pnal_buf_set_size (p_buf, sizeof (set_ip_req));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock set ident request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, ident_req, sizeof (ident_req));
   pnal_buf_set_size (p_buf, sizeof (ident_req));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock factory reset request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, factory_reset_req, sizeof (factory_reset_req));
   pnal_buf_set_size (p_buf, sizeof (factory_reset_req));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock flash LED request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, signal_req, sizeof (signal_req));
   pnal_buf_set_size (p_buf, sizeof (signal_req));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);
   /* Wait for LED to flash three times at 1 Hz */
   run_stack (4 * 1000 * 1000);

   EXPECT_EQ (
      mock_os_data.eth_send_count,
      9 + (PNET_MAX_PHYSICAL_PORTS - 1) * 4);
   EXPECT_EQ (mock_os_data.set_ip_suite_count, 4);

   EXPECT_EQ (appdata.call_counters.led_on_calls, 3);
   EXPECT_EQ (appdata.call_counters.led_off_calls, 4);
   EXPECT_EQ (appdata.call_counters.state_calls, 0);
   EXPECT_EQ (appdata.call_counters.connect_calls, 0);
   EXPECT_EQ (appdata.call_counters.release_calls, 0);
   EXPECT_EQ (appdata.call_counters.dcontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.ccontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.read_calls, 0);
   EXPECT_EQ (appdata.call_counters.write_calls, 0);
}

TEST_F (
   DcpTest,
   ARequestToIdentifyByOurStationNameIsRespondedToAfterADelay)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0x01,0x0e,0xcf,0x00,0x00,0x00,   // Destination MAC (All)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x01,        // Response Delay Factor
       0x00,0x10,        // DCP Data Length = 16 bytes

       // --- DCP Block: Option 2 / Suboption 2 (Station Name) ---
       0x02,             // Option: Device Properties
       0x02,             // Suboption: Name of Station
       0x00,0x0c,        // Block Length: 12 bytes (Station Name only)
       'r','t','-','l','a','b','s','-','d','e','m','o'  // Station Name ASCII
   };
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 1);
}

TEST_F (
   DcpTest,
   ARequestToIdentifyByOtherStationNameIsIgnored)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0x01,0x0e,0xcf,0x00,0x00,0x00,   // Destination MAC (All)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x01,        // Response Delay Factor
       0x00,0x10,        // DCP Data Length = 16 bytes

       // --- DCP Block: Option 2 / Suboption 2 (Station Name) ---
       0x02,             // Option: Device Properties
       0x02,             // Suboption: Name of Station
       0x00,0x0c,        // Block Length: 12 bytes (Station Name only)
       'U','N','K','N','O','W','N','-','N','A','M','E'  // Station Name ASCII
   };
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
}

TEST_F (
   DcpTest,
   ARequestToIdentifyByOtherAliasNameIsIgnored)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0x01,0x0e,0xcf,0x00,0x00,0x00,   // Destination MAC (All)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x01,        // Response Delay Factor
       0x00,0x10,        // DCP Data Length = 16 bytes (one block)

       // --- DCP Block: DeviceProperties / AliasName ---
       0x02,             // Option: DeviceProperties
       0x06,             // Suboption: AliasName
       0x00,0x0c,        // Block Length = 12 bytes
      'd','e','m','o','-','a','l','i','a','s','0','1'  // "demo-alias01"
   };
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
}

TEST_F (
   DcpTest,
   ARequestToIdentifyByOurMacAddressIsRespondedToAfterADelay)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0x01,0x0e,0xcf,0x00,0x00,0x00,   // Destination MAC (All)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x00,        // Response Delay Factor
       0x00,0x1a,        // DCP Data Length = 26 bytes (two blocks)

       // --- DCP Block 1: DeviceProperties / NameOfStation ---
       0x02,             // Option: DeviceProperties
       0x02,             // Suboption: NameOfStation
       0x00,0x0c,        // Block Length = 12 bytes
       'r','t','-','l','a','b','s','-','d','e','m','o',  // Station Name ASCII

       // --- DCP Block 2: IPOption / SuboptionMACAddress ---
       0x01,             // Option: IPOption
       0x01,             // Suboption: MACAddress
       0x00,0x06,        // Block Length = 6 bytes
       0x12,0x34,0x00,0x78,0x90,0xab  // Target MAC address:
                                      // See mock_pnal_get_macaddress()
   };
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 1);
}

TEST_F (
   DcpTest,
   ARequestToIdentifyByOtherMacAddressIsIgnored)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0x01,0x0e,0xcf,0x00,0x00,0x00,   // Destination MAC (All)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x00,        // Response Delay Factor
       0x00,0x1a,        // DCP Data Length = 26 bytes (two blocks)

       // --- DCP Block 1: DeviceProperties / NameOfStation ---
       0x02,             // Option: DeviceProperties
       0x02,             // Suboption: NameOfStation
       0x00,0x0c,        // Block Length = 12 bytes
       'r','t','-','l','a','b','s','-','d','e','m','o',  // Station Name ASCII

       // --- DCP Block 2: IPOption / SuboptionMACAddress ---
       0x01,             // Option: IPOption
       0x01,             // Suboption: MACAddress
       0x00,0x06,        // Block Length = 6 bytes
       0x00,0x0e,0xcf,0x01,0x23,0x45  // Target MAC address (UNKNOWN)
   };
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
}

TEST_F (
   DcpTest,
   ARequestToIdentifyIsRespondedToAfterADelay)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0x01,0x0e,0xcf,0x00,0x00,0x00,   // Destination MAC (All)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x01,        // Response Delay Factor
       0x00,0x04,        // DCP Data Length = 4 bytes (AllSelectorBlock)

       // --- DCP AllSelectorBlock ---
       0xff,             // Option: AllSelector
       0xff,             // Suboption: AllSelector
       0x00,0x00         // Block Length = 0 (big-endian)
   };
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 1);
}

TEST_F (
   DcpTest,
   ARequestToIdentifyIsRespondedToWithCorrectData)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0x01,0x0e,0xcf,0x00,0x00,0x00,   // Destination MAC (All)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x01,        // Response Delay Factor
       0x00,0x10,        // DCP Data Length = 16 bytes

       // --- DCP Block: Option 2 / Suboption 2 (Station Name) ---
       0x02,             // Option: Device Properties
       0x02,             // Suboption: Name of Station
       0x00,0x0c,        // Block Length: 12 bytes (Station Name only)
       'r','t','-','l','a','b','s','-','d','e','m','o'  // Station Name ASCII
   };
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 1);
   const uint8_t * p = mock_os_data.eth_send_copy[0].data;

   // --- 1. Verify full Ethernet Header (6-byte dest MAC, 6-byte src MAC,
   // 2-byte EtherType) ---

   // Destination MAC (bytes 0..5)
   EXPECT_EQ (p[0], 0x00);
   EXPECT_EQ (p[1], 0x0a);
   EXPECT_EQ (p[2], 0x35);
   EXPECT_EQ (p[3], 0x00);
   EXPECT_EQ (p[4], 0x01);
   EXPECT_EQ (p[5], 0x02);

   // Source MAC (bytes 6..11 - see mock_pnal_get_macaddress())
   EXPECT_EQ (p[6], 0x12);
   EXPECT_EQ (p[7], 0x34);
   EXPECT_EQ (p[8], 0x00);
   EXPECT_EQ (p[9], 0x78);
   EXPECT_EQ (p[10], 0x90);
   EXPECT_EQ (p[11], 0xab);

   // EtherType (bytes 12..13)
   EXPECT_EQ (p[12], 0x88);
   EXPECT_EQ (p[13], 0x92);

   // --- 2. Verify Profinet Frame ID (bytes 14..15) ---
   EXPECT_EQ (p[14], 0xfe);
   EXPECT_EQ (p[15], 0xff);

   // --- 3. Verify DCP Header fields (bytes 16..25) ---
   EXPECT_EQ (p[16], 0x05); // Service ID
   EXPECT_EQ (p[17], 0x01); // Service Type
   EXPECT_EQ (p[18], 0x12); // XID[0]
   EXPECT_EQ (p[19], 0x34); // XID[1]
   EXPECT_EQ (p[20], 0x56); // XID[2]
   EXPECT_EQ (p[21], 0x78); // XID[3]
   EXPECT_EQ (p[22], 0x00); // Padding byte 1
   EXPECT_EQ (p[23], 0x00); // Padding byte 2
   EXPECT_EQ (p[24], 0x00); // DCP Data Length (hi) -- encoded in header
   EXPECT_EQ (p[25], mock_os_data.eth_send_copy[0].len - 26); // DCP Data Length
                                                              // (lo) --
                                                      // encoded in header

   // TODO: Check DCP blocks
}

TEST_F (
   DcpTest,
   ARequestToIdentifyWithTooLargeDataLengthIsIgnored)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0xff,0xff,0xff,0xff,0xff,0xff,   // Destination MAC (Broadcast)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x01,        // Response Delay Factor
       0x00,0x12,        // DCP Data Length = 18 bytes - INCORRECT LENGTH (should be 16)

       // --- DCP Block: Option 2 / Suboption 2 (Station Name) ---
       0x02,             // Option: Device Properties
       0x02,             // Suboption: Name of Station
       0x00,0x0c,        // Block Length: 12 bytes (Station Name only)
       'r','t','-','l','a','b','s','-','d','e','m','o'  // Station Name ASCII
   };
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
}

TEST_F (
   DcpTest,
   ARequestToIdentifyWithTooLargeBlockLengthIsIgnored)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0xff,0xff,0xff,0xff,0xff,0xff,   // Destination MAC (Broadcast)
       0x00,0x0a,0x35,0x00,0x01,0x02,   // Source MAC
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfe,                       // Frame ID: DCP Identify Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x00,             // Service Type: Request
       0x12,0x34,0x56,0x78, // XID (32-bit)
       0x00,0x01,        // Response Delay Factor
       0x00,0x10,        // DCP Data Length = 16 bytes

       // --- DCP Block: Option 2 / Suboption 2 (Station Name) ---
       0x02,             // Option: Device Properties
       0x02,             // Suboption: Name of Station
       0x00,0x0d,        // Block Length: 13 bytes - INCORRECT LENGTH (should be 12)
       'r','t','-','l','a','b','s','-','d','e','m','o'  // Station Name ASCII
   };
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
   run_stack (11 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);
}

TEST_F (
   DcpTest,
   AHelloMessageIsReceivedSuccessfully)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
       // --- Ethernet Header ---
       0x01,0x0e,0xcf,0x00,0x00,0x00,   // Destination MAC: Profinet DCP multicast
       0x12,0x34,0x00,0x78,0x90,0xab,   // Source MAC: Device
       0x88,0x92,                       // EtherType: Profinet

       // --- Profinet Frame ID ---
       0xfe,0xfc,                       // Frame ID: DCP Hello Request

       // --- DCP Header ---
       0x05,             // Service ID: Identify
       0x04,             // Service Type: Hello request
       0x12,0x34,0x56,0x79, // XID (unique transaction)
       0x00,0x00,        // Padding (reserved)
       0x00,0x36,        // DCP Data Length = 54 bytes (sum of all blocks)

       // --- Block 1: DeviceProperties / NameOfStation ---
       0x02,             // Option: DeviceProperties
       0x02,             // Suboption: NameOfStation
       0x00,0x0e,        // Block Length = 14 bytes
       'd','e','m','o','-','d','e','v','i','c','e','-','0','1',

       // --- Block 2: DeviceProperties / DeviceVendor ---
       0x02,             // Option: DeviceProperties
       0x01,             // Suboption: DeviceVendor
       0x00,0x0b,        // Block Length = 11 bytes
       'E','x','a','m','p','l','e','C','o','r','p',

       // --- Block 3: IP / IPv4 Settings ---
       0x01,             // Option: IP
       0x01,             // Suboption: IPParameter
       0x00,0x10,        // Block Length = 16 bytes
       0xc0,0xa8,0x01,0x64,   // IP Address: 192.168.1.100
       0xff,0xff,0xff,0x00,   // Subnet Mask: 255.255.255.0
       0xc0,0xa8,0x01,0x01,   // Gateway: 192.168.1.1
       0x00,0x00,0x00,0x00    // Reserved
   };
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
}

TEST_F (
   DcpTest,
   AHelloMessageIsSentSuccessfully)
{
   int ret;
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);

   ret = pf_dcp_hello_req (net);

   EXPECT_EQ (ret, 0);
   EXPECT_EQ (mock_os_data.eth_send_count, 1);
   const uint8_t * request = mock_os_data.eth_send_copy[0].data;
   size_t frame_len = mock_os_data.eth_send_copy[0].len;
   EXPECT_EQ (frame_len, 88); // Total frame length

   // --- Ethernet Header ---
   EXPECT_EQ (request[0], 0x01); // Dest MAC byte 0 (multicast)
   EXPECT_EQ (request[1], 0x0E); // Dest MAC byte 1
   EXPECT_EQ (request[2], 0xCF); // Dest MAC byte 2
   EXPECT_EQ (request[3], 0x00); // Dest MAC byte 3
   EXPECT_EQ (request[4], 0x00); // Dest MAC byte 4
   EXPECT_EQ (request[5], 0x01); // Dest MAC byte 5
   EXPECT_EQ (request[6], 0x12);  // Src MAC byte 0 (our MAC)
   EXPECT_EQ (request[7], 0x34);  // Src MAC byte 1
   EXPECT_EQ (request[8], 0x00);  // Src MAC byte 2
   EXPECT_EQ (request[9], 0x78);  // Src MAC byte 3
   EXPECT_EQ (request[10], 0x90); // Src MAC byte 4
   EXPECT_EQ (request[11], 0xAB); // Src MAC byte 5
   EXPECT_EQ (request[12], 0x88); // EtherType MSB
   EXPECT_EQ (request[13], 0x92); // EtherType LSB

   // --- Profinet Frame ID ---
   EXPECT_EQ (request[14], 0xFE); // Frame ID MSB
   EXPECT_EQ (request[15], 0xFC); // Frame ID LSB (Hello Request)

   // --- DCP Header (partial check, skip XID) ---
   EXPECT_EQ (request[16], 0x06); // Service ID: Hello
   EXPECT_EQ (request[17], 0x00); // Service Type: Request
   EXPECT_EQ (request[22], 0x00); // Padding byte 0
   EXPECT_EQ (request[23], 0x00); // Padding byte 1
   EXPECT_EQ (request[24], 0);    // DCP Data Length MSB
   EXPECT_EQ (request[25], 62);   // DCP Data Length LSB

   // --- DCP Hello Block: NameOfStation ---
   EXPECT_EQ (request[26], 0x02); // Option: Device Properties
   EXPECT_EQ (request[27], 0x02); // Suboption: NameOfStation
   EXPECT_EQ (request[28], 0x00); // BlockLength MSB
   EXPECT_EQ (request[29], 0x0E); // BlockLength LSB
   EXPECT_EQ (request[30], 0x00); // BlockQualifier MSB
   EXPECT_EQ (request[31], 0x00); // BlockQualifier LSB

   EXPECT_EQ (request[32], 'r'); // NameOfStation[0]
   EXPECT_EQ (request[33], 't');
   EXPECT_EQ (request[34], '-');
   EXPECT_EQ (request[35], 'l');
   EXPECT_EQ (request[36], 'a');
   EXPECT_EQ (request[37], 'b');
   EXPECT_EQ (request[38], 's');
   EXPECT_EQ (request[39], '-');
   EXPECT_EQ (request[40], 'd');
   EXPECT_EQ (request[41], 'e');
   EXPECT_EQ (request[42], 'm');
   EXPECT_EQ (request[43], 'o'); // Last character

      // --- IPParameterBlockRes ---
   EXPECT_EQ (request[44], 0x01); // Option: IP
   EXPECT_EQ (request[45], 0x02); // Suboption: IPParameter
   EXPECT_EQ (request[46], 0x00); // BlockLength MSB
   EXPECT_EQ (request[47], 0x0E); // BlockLength LSB
   EXPECT_EQ (request[48], 0x00); // BlockQualifier MSB
   EXPECT_EQ (request[49], 0x00); // BlockQualifier LSB

   // IP Address: 192.168.1.171 - see cfg_init()
   EXPECT_EQ (request[50], 192);
   EXPECT_EQ (request[51], 168);
   EXPECT_EQ (request[52], 1);
   EXPECT_EQ (request[53], 171);

   // Subnet mask: 255.255.255.255 - see cfg_init()
   EXPECT_EQ (request[54], 255);
   EXPECT_EQ (request[55], 255);
   EXPECT_EQ (request[56], 255);
   EXPECT_EQ (request[57], 255);

   // Gateway: 192.168.1.1 - see cfg_init()
   EXPECT_EQ (request[58], 192);
   EXPECT_EQ (request[59], 168);
   EXPECT_EQ (request[60], 1);
   EXPECT_EQ (request[61], 1);

   // --- DeviceIDBlockRes ---
   EXPECT_EQ (request[62], 0x02); // Option: Device Properties
   EXPECT_EQ (request[63], 0x03); // Suboption: DeviceID
   EXPECT_EQ (request[64], 0x00); // BlockLength MSB
   EXPECT_EQ (request[65], 0x06); // BlockLength LSB = 6
   EXPECT_EQ (request[66], 0x00); // BlockQualifier MSB
   EXPECT_EQ (request[67], 0x00); // BlockQualifier LSB

   // DeviceID - see cfg_init()
   EXPECT_EQ (request[68], 0xFE); // VendorIDHigh
   EXPECT_EQ (request[69], 0xED); // VendorIDLow
   EXPECT_EQ (request[70], 0xBE); // DeviceIDHigh
   EXPECT_EQ (request[71], 0xEF); // DeviceIDLow

   // --- DeviceRoleBlockRes ---
   EXPECT_EQ (request[72], 0x02); // Option: Device Properties
   EXPECT_EQ (request[73], 0x04); // Suboption: DeviceRole
   EXPECT_EQ (request[74], 0x00); // BlockLength MSB
   EXPECT_EQ (request[75], 0x04); // BlockLength LSB = 4
   EXPECT_EQ (request[76], 0x00); // BlockQualifier MSB
   EXPECT_EQ (request[77], 0x00); // BlockQualifier LSB

   EXPECT_EQ (request[78], 0x01); // DeviceRoleDetails: 1 == IO device
   EXPECT_EQ (request[79], 0x00); // Padding, counted in BlockLength

   // --- DeviceInitiativeBlockRes ---
   EXPECT_EQ (request[80], 0x06); // Option: DeviceInitiative
   EXPECT_EQ (request[81], 0x01); // Suboption: DeviceInitiative
   EXPECT_EQ (request[82], 0x00); // BlockLength MSB
   EXPECT_EQ (request[83], 0x04); // BlockLength LSB = 4
   EXPECT_EQ (request[84], 0x00); // BlockQualifier MSB
   EXPECT_EQ (request[85], 0x00); // BlockQualifier LSB

   EXPECT_EQ (request[86], 0x00); // DeviceInitiative high
   EXPECT_EQ (request[87], 0x01); // DeviceInitiative low: 1 == Hello after power on
}

TEST_F (
   DcpTest,
   OurMacAddressIsSentBackOnRequest)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
      // -----------------------------------------------------------------------
      // Ethernet header
      // -----------------------------------------------------------------------
      0x12, 0x34, 0x00, 0x78, 0x90, 0xAB,  // Destination MAC (our MAC)
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55,  // Source MAC (some other device)
      0x88, 0x92,                          // EtherType (PROFINET)

      // -----------------------------------------------------------------------
      // Profinet DCP Header
      // -----------------------------------------------------------------------
      0xFE, 0xFD,                          // FrameID: DCP Get Request (0xFEFD)
      0x03,                                // Service ID: Get
      0x00,                                // Service Type: Request (0)
      0x12, 0x34, 0x56, 0x78,              // XID (Transaction ID, arbitrary)
      0x00,                                // ResponseDelayFactor (not used)
      0x00,                                // Padding
      0x00, 0x02,                          // DCP Data Length (2 bytes)

      // -----------------------------------------------------------------------
      // DCP Block: IPOption / SuboptionMACAddress (MACAddressType)
      // -----------------------------------------------------------------------
      0x01,                                // Option: IP
      0x01                                 // Suboption: MACAddress
   };
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_GE (mock_os_data.eth_send_count, 1);
   EXPECT_EQ (mock_os_data.eth_send_copy[0].len, 38);
   const uint8_t * response = mock_os_data.eth_send_copy[0].data;

   // --- Ethernet header ---
   EXPECT_EQ (response[0], 0x00); // Destination MAC (source of request)
   EXPECT_EQ (response[1], 0x11);
   EXPECT_EQ (response[2], 0x22);
   EXPECT_EQ (response[3], 0x33);
   EXPECT_EQ (response[4], 0x44);
   EXPECT_EQ (response[5], 0x55);

   EXPECT_EQ (response[6], 0x12); // Source MAC (our MAC)
   EXPECT_EQ (response[7], 0x34);
   EXPECT_EQ (response[8], 0x00);
   EXPECT_EQ (response[9], 0x78);
   EXPECT_EQ (response[10], 0x90);
   EXPECT_EQ (response[11], 0xAB);

   EXPECT_EQ (response[12], 0x88); // EtherType
   EXPECT_EQ (response[13], 0x92);

   // --- Profinet Frame ID ---
   EXPECT_EQ (response[14], 0xFE);
   EXPECT_EQ (response[15], 0xFD); // DCP Get Response

   // --- DCP Header ---
   EXPECT_EQ (response[16], 0x03); // Service ID: Get
   EXPECT_EQ (response[17], 0x01); // Service Type: Response

   // XID should match request
   EXPECT_EQ (response[18], 0x12);
   EXPECT_EQ (response[19], 0x34);
   EXPECT_EQ (response[20], 0x56);
   EXPECT_EQ (response[21], 0x78);

   EXPECT_EQ (response[22], 0x00); // Padding
   EXPECT_EQ (response[23], 0x00); // Padding

   // --- DCP Data Length ---
   EXPECT_EQ (response[24], 0x00); // MSB
   EXPECT_EQ (response[25], 0x0C); // LSB = 12 bytes

   // --- DCP Block: IPOption / SuboptionMACAddress ---
   EXPECT_EQ (response[26], 0x01); // Option: IP
   EXPECT_EQ (response[27], 0x01); // Suboption: MACAddress
   EXPECT_EQ (response[28], 0x00); // BlockLength MSB
   EXPECT_EQ (response[29], 0x08); // BlockLength LSB = 8 (BlockQualifier + MAC)
   EXPECT_EQ (response[30], 0x00); // BlockQualifier MSB
   EXPECT_EQ (response[31], 0x00); // BlockQualifier LSB

   // MAC Address
   EXPECT_EQ (response[32], 0x12);
   EXPECT_EQ (response[33], 0x34);
   EXPECT_EQ (response[34], 0x00);
   EXPECT_EQ (response[35], 0x78);
   EXPECT_EQ (response[36], 0x90);
   EXPECT_EQ (response[37], 0xAB);
}

TEST_F (
   DcpTest,
   OurFullIpSuiteIsSentBackOnRequest)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
      // -----------------------------------------------------------------------
      // Ethernet header
      // -----------------------------------------------------------------------
      0x12, 0x34, 0x00, 0x78, 0x90, 0xAB,  // Destination MAC (our MAC)
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55,  // Source MAC (some other device)
      0x88, 0x92,                          // EtherType (PROFINET)

      // -----------------------------------------------------------------------
      // Profinet DCP Header
      // -----------------------------------------------------------------------
      0xFE, 0xFD,                          // FrameID: DCP Get Request (0xFEFD)
      0x03,                                // Service ID: Get
      0x00,                                // Service Type: Request (0)
      0x12, 0x34, 0x56, 0x78,              // XID (Transaction ID, arbitrary)
      0x00,                                // ResponseDelayFactor (not used)
      0x00,                                // Padding
      0x00, 0x02,                          // DCP Data Length (2 bytes)

      // -----------------------------------------------------------------------
      // DCP Block: IPOption / SuboptionFullIPSuite (FullIPSuiteType)
      // -----------------------------------------------------------------------
      0x01,                                // Option: IP
      0x03                                 // Suboption: FullIPSuite
   };
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_GE (mock_os_data.eth_send_count, 1);
   EXPECT_EQ (mock_os_data.eth_send_copy[0].len, 60);
   const uint8_t * response = mock_os_data.eth_send_copy[0].data;

   // --- Ethernet header ---
   EXPECT_EQ (response[0], 0x00); // Destination MAC (source of request)
   EXPECT_EQ (response[1], 0x11);
   EXPECT_EQ (response[2], 0x22);
   EXPECT_EQ (response[3], 0x33);
   EXPECT_EQ (response[4], 0x44);
   EXPECT_EQ (response[5], 0x55);

   EXPECT_EQ (response[6], 0x12); // Source MAC (our MAC)
   EXPECT_EQ (response[7], 0x34);
   EXPECT_EQ (response[8], 0x00);
   EXPECT_EQ (response[9], 0x78);
   EXPECT_EQ (response[10], 0x90);
   EXPECT_EQ (response[11], 0xAB);

   EXPECT_EQ (response[12], 0x88); // EtherType
   EXPECT_EQ (response[13], 0x92);

   // --- Profinet Frame ID ---
   EXPECT_EQ (response[14], 0xFE);
   EXPECT_EQ (response[15], 0xFD); // DCP Get Response

   // --- DCP Header ---
   EXPECT_EQ (response[16], 0x03); // Service ID: Get
   EXPECT_EQ (response[17], 0x01); // Service Type: Response

   // XID should match request
   EXPECT_EQ (response[18], 0x12);
   EXPECT_EQ (response[19], 0x34);
   EXPECT_EQ (response[20], 0x56);
   EXPECT_EQ (response[21], 0x78);

   EXPECT_EQ (response[22], 0x00); // ResponseDelayFactor
   EXPECT_EQ (response[23], 0x00); // Padding

   // --- DCP Data Length ---
   EXPECT_EQ (response[24], 0x00); // MSB
   EXPECT_EQ (response[25], 0x22); // LSB = 34 bytes

   // --- DCP Block: IPOption / SuboptionFullIPSuite (skip BlockInfo) ---
   EXPECT_EQ (response[26], 0x01); // Option: IP
   EXPECT_EQ (response[27], 0x03); // Suboption: FullIPSuite
   EXPECT_EQ (response[28], 0x00); // BlockLength MSB
   EXPECT_EQ (response[29], 0x1E); // BlockLength LSB = 30 bytes (BlockInfo
                                   // + FullIPSuite)

   // --- FullIPSuite content ---
   EXPECT_EQ (response[32], 192); // IP Address - see cfg_init()
   EXPECT_EQ (response[33], 168);
   EXPECT_EQ (response[34], 1);
   EXPECT_EQ (response[35], 171);

   EXPECT_EQ (response[36], 255); // Subnet Mask - see cfg_init()
   EXPECT_EQ (response[37], 255);
   EXPECT_EQ (response[38], 255);
   EXPECT_EQ (response[39], 255);

   EXPECT_EQ (response[40], 192); // Gateway - see cfg_init()
   EXPECT_EQ (response[41], 168);
   EXPECT_EQ (response[42], 1);
   EXPECT_EQ (response[43], 1);

   EXPECT_EQ (response[44], 0); // DNS1 - no address
   EXPECT_EQ (response[45], 0);
   EXPECT_EQ (response[46], 0);
   EXPECT_EQ (response[47], 0);

   EXPECT_EQ (response[48], 0); // DNS2 - no address
   EXPECT_EQ (response[49], 0);
   EXPECT_EQ (response[50], 0);
   EXPECT_EQ (response[51], 0);

   EXPECT_EQ (response[52], 0); // DNS3 - no address
   EXPECT_EQ (response[53], 0);
   EXPECT_EQ (response[54], 0);
   EXPECT_EQ (response[55], 0);

   EXPECT_EQ (response[56], 0); // DNS4 - no address
   EXPECT_EQ (response[57], 0);
   EXPECT_EQ (response[58], 0);
   EXPECT_EQ (response[59], 0);
}

TEST_F (
   DcpTest,
   OurFullIPSuiteIsChangedOnRequest)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
      // --- Ethernet Header ---
      0x12,0x34,0x00,0x78,0x90,0xab,   // Destination MAC
      0x00,0x11,0x22,0x33,0x44,0x55,   // Source MAC
      0x88,0x92,                       // EtherType: Profinet

      // --- Profinet Frame ID ---
      0xfe,0xfd,                       // Frame ID: DCP Set Request

      // --- DCP Header ---
      0x04,             // Service ID: Set
      0x00,             // Service Type: Request
      0x12,0x34,0x56,0x78, // XID
      0x00,0x00,        // Padding
      0x00,0x22,        // DCP Data Length = 34 bytes

      // --- DCP Block: IPOption / SuboptionFullIPSuite ---
      0x01,             // Option: IP
      0x03,             // Suboption: FullIPSuite
      0x00,0x1E,        // Block Length = 30 bytes
      0x00,0x00,        // Block Qualifier
      0xc0,0xa8,0x01,0x64,   // IP Address
      0xff,0xff,0xff,0x00,   // Subnet Mask
      0xc0,0xa8,0x01,0x01,   // Gateway
      0x08,0x08,0x08,0x08,   // DNS1: 8.8.8.8
      0x00,0x00,0x00,0x00,   // DNS2: no address
      0x00,0x00,0x00,0x00,   // DNS3: no address
      0x00,0x00,0x00,0x00,   // DNS4: no address
   };
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_GE (mock_os_data.eth_send_count, 1);
   EXPECT_EQ (mock_os_data.eth_send_copy[0].len, 34);
   const uint8_t * response = mock_os_data.eth_send_copy[0].data;

   // --- Ethernet Header ---
   EXPECT_EQ (response[0], 0x00); // Dest MAC byte 0
   EXPECT_EQ (response[1], 0x11); // Dest MAC byte 1
   EXPECT_EQ (response[2], 0x22); // Dest MAC byte 2
   EXPECT_EQ (response[3], 0x33); // Dest MAC byte 3
   EXPECT_EQ (response[4], 0x44); // Dest MAC byte 4
   EXPECT_EQ (response[5], 0x55); // Dest MAC byte 5
   EXPECT_EQ (response[6], 0x12);  // Src MAC byte 0
   EXPECT_EQ (response[7], 0x34);  // Src MAC byte 1
   EXPECT_EQ (response[8], 0x00);  // Src MAC byte 2
   EXPECT_EQ (response[9], 0x78);  // Src MAC byte 3
   EXPECT_EQ (response[10], 0x90); // Src MAC byte 4
   EXPECT_EQ (response[11], 0xab); // Src MAC byte 5
   EXPECT_EQ (response[12], 0x88); // EtherType byte 0
   EXPECT_EQ (response[13], 0x92); // EtherType byte 1

   // --- Profinet Frame ID ---
   EXPECT_EQ (response[14], 0xfe); // Frame ID byte 0
   EXPECT_EQ (response[15], 0xfd); // Frame ID byte 1

   // --- DCP Header ---
   EXPECT_EQ (response[16], 0x04); // Service ID: Set
   EXPECT_EQ (response[17], 0x01); // Service Type: Response
   EXPECT_EQ (response[18], 0x12); // XID byte 0
   EXPECT_EQ (response[19], 0x34); // XID byte 1
   EXPECT_EQ (response[20], 0x56); // XID byte 2
   EXPECT_EQ (response[21], 0x78); // XID byte 3
   EXPECT_EQ (response[22], 0x00); // Padding byte 0
   EXPECT_EQ (response[23], 0x00); // Padding byte 1
   EXPECT_EQ (response[24], 0x00); // DCP Data Length byte 0
   EXPECT_EQ (response[25], 0x08); // DCP Data Length byte 1 (7
                                                    // bytes + 1 padding)

   // --- DCP SetResBlock ---
   EXPECT_EQ (response[26], 0x05); // ControlOption
   EXPECT_EQ (response[27], 0x04); // SuboptionResponse
   EXPECT_EQ (response[28], 0x00); // DCP Block Length byte 0
   EXPECT_EQ (response[29], 0x03); // DCP Block Length byte 1
   EXPECT_EQ (response[30], 0x01); // IPParameterType: IPOption
   EXPECT_EQ (response[31], 0x03); // FullIPSuiteType: SuboptionFullIPSuite
   EXPECT_EQ (response[32], 0x00); // BlockError: 0 = OK

   // --- Padding byte for 16-bit alignment ---
   EXPECT_EQ (response[33], 0x00); // Padding byte
}

TEST_F (
   DcpTest,
   OurNameOfStationIsChangedOnRequest)
{
   pnal_buf_t * p_buf;
   int ret;
   const uint8_t request[] =
   {
      // --- Ethernet Header ---
      0x12, 0x34, 0x00, 0x78, 0x90, 0xAB,    // Destination MAC
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55,    // Source MAC
      0x88, 0x92,                            // EtherType: Profinet

      // --- Profinet Frame ID ---
      0xFE, 0xFD,                            // Frame ID: DCP Set

      // --- DCP Header ---
      0x04,                                  // Service ID: Set
      0x00,                                  // Service Type: Request
      0x12, 0x34, 0x56, 0x78,                // XID
      0x00, 0x00,                            // Padding
      0x00, 0x27,                            // DCP Data Length = 39 byte

      // --- DCP Set Block ---
      0x02,                                  // Option: Device Properties
      0x02,                                  // Suboption: NameOfStation
      0x00, 0x23,                            // BlockLength = 35 bytes
      0x00, 0x00,                            // BlockQualifier
      'd', 'e', 'v', 'i', 'c', 'e', '-', '1', '.',
      'm', 'a', 'c', 'h', 'i', 'n', 'e', '-', '1', '.',
      'p', 'l', 'a', 'n', 't', '-', '1', '.',
      'v', 'e', 'n', 'd', 'o', 'r' // NameOfStation = "device-1.machine-1.plant-1.vendor"
   };
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   pnal_buf_write (p_buf, 0, request, sizeof (request));
   pnal_buf_set_size (p_buf, sizeof (request));

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_GE (mock_os_data.eth_send_count, 1);
   EXPECT_EQ (mock_os_data.eth_send_copy[0].len, 34);
   const uint8_t * response = mock_os_data.eth_send_copy[0].data;

   // --- Ethernet Header ---
   EXPECT_EQ ( response[0],  0x00 ); // Dest MAC byte 0
   EXPECT_EQ ( response[1],  0x11 ); // Dest MAC byte 1
   EXPECT_EQ ( response[2],  0x22 ); // Dest MAC byte 2
   EXPECT_EQ ( response[3],  0x33 ); // Dest MAC byte 3
   EXPECT_EQ ( response[4],  0x44 ); // Dest MAC byte 4
   EXPECT_EQ ( response[5],  0x55 ); // Dest MAC byte 5
   EXPECT_EQ ( response[6],  0x12 ); // Src MAC byte 0
   EXPECT_EQ ( response[7],  0x34 ); // Src MAC byte 1
   EXPECT_EQ ( response[8],  0x00 ); // Src MAC byte 2
   EXPECT_EQ ( response[9],  0x78 ); // Src MAC byte 3
   EXPECT_EQ ( response[10], 0x90 ); // Src MAC byte 4
   EXPECT_EQ ( response[11], 0xAB ); // Src MAC byte 5
   EXPECT_EQ ( response[12], 0x88 ); // EtherType byte 0
   EXPECT_EQ ( response[13], 0x92 ); // EtherType byte 1

   // --- Profinet Frame ID ---
   EXPECT_EQ ( response[14], 0xFE ); // Frame ID byte 0
   EXPECT_EQ ( response[15], 0xFD ); // Frame ID byte 1

   // --- DCP Header ---
   EXPECT_EQ ( response[16], 0x04 ); // Service ID: Set
   EXPECT_EQ ( response[17], 0x01 ); // Service Type: Response
   EXPECT_EQ ( response[18], 0x12 ); // XID byte 0
   EXPECT_EQ ( response[19], 0x34 ); // XID byte 1
   EXPECT_EQ ( response[20], 0x56 ); // XID byte 2
   EXPECT_EQ ( response[21], 0x78 ); // XID byte 3
   EXPECT_EQ ( response[22], 0x00 ); // Padding byte 0
   EXPECT_EQ ( response[23], 0x00 ); // Padding byte 1
   EXPECT_EQ ( response[24], 0x00 ); // DCP Data Length byte 0
   EXPECT_EQ ( response[25], 0x08 ); // DCP Data Length byte 1 (includes padding)

   // --- DCP SetResBlock ---
   EXPECT_EQ (response[26], 0x05); // ControlOption
   EXPECT_EQ (response[27], 0x04); // SuboptionResponse
   EXPECT_EQ (response[28], 0x00); // DCP Block Length byte 0
   EXPECT_EQ (response[29], 0x03); // DCP Block Length byte 1
   EXPECT_EQ (response[30], 0x02); // Option: Device Properties
   EXPECT_EQ (response[31], 0x02); // Suboption: NameOfStation
   EXPECT_EQ (response[32], 0x00); // BlockError: 0 = OK

   // --- Padding byte for 16-bit alignment ---
   EXPECT_EQ (response[33], 0x00); // Padding byte
}

/**
 * Let the stack receive a frame, in an exactly sized buffer. This makes tools
 * like AddressSanitizer detect reads beyond the end of the received frame.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame            In:    The Ethernet frame
 * @param frame_len        In:    Length of the frame
 * @return  1 if the frame was handled by the stack (and the buffer freed)
 */
static int receive_frame (
   pnet_t * net,
   const uint8_t * frame,
   uint16_t frame_len)
{
   pnal_buf_t * p_buf = pnal_buf_alloc (frame_len);

   pnal_buf_write (p_buf, 0, frame, frame_len);

   return pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
}

TEST_F (DcpTest, DcpSetLastBlockAtEndOfFrame)
{
   /* The only block ends at the very end of the frame. There is no room for
    * a next block header, which must not be read. */
   // clang-format off
   uint8_t req[] = {
      // Ethernet header
      0x12, 0x34, 0x00, 0x78, 0x90, 0xab, // [00-05] Destination MAC address
      0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, // [06-11] Source MAC address
      0x88, 0x92,                         // [12-13] EtherType: PROFINET
      // DCP header
      0xfe, 0xfd,                         // [14-15] Frame ID: DCP Get/Set
      0x04,                               // [16]    Service ID: Set
      0x00,                               // [17]    Service type: Request
      0x00, 0x00, 0x00, 0x03,             // [18-21] Xid
      0x00, 0x00,                         // [22-23] Response delay
      0x00, 0x10,                         // [24-25] DCP data length: 16 bytes
      // DCP block: Name of station
      0x02, 0x02,                         // [26-27] Option: Device props, Name of station
      0x00, 0x0c,                         // [28-29] Block length: 12 (qualifier, name)
      0x00, 0x00,                         // [30-31] Block qualifier: Temporary
      0x72, 0x74, 0x2d, 0x6c, 0x61,       // [32-36] Name: "rt-la"
      0x62, 0x73, 0x2d, 0x64, 0x65        // [37-41] Name: "bs-de"
   };
   // clang-format on

   mock_clear();
   EXPECT_EQ (receive_frame (net, req, sizeof (req)), 1);
   EXPECT_STREQ (net->cmina_current_dcp_ase.station_name, "rt-labs-de");
}

TEST_F (DcpTest, DcpSetTruncatedBlockHeader)
{
   /* DCP data length claims 1 byte after the first block, which is not
    * enough for a block header. The first block is still handled. */
   // clang-format off
   uint8_t req[] = {
      // Ethernet header
      0x12, 0x34, 0x00, 0x78, 0x90, 0xab, // [00-05] Destination MAC address
      0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, // [06-11] Source MAC address
      0x88, 0x92,                         // [12-13] EtherType: PROFINET
      // DCP header
      0xfe, 0xfd,                         // [14-15] Frame ID: DCP Get/Set
      0x04,                               // [16]    Service ID: Set
      0x00,                               // [17]    Service type: Request
      0x00, 0x00, 0x00, 0x03,             // [18-21] Xid
      0x00, 0x00,                         // [22-23] Response delay
      0x00, 0x11,                         // [24-25] DCP data length: 17 bytes
      // DCP block: Name of station
      0x02, 0x02,                         // [26-27] Option: Device props, Name of station
      0x00, 0x0c,                         // [28-29] Block length: 12 (qualifier, name)
      0x00, 0x00,                         // [30-31] Block qualifier: Temporary
      0x72, 0x74, 0x2d, 0x6c, 0x61,       // [32-36] Name: "rt-la"
      0x62, 0x73, 0x2d, 0x64, 0x65,       // [37-41] Name: "bs-de"
      0x00                                // [42]    Extra byte, too short for block header
   };
   // clang-format on

   mock_clear();
   EXPECT_EQ (receive_frame (net, req, sizeof (req)), 1);
   EXPECT_STREQ (net->cmina_current_dcp_ase.station_name, "rt-labs-de");
}

TEST_F (DcpTest, DcpSetBlockShorterThanQualifier)
{
   /* Signal block with block length 1, which is too short to hold the block
    * qualifier. It must be rejected, so the LED is not flashed. */
   // clang-format off
   uint8_t req_len_1[] = {
      // Ethernet header
      0x12, 0x34, 0x00, 0x78, 0x90, 0xab, // [00-05] Destination MAC address
      0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, // [06-11] Source MAC address
      0x88, 0x92,                         // [12-13] EtherType: PROFINET
      // DCP header
      0xfe, 0xfd,                         // [14-15] Frame ID: DCP Get/Set
      0x04,                               // [16]    Service ID: Set
      0x00,                               // [17]    Service type: Request
      0x00, 0x00, 0x00, 0x04,             // [18-21] Xid
      0x00, 0x00,                         // [22-23] Response delay
      0x00, 0x06,                         // [24-25] DCP data length: 6 bytes
      // DCP block: Signal
      0x05, 0x03,                         // [26-27] Option: Control, suboption: Signal
      0x00, 0x01,                         // [28-29] Block length: 1 (no room for qualifier)
      0x00,                               // [30]    Block value
      0x00                                // [31]    Padding
   };
   // clang-format on

   mock_clear();
   EXPECT_EQ (receive_frame (net, req_len_1, sizeof (req_len_1)), 1);
   run_stack (2 * 1000 * 1000);
   EXPECT_EQ (appdata.call_counters.led_on_calls, 0);
}

TEST_F (DcpTest, DcpSetTooShortFrames)
{
   /* Frames that end inside the DCP header or the first block header */
   // clang-format off
   uint8_t req[30] = {
      // Ethernet header
      0x12, 0x34, 0x00, 0x78, 0x90, 0xab, // [00-05] Destination MAC address
      0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, // [06-11] Source MAC address
      0x88, 0x92,                         // [12-13] EtherType: PROFINET
      // DCP header
      0xfe, 0xfd,                         // [14-15] Frame ID: DCP Get/Set
      0x04,                               // [16]    Service ID: Set
      0x00,                               // [17]    Service type: Request
      0x00, 0x00, 0x00, 0x03,             // [18-21] Xid
      0x00, 0x00,                         // [22-23] Response delay
      0x00, 0x00,                         // [24-25] DCP data length: 0 bytes
      0x00, 0x00, 0x00, 0x00              // [26-29] First block header (frame is cut off)
   };
   // clang-format on
   uint16_t len;

   for (len = 16; len < sizeof (req); len++)
   {
      mock_clear();
      EXPECT_EQ (receive_frame (net, req, len), 1);
   }
}

TEST_F (DcpTest, DcpIdentifyTooShortFrames)
{
   /* Frames that end inside the DCP header or the first block header */
   // clang-format off
   uint8_t req[30] = {
      // Ethernet header
      0x01, 0x0e, 0xcf, 0x00, 0x00, 0x00, // [00-05] Destination MAC: Identify multicast
      0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, // [06-11] Source MAC address
      0x88, 0x92,                         // [12-13] EtherType: PROFINET
      // DCP header
      0xfe, 0xfe,                         // [14-15] Frame ID: DCP Identify request
      0x05,                               // [16]    Service ID: Identify
      0x00,                               // [17]    Service type: Request
      0x00, 0x00, 0x00, 0x02,             // [18-21] Xid
      0x00, 0x01,                         // [22-23] Response delay factor
      0x00, 0x00,                         // [24-25] DCP data length: 0 bytes
      0x00, 0x00, 0x00, 0x00              // [26-29] First block header (frame is cut off)
   };
   // clang-format on
   uint16_t len;

   for (len = 16; len < sizeof (req); len++)
   {
      mock_clear();
      EXPECT_EQ (receive_frame (net, req, len), 1);
   }
}

TEST_F (DcpTest, DcpIdentifyLastBlockAtEndOfFrame)
{
   /* Name of station filter ends at the very end of the frame */
   // clang-format off
   uint8_t req[] = {
      // Ethernet header
      0x01, 0x0e, 0xcf, 0x00, 0x00, 0x00, // [00-05] Destination MAC: Identify multicast
      0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, // [06-11] Source MAC address
      0x88, 0x92,                         // [12-13] EtherType: PROFINET
      // DCP header
      0xfe, 0xfe,                         // [14-15] Frame ID: DCP Identify request
      0x05,                               // [16]    Service ID: Identify
      0x00,                               // [17]    Service type: Request
      0x00, 0x00, 0x00, 0x02,             // [18-21] Xid
      0x00, 0x01,                         // [22-23] Response delay factor
      0x00, 0x10,                         // [24-25] DCP data length: 16 bytes
      // DCP block: Name of station
      0x02, 0x02,                         // [26-27] Option: Device props, Name of station
      0x00, 0x0c,                         // [28-29] Block length: 12 (name)
      0x72, 0x74, 0x2d, 0x6c, 0x61, 0x62, // [30-35] Name: "rt-lab"
      0x73, 0x2d, 0x64, 0x65, 0x6d, 0x6f  // [36-41] Name: "s-demo"
   };
   // clang-format on

   /* Set the name that the request is looking for */
   EXPECT_EQ (receive_frame (net, set_name_req, sizeof (set_name_req)), 1);

   mock_clear();
   EXPECT_EQ (receive_frame (net, req, sizeof (req)), 1);
   run_stack (1000 * 1000);
   EXPECT_EQ (mock_os_data.eth_send_count, 1); /* Identify response */
}

TEST_F (DcpUnitTest, DcpCalculateDelay)
{
   pnet_ethaddr_t mac_address = {0};
   const uint16_t step = 10000; /* Output resolution in microseconds */

   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 2), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 10), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6400), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6401), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0xFFFF), 0U);

   mac_address.addr[5] = 0x01;
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 2), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 10), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6400), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6401), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0xFFFF), 0U);

   mac_address.addr[5] = 0x02;
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 2), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 10), 2U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 2U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 2U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6400), 2U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6401), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0xFFFF), 0U);

   mac_address.addr[5] = 0xC7; /* 199 */
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 99U * step);

   mac_address.addr[5] = 0xFF; /* 255 */
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 2), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 10), 5U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 55U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 252), 3U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 253), 2U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 254), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 255), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 256), 255U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 257), 255U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 258), 255U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 255U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6400), 255U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6401), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0xFFFF), 0U);
   mac_address.addr[5] = 0x00;

   mac_address.addr[4] = 0x01; /* 256 */
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 2), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 10), 6U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 56U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 253), 3U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 254), 2U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 255), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 256), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 257), 256U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 258), 256U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 256U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 256U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6400), 256U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6401), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0xFFFF), 0U);

   mac_address.addr[4] = 0x02; /* 512 */
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 2), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 10), 2U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 12U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 512U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6400), 512U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6401), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0xFFFF), 0U);

   mac_address.addr[4] = 0xFF; /* 65280 */
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 2), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 10), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 80U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 280U * step);
   ASSERT_EQ (
      pf_dcp_calculate_response_delay (&mac_address, 6400),
      1280U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6401), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0xFFFF), 0U);

   mac_address.addr[4] = 0xFF;
   mac_address.addr[5] = 0xFF; /* 65535 */
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 2), 1U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 10), 5U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 100), 35U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 1000), 535U * step);
   ASSERT_EQ (
      pf_dcp_calculate_response_delay (&mac_address, 6400),
      1535U * step);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 6401), 0U);
   ASSERT_EQ (pf_dcp_calculate_response_delay (&mac_address, 0xFFFF), 0U);
}

TEST_F (DcpUnitTest, DcpCheckDestinationAddress)
{
   pnet_ethaddr_t local_mac_address = {0x12, 0x34, 0x00, 0x78, 0x90, 0xab};
   pnet_ethaddr_t unknown_mac_address = {0x12, 0x34, 0x00, 0x78, 0x95, 0xab};
   pnet_ethaddr_t multicast_mac_address = {0x01, 0x0e, 0xcf, 0x00, 0x00, 0x00};

   ASSERT_EQ (
      pf_dcp_check_destination_address (&local_mac_address, &local_mac_address),
      true);
   ASSERT_EQ (
      pf_dcp_check_destination_address (
         &local_mac_address,
         &unknown_mac_address),
      false);

   ASSERT_EQ (
      pf_dcp_check_destination_address (
         &local_mac_address,
         &multicast_mac_address),
      false);
}


#if PNET_OPTION_MC_PRUNING
#define PF_DCP_MC_MAC_ADD_FILTER_BASE 0x20
TEST_F (DcpTest, DcpFilterDestinationAddress)
{
   pnet_ethaddr_t multicast_mac_address = {0x01, 0x0e, 0xcf, 0x00, 0x00, 0x00};

   /* Assumes the default station name is "", which produces md5 output
   * d41d8cd98f00b204e9800998ecf8427e, which & 0x1f becomes 0x1e */
   uint8_t selector = PF_DCP_MC_MAC_ADD_FILTER_BASE + 0x1e;

   bool actual_result;
   bool expected_result;
   uint16_t i;

   /* Validate the assumption that the default station name is empty */
   ASSERT_EQ (net->cmina_current_dcp_ase.station_name[0], '\0');

   TEST_TRACE ("\nFiltering DCP Identify destination addresses\n");

   /* Should always respond to 01:0E:CF:00:00:00 */
   multicast_mac_address.addr[5] = 0;
   ASSERT_EQ (
      pf_dcp_filter_destination_address (net, &multicast_mac_address),
      true);

   /* Should only respond to 01:0E:CF:00:00:{selector} */
   i = PF_DCP_MC_MAC_ADD_FILTER_BASE;
   for (; i < PF_DCP_MC_MAC_ADD_FILTER_BASE + 0x1f; ++i)
   {
      multicast_mac_address.addr[5] = i;
      expected_result = (i == 0) || (i == selector);
      actual_result = pf_dcp_filter_destination_address (
         net,
         &multicast_mac_address);

      ASSERT_EQ (actual_result, expected_result);
   }

   TEST_TRACE ("Repeating with station name \"rt-labs-demo\"\n");
   strncpy (
      net->cmina_current_dcp_ase.station_name,
      "rt-labs-demo",
      PNET_STATION_NAME_MAX_SIZE);
   net->cmina_commit_ip_suite = true;
   pf_cmina_dcp_set_commit (net);

   /* MD5 => 2366899b24000f03cec5e358f99e5d04 */
   selector = PF_DCP_MC_MAC_ADD_FILTER_BASE + 0x04;

   /* Should always respond to 01:0E:CF:00:00:00 */
   multicast_mac_address.addr[5] = 0;
   ASSERT_EQ (
      pf_dcp_filter_destination_address (net, &multicast_mac_address),
      true);

   /* Should only respond to 01:0E:CF:00:00:{selector} */
   i = PF_DCP_MC_MAC_ADD_FILTER_BASE;
   for (; i < PF_DCP_MC_MAC_ADD_FILTER_BASE + 0x1f; ++i)
   {
      multicast_mac_address.addr[5] = i;
      expected_result = (i == 0) || (i == selector);
      actual_result = pf_dcp_filter_destination_address (
         net,
         &multicast_mac_address);

      ASSERT_EQ (actual_result, expected_result);
   }

   memset (
      net->cmina_current_dcp_ase.station_name,
      0,
      PNET_STATION_NAME_MAX_SIZE);
   net->cmina_commit_ip_suite = true;
   pf_cmina_dcp_set_commit (net);
}
#endif
