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
 * @brief Integration testing of diagnostics.
 *
 * For example:
 *  - pnet_diag_std_add()
 *  - pnet_diag_std_update()
 *  - pnet_diag_std_remove()
 *  - pnet_diag_usi_add()
 *  - pnet_diag_usi_update()
 *  - pnet_diag_usi_remove()
 *
 */

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

class DiagTest : public PnetIntegrationTest
{
};

// clang-format off

static uint8_t connect_req[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x86, 0x01, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0x00, 0x00, 0x72, 0x01,
 0x00, 0x00, 0x24, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0x01, 0x00, 0x00, 0x01, 0x01,
 0x00, 0x42, 0x01, 0x00, 0x00, 0x01, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, 0xde, 0xa0,
 0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x01, 0x40, 0x00,
 0x00, 0x11, 0x02, 0x58, 0x88, 0x92, 0x00, 0x0c, 0x72, 0x74, 0x2d, 0x6c, 0x61, 0x62, 0x73, 0x2d,
 0x64, 0x65, 0x6d, 0x6f, 0x01, 0x02, 0x00, 0x50, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x88, 0x92,
 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x80, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
 0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03,
 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x05, 0x01, 0x02, 0x00, 0x50, 0x01, 0x00, 0x00, 0x02,
 0x00, 0x02, 0x88, 0x92, 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x80, 0x00, 0x00, 0x20, 0x00, 0x01,
 0x00, 0x01, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01,
 0x00, 0x00, 0x80, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03, 0x01, 0x04, 0x00, 0x3c,
 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01,
 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x80, 0x01,
 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x04, 0x00, 0x26,
 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01,
 0x00, 0x02, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x16, 0x01, 0x00, 0x00, 0x01, 0x88, 0x92,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x02, 0x00, 0xc8, 0xc0, 0x00, 0xa0, 0x00
};

static uint8_t release_req[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x20, 0x00,
 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x14,
 0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00
};

static uint8_t prm_end_req[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x20, 0x00,
 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x10,
 0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
};

static uint8_t appl_rdy_rsp[] =
{
                                                             0x04, 0x02, 0x0a, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x00, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0x79, 0x56, 0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
 0x07, 0x08, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
 0x00, 0x00, 0xdc, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x81, 0x12,
 0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00
};

static uint8_t data_packet_good_iops_good_iocs[] =
{
 0x1e, 0x30, 0x6c, 0xa2, 0x45, 0x5e, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, 0x88, 0x92, 0x80, 0x00,
 0x80, 0x80, 0x80, 0x80, 0x23, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6, 0x35, 0x00
};

// clang-format on

TEST_F (DiagTest, DiagRunTest)
{
   int ret;
   bool new_flag = false;
   uint8_t in_data[10];
   uint16_t in_len = sizeof (in_data);
   uint8_t out_data[] = {
      0x33, /* Slot 1, subslot 1 Data */
   };
   uint8_t iops = PNET_IOXS_BAD;
   uint8_t iocs = PNET_IOXS_BAD;
   uint32_t ix;
   pnet_diag_source_t diag_source = {
      .api = TEST_API_IDENT,
      .slot = TEST_SLOT_IDENT,
      .subslot = TEST_SUBSLOT_IDENT,
      .ch = TEST_CHANNEL_IDENT,
      .ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL,
      .ch_direction = TEST_CHANNEL_DIRECTION};

   TEST_TRACE ("\nGenerating mock connection request\n");
   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_GT (mock_os_data.eth_send_count, 0);

   TEST_TRACE ("\nGenerating mock parameter end request\n");
   mock_set_pnal_udp_recvfrom_buffer (prm_end_req, sizeof (prm_end_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 2);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_PRMEND);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);

   TEST_TRACE ("\nSimulate application calling APPL_RDY\n");
   ret = pnet_application_ready (net, appdata.main_arep);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (appdata.call_counters.state_calls, 3);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   TEST_TRACE ("\nGenerating mock application ready response\n");
   mock_set_pnal_udp_recvfrom_buffer (appl_rdy_rsp, sizeof (appl_rdy_rsp));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 3);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   TEST_TRACE ("\nGenerating cyclic data\n");
   for (ix = 0; ix < 100; ix++)
   {
      send_data (
         data_packet_good_iops_good_iocs,
         sizeof (data_packet_good_iops_good_iocs));
      run_stack (TEST_DATA_DELAY);
   }

   TEST_TRACE ("\nTesting pnet_output_get_data_and_iops()\n");
   iops = 88; /* Something non-valid */
   in_len = sizeof (in_data);
   ret = pnet_output_get_data_and_iops (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      &new_flag,
      in_data,
      &in_len,
      &iops);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (new_flag, true);
   EXPECT_EQ (in_len, 1);
   EXPECT_EQ (in_data[0], 0x23);
   EXPECT_EQ (iops, PNET_IOXS_GOOD);

   TEST_TRACE ("\nTesting pnet_input_get_iocs()\n");
   iocs = 77; /* Something non-valid */
   ret = pnet_input_get_iocs (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      &iocs);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (new_flag, true);
   EXPECT_EQ (in_len, 1);
   EXPECT_EQ (iocs, PNET_IOXS_GOOD);

   EXPECT_EQ (appdata.call_counters.state_calls, 4);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_DATA);

   TEST_TRACE ("\nSend some data to the controller\n");
   ret = pnet_input_set_data_and_iops (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      out_data,
      sizeof (out_data),
      PNET_IOXS_GOOD);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("\nAcknowledge the reception of controller data\n");
   ret = pnet_output_set_iocs (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      PNET_IOXS_GOOD);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (appdata.call_counters.state_calls, 4);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_DATA);

   /* Send data to avoid timeout */
   send_data (
      data_packet_good_iops_good_iocs,
      sizeof (data_packet_good_iops_good_iocs));
   run_stack (TEST_DATA_DELAY);

   TEST_TRACE ("\nCreate a standard diag entry. Then update it and finally "
               "remove it.\n");
   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE_B);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("\nCreate several different severity STD diag entries. Then "
               "remove them.\n");
   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_REQUIRED,
      TEST_CHANNEL_ERRORTYPE_B,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_DEMANDED,
      TEST_CHANNEL_ERRORTYPE_C,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_QUALIFIED,
      TEST_CHANNEL_ERRORTYPE_D,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE_B,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE_C,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE_D,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("Try to remove it again\n");
   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("Remove non-existing errortype\n");
   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE_NONEXIST,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("Try to add two of the same kind\n");
   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_REQUIRED,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, 0);

   /* This is expected to succeed as the same entry will be re-used and
    * overwritten. */
   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_REQUIRED,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("Update with the same value\n");
   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_REQUIRED,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("Try to update a diag entry that does not exist\n");
   ret = pnet_diag_std_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_std_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE_NONEXIST,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("Add diag to non-existing sub-slot\n");
   diag_source.slot = TEST_SUBSLOT_NONEXIST_IDENT;
   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_REQUIRED,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, -1);
   diag_source.slot = TEST_SUBSLOT_IDENT;

   TEST_TRACE ("Try adding, updating and remove diag for illegal channel\n");
   diag_source.ch = TEST_CHANNEL_ILLEGAL;
   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_REQUIRED,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_std_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);
   diag_source.ch = TEST_CHANNEL_IDENT;

   TEST_TRACE ("\nMake sure that all parts of the diag_source are correct "
               "before removing the diagnosis.\n");
   ret = pnet_diag_std_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET);
   EXPECT_EQ (ret, 0);

   diag_source.api = TEST_API_NONEXIST_IDENT;
   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);
   diag_source.api = TEST_API_IDENT;

   diag_source.slot = TEST_SLOT_NONEXIST_IDENT;
   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);
   diag_source.slot = TEST_SLOT_IDENT;

   diag_source.subslot = TEST_SUBSLOT_NONEXIST_IDENT;
   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);
   diag_source.subslot = TEST_SUBSLOT_IDENT;

   diag_source.ch = TEST_CHANNEL_NONEXIST_IDENT;
   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);
   diag_source.ch = TEST_CHANNEL_IDENT;

   diag_source.ch_grouping = PNET_DIAG_CH_CHANNEL_GROUP;
   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);
   diag_source.ch_grouping = PNET_DIAG_CH_INDIVIDUAL_CHANNEL;

   diag_source.ch_direction = PNET_DIAG_CH_PROP_DIR_INPUT;
   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);
   diag_source.ch_direction = TEST_CHANNEL_DIRECTION;

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE_NONEXIST,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE_NONEXIST);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_std_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("Test with a manufacturer specific USI\n");
   ret = pnet_diag_usi_add (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_CUSTOM,
      5, /* Should fit in PNET_MAX_DIAG_MANUF_DATA_SIZE */
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("Update data for a manufacturer specific USI, via add()\n");
   ret = pnet_diag_usi_add (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABC1");
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("Update data for a manufacturer specific USI, via update()\n");
   ret = pnet_diag_usi_update (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABC2");
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("Update for wrong USI and wrong subslot\n");
   ret = pnet_diag_usi_update (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_NONEXIST,
      5,
      (uint8_t *)"ABC2");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_usi_update (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_NONEXIST_IDENT,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABC2");
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("Remove wrong USI\n");
   ret = pnet_diag_usi_remove (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_NONEXIST);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("Remove correct USI\n");
   ret = pnet_diag_usi_remove (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_CUSTOM);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("Add for invalid subslot\n");
   ret = pnet_diag_usi_add (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_NONEXIST_IDENT,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("Add, update and remove diagnosis for invalid USI\n");
   ret = pnet_diag_usi_add (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_INVALID,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_usi_update (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_INVALID,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_usi_remove (
      net,
      TEST_API_IDENT,
      TEST_SLOT_IDENT,
      TEST_SUBSLOT_IDENT,
      TEST_DIAG_USI_INVALID);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nUse low-level functionality. Create a standard diag entry, "
               "with QUALIFIED and FAULT. Then update it and finally "
               "remove it.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET,
      0x8003, /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
      0,
      NULL);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE_B,
      0x8003, /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
      0,
      NULL);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      0x8003 /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
   );
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("\nUse low-level functionality. Create a standard diag entry, "
               "with QUALIFIED. Then update it and finally "
               "remove it.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_QUALIFIED,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER,
      0x8003, /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
      0,
      NULL);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE_B,
      0x8003, /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
      0,
      NULL);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      0x8003 /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
   );
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("\nUse low-level functionality. Create a standard diag entry, "
               "with EXTENDED. Then update it and finally "
               "remove it.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET,
      0x8002, /* PF_USI_EXTENDED_CHANNEL_DIAGNOSIS */
      0,
      NULL);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE_B,
      0x8002, /* PF_USI_EXTENDED_CHANNEL_DIAGNOSIS */
      0,
      NULL);
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      0x8002 /* PF_USI_EXTENDED_CHANNEL_DIAGNOSIS */
   );
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("\nUse low-level functionality. Create a USI diag entry. Then "
               "update it and finally "
               "remove it.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      0, /* Channel error type */
      0, /* Ext channel error type */
      0, /* Add value */
      0, /* Qualifier */
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_update (
      net,
      &diag_source,
      0, /* Channel error type */
      0, /* Ext channel error type */
      0, /* Add value */
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABC2");
   EXPECT_EQ (ret, 0);

   ret = pnet_diag_remove (
      net,
      &diag_source,
      0, /* Channel error type */
      0, /* Ext channel error type */
      TEST_DIAG_USI_CUSTOM);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("\nError checking for low-level functionality. Invalid USI.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET,
      0x8001, /* Invalid */
      0,
      NULL);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET,
      0x8004, /* Invalid */
      0,
      NULL);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE_B,
      0x8001, /* Invalid */
      0,
      NULL);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      0x8001 /* Invalid */
   );
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE_B,
      0x8004, /* Invalid */
      0,
      NULL);
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      0x8004 /* Invalid */
   );
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nError checking for low-level functionality. Channel error "
               "type given for custom USI.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      0, /* Ext channel error type */
      0, /* Add value */
      TEST_DIAG_QUALIFIER_NOTSET,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      0, /* Extended hannel error type */
      0, /* Add value */
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_remove (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      0, /* Ext channel error type */
      TEST_DIAG_USI_CUSTOM);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nError checking for low-level functionality. Ext channel "
               "error type given for custom USI.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      0, /* Channel error type */
      TEST_DIAG_EXT_ERRTYPE,
      0, /* Add value */
      TEST_DIAG_QUALIFIER_NOTSET,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_update (
      net,
      &diag_source,
      0, /* Channel error type */
      TEST_DIAG_EXT_ERRTYPE,
      0, /* Add value */
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABC2");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_remove (
      net,
      &diag_source,
      0, /* Channel error type */
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_USI_CUSTOM);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nError checking for low-level functionality. Channel "
               "additional value given for custom USI.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      0, /* Channel error type */
      0, /* Ext channel error type */
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_update (
      net,
      &diag_source,
      0, /* Channel error type */
      0, /* Ext channel error type */
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABC2");
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nError checking for low-level functionality. Qualifier given "
               "for custom USI.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      PNET_DIAG_CH_PROP_TYPE_UNSPECIFIED,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      0, /* Channel error type */
      0, /* Ext channel error type */
      0, /* Add value */
      TEST_DIAG_QUALIFIER,
      TEST_DIAG_USI_CUSTOM,
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nError checking for low-level functionality. Manufacturer "
               "data given for standard USI.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER_NOTSET,
      0x8003, /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   ret = pnet_diag_update (
      net,
      &diag_source,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE_B,
      0x8003, /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
      5,
      (uint8_t *)"ABCD");
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nError checking for low-level functionality. Qualifier given "
               "for non-qualifyer USI.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER,
      0x8002, /* PF_USI_EXTENDED_CHANNEL_DIAGNOSIS */
      0,
      NULL);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nError checking for low-level functionality. Qualifier given "
               "for wrong severity.\n");
   ret = pnet_diag_add (
      net,
      &diag_source,
      TEST_CHANNEL_NUMBER_OF_BITS,
      PNET_DIAG_CH_PROP_MAINT_FAULT,
      TEST_CHANNEL_ERRORTYPE,
      TEST_DIAG_EXT_ERRTYPE,
      TEST_DIAG_EXT_ADDVALUE,
      TEST_DIAG_QUALIFIER,
      0x8003, /* PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS */
      0,
      NULL);
   EXPECT_EQ (ret, -1);

   TEST_TRACE ("\nGenerating mock release request\n");
   mock_set_pnal_udp_recvfrom_buffer (release_req, sizeof (release_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.release_calls, 1);
   EXPECT_EQ (appdata.call_counters.state_calls, 5);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);
}
