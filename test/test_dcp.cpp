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
 * Checks only function return codes. No checking of the sent Ethernet frames is
 * done.
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
   memcpy (p_buf->payload, get_name_req, sizeof (get_name_req));
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
   memcpy (p_buf->payload, get_wrong_length_req, sizeof (get_wrong_length_req));
   p_buf->len = sizeof (get_wrong_length_req);

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
   memcpy (p_buf->payload, set_name_req, sizeof (set_name_req));
   p_buf->len = sizeof (set_name_req);

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock set IP request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   memcpy (p_buf->payload, set_ip_req, sizeof (set_ip_req));
   p_buf->len = sizeof (set_ip_req);

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock set ident request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   memcpy (p_buf->payload, ident_req, sizeof (ident_req));
   p_buf->len = sizeof (ident_req);

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock factory reset request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   memcpy (p_buf->payload, factory_reset_req, sizeof (factory_reset_req));
   p_buf->len = sizeof (factory_reset_req);

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock flash LED request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   memcpy (p_buf->payload, signal_req, sizeof (signal_req));
   p_buf->len = sizeof (signal_req);

   ret = pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
   EXPECT_EQ (ret, 1);
   /* Wait for LED to flash three times at 1 Hz */
   run_stack (4 * 1000 * 1000);

   EXPECT_EQ (
      mock_os_data.eth_send_count,
      9 + (PNET_MAX_PHYSICAL_PORTS - 1) * 4);
   EXPECT_EQ (mock_os_data.set_ip_suite_count, 3);

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

   memcpy (p_buf->payload, frame, frame_len);
   p_buf->len = frame_len;

   return pf_eth_recv (mock_os_data.eth_if_handle, net, p_buf);
}

TEST_F (DcpTest, DcpSetLastBlockAtEndOfFrame)
{
   /* The only block ends at the very end of the frame. There is no room for
    * a next block header, which must not be read. */
   uint8_t req[] = {
      0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
      0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
      0x00, 0x10, 0x02, 0x02, 0x00, 0x0c, 0x00, 0x00, 0x72, 0x74, 0x2d, 0x6c,
      0x61, 0x62, 0x73, 0x2d, 0x64, 0x65};

   mock_clear();
   EXPECT_EQ (receive_frame (net, req, sizeof (req)), 1);
   EXPECT_STREQ (net->cmina_current_dcp_ase.station_name, "rt-labs-de");
}

TEST_F (DcpTest, DcpSetTruncatedBlockHeader)
{
   /* DCP data length claims 1 byte after the first block, which is not
    * enough for a block header. The first block is still handled. */
   uint8_t req[] = {
      0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
      0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
      0x00, 0x11, 0x02, 0x02, 0x00, 0x0c, 0x00, 0x00, 0x72, 0x74, 0x2d, 0x6c,
      0x61, 0x62, 0x73, 0x2d, 0x64, 0x65, 0x00};

   mock_clear();
   EXPECT_EQ (receive_frame (net, req, sizeof (req)), 1);
   EXPECT_STREQ (net->cmina_current_dcp_ase.station_name, "rt-labs-de");
}

TEST_F (DcpTest, DcpSetBlockShorterThanQualifier)
{
   /* Signal block with block length 1, which is too short to hold the block
    * qualifier. It must be rejected, so the LED is not flashed. */
   uint8_t req_len_1[] = {
      0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
      0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
      0x00, 0x06, 0x05, 0x03, 0x00, 0x01, 0x00, 0x00};

   mock_clear();
   EXPECT_EQ (receive_frame (net, req_len_1, sizeof (req_len_1)), 1);
   run_stack (2 * 1000 * 1000);
   EXPECT_EQ (appdata.call_counters.led_on_calls, 0);
}

TEST_F (DcpTest, DcpSetTooShortFrames)
{
   /* Frames that end inside the DCP header or the first block header */
   uint8_t req[30] = {
      0x12, 0x34, 0x00, 0x78, 0x90, 0xab, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
      0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
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
   uint8_t req[30] = {
      0x01, 0x0e, 0xcf, 0x00, 0x00, 0x00, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
      0x88, 0x92, 0xfe, 0xfe, 0x05, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
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
   uint8_t req[] = {
      0x01, 0x0e, 0xcf, 0x00, 0x00, 0x00, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
      0x88, 0x92, 0xfe, 0xfe, 0x05, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01,
      0x00, 0x10, 0x02, 0x02, 0x00, 0x0c, 0x72, 0x74, 0x2d, 0x6c, 0x61, 0x62,
      0x73, 0x2d, 0x64, 0x65, 0x6d, 0x6f};

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
