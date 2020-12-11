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

static uint8_t get_name_req[] = {
   0x1e, 0x30, 0x6c, 0xa2, 0x45, 0x5e, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
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
   0x1e, 0x30, 0x6c, 0xa2, 0x45, 0x5e, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
   0x00, 0x12, 0x02, 0x02, 0x00, 0x0e, 0x00, 0x00, 0x72, 0x74, 0x2d, 0x6c,
   0x61, 0x62, 0x73, 0x2d, 0x64, 0x65, 0x6d, 0x6f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t set_ip_req[] = {
   0x1e, 0x30, 0x6c, 0xa2, 0x45, 0x5e, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
   0x00, 0x18, 0x01, 0x02, 0x00, 0x0e, 0x00, 0x00, 0xc0, 0xa8, 0x01, 0xab,
   0xff, 0xff, 0xff, 0x00, 0xc0, 0xa8, 0x01, 0x01, 0x05, 0x02, 0x00, 0x02,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t factory_reset_req[] = {
   0x1e, 0x30, 0x6c, 0xa2, 0x45, 0x5e, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
   0x88, 0x92, 0xfe, 0xfd, 0x04, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
   0x00, 0x06, 0x05, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t signal_req[] = {
   0x1e, 0x30, 0x6c, 0xa2, 0x45, 0x5e, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf,
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
   ret = pf_eth_recv (net, p_buf);

   EXPECT_EQ (ret, 1);
   EXPECT_EQ (mock_os_data.eth_send_count, 2);

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
   ret = pf_eth_recv (net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock set IP request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   memcpy (p_buf->payload, set_ip_req, sizeof (set_ip_req));
   ret = pf_eth_recv (net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock set ident request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   memcpy (p_buf->payload, ident_req, sizeof (ident_req));
   ret = pf_eth_recv (net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock factory reset request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   memcpy (p_buf->payload, factory_reset_req, sizeof (factory_reset_req));
   ret = pf_eth_recv (net, p_buf);
   EXPECT_EQ (ret, 1);

   TEST_TRACE ("\nGenerating mock flash LED request\n");
   p_buf = pnal_buf_alloc (PF_FRAME_BUFFER_SIZE);
   memcpy (p_buf->payload, signal_req, sizeof (signal_req));
   ret = pf_eth_recv (net, p_buf);
   EXPECT_EQ (ret, 1);
   /* Wait for LED to flash three times at 1 Hz */
   run_stack (4 * 1000 * 1000);

   EXPECT_EQ (mock_os_data.eth_send_count, 9);
   EXPECT_EQ (mock_os_data.set_ip_suite_count, 2);

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
