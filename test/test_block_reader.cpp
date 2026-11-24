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

#include "pf_block_reader.h"
#include "pf_includes.h"

#include <gtest/gtest.h>

class BlockReaderUnitTest : public PnetUnitTest
{
};

#define VALID_PEER_PORT_NAME_LENGTH 14
#define VALID_PEER_PORT_NAME                                                   \
   {                                                                           \
      'p', 'o', 'r', 't', '-', '0', '0', '1', '-', '0', '0', '0', '0', '0'     \
   }
#define VALID_PEER_STATION_NAME_LENGTH 5
#define VALID_PEER_STATION_NAME                                                \
   {                                                                           \
      'd', 'e', 'v', '-', '2'                                                  \
   }
#define INVALID_PEER_PORT_NAME_LENGTH    16
#define INVALID_PEER_STATION_NAME_LENGTH 255

static struct
{
   uint8_t number_of_peers;
   uint8_t length_peer_port_name;
   uint8_t peer_port_name[VALID_PEER_PORT_NAME_LENGTH];
   uint8_t length_peer_station_name;
   uint8_t peer_station_name[VALID_PEER_STATION_NAME_LENGTH];
} check_peers_data_valid = {
   .number_of_peers = 1,
   .length_peer_port_name = VALID_PEER_PORT_NAME_LENGTH,
   .peer_port_name = VALID_PEER_PORT_NAME,
   .length_peer_station_name = VALID_PEER_STATION_NAME_LENGTH,
   .peer_station_name = VALID_PEER_STATION_NAME};

static struct
{
   uint8_t number_of_peers;
   uint8_t length_peer_port_name;
   uint8_t peer_port_name[INVALID_PEER_PORT_NAME_LENGTH];
   uint8_t length_peer_station_name;
   uint8_t peer_station_name[VALID_PEER_STATION_NAME_LENGTH];
} check_peers_data_invalid_peer_port_name = {
   .number_of_peers = 1,
   .length_peer_port_name = INVALID_PEER_PORT_NAME_LENGTH,
   .peer_port_name = {0},
   .length_peer_station_name = VALID_PEER_STATION_NAME_LENGTH,
   .peer_station_name = VALID_PEER_STATION_NAME};

static struct
{
   uint8_t number_of_peers;
   uint8_t length_peer_port_name;
   uint8_t peer_port_name[VALID_PEER_PORT_NAME_LENGTH];
   uint8_t length_peer_station_name;
   uint8_t peer_station_name[INVALID_PEER_STATION_NAME_LENGTH];
} check_peers_data_invalid_peer_station_name = {
   .number_of_peers = 1,
   .length_peer_port_name = VALID_PEER_PORT_NAME_LENGTH,
   .peer_port_name = VALID_PEER_PORT_NAME,
   .length_peer_station_name = INVALID_PEER_STATION_NAME_LENGTH,
   .peer_station_name = {0}};

#if PNET_OPTION_AR_VENDOR_BLOCKS
#define DUMMY_AP_STRUCTURE_IDENTIFIER 0x11223344
#define AR_VENDOR_DUMMY_AP_STRUCTURE_IDENTIFIER                                \
   {                                                                           \
      0x11, 0x22, 0x33, 0x44                                                   \
   }
#define DUMMY_API 0xaabbccdd
#define AR_VENDOR_DUMMY_API                                                    \
   {                                                                           \
      0xaa, 0xbb, 0xcc, 0xdd                                                   \
   }
#define INVALID_AR_VENDOR_BLOCK_REQ_LENGTH                                     \
   (PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH + (2 * sizeof (uint32_t)) + 2)

static struct
{
   uint8_t ap_structure_identifier[sizeof (uint32_t)];
   uint8_t api[sizeof (uint32_t)];
   uint8_t data[2];
} ar_vendor_block_req_dummy = {
   .ap_structure_identifier = AR_VENDOR_DUMMY_AP_STRUCTURE_IDENTIFIER,
   .api = AR_VENDOR_DUMMY_API,
   .data = {0x55, 0xaa}};
#endif /* PNET_OPTION_AR_VENDOR_BLOCKS */

TEST_F (BlockReaderUnitTest, BlockReaderTestGetBits)
{

   /* 11111111 11111111 11111111 11111111 */
   EXPECT_EQ (0x00000000ul, pf_get_bits (0xFFFFFFFF, 0, 0));
   EXPECT_EQ (0x00000001ul, pf_get_bits (0xFFFFFFFF, 0, 1));
   EXPECT_EQ (0x00000003ul, pf_get_bits (0xFFFFFFFF, 0, 2));
   EXPECT_EQ (0x00000007ul, pf_get_bits (0xFFFFFFFF, 0, 3));
   EXPECT_EQ (0x0000000Ful, pf_get_bits (0xFFFFFFFF, 0, 4));
   EXPECT_EQ (0x0000001Ful, pf_get_bits (0xFFFFFFFF, 0, 5));
   EXPECT_EQ (0x0000003Ful, pf_get_bits (0xFFFFFFFF, 0, 6));
   EXPECT_EQ (0x0000007Ful, pf_get_bits (0xFFFFFFFF, 0, 7));
   EXPECT_EQ (0x000000FFul, pf_get_bits (0xFFFFFFFF, 0, 8));
   EXPECT_EQ (0x7FFFFFFFul, pf_get_bits (0xFFFFFFFF, 0, 31));
   EXPECT_EQ (0xFFFFFFFFul, pf_get_bits (0xFFFFFFFF, 0, 32));
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 0, 33)); /* Illegal length */

   EXPECT_EQ (0x00000000ul, pf_get_bits (0xFFFFFFFF, 1, 0));
   EXPECT_EQ (0x00000001ul, pf_get_bits (0xFFFFFFFF, 1, 1));
   EXPECT_EQ (0x00000003ul, pf_get_bits (0xFFFFFFFF, 1, 2));
   EXPECT_EQ (0x00000007ul, pf_get_bits (0xFFFFFFFF, 1, 3));
   EXPECT_EQ (0x000000FFul, pf_get_bits (0xFFFFFFFF, 1, 8));
   EXPECT_EQ (0x7FFFFFFFul, pf_get_bits (0xFFFFFFFF, 1, 31));
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 1, 32)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 1, 33)); /* Illegal length */

   EXPECT_EQ (0x00000000ul, pf_get_bits (0xFFFFFFFF, 31, 0));
   EXPECT_EQ (0x00000001ul, pf_get_bits (0xFFFFFFFF, 31, 1));
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 31, 2));  /* Illegal length and
                                                         position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 31, 32)); /* Illegal length and
                                                         position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 31, 33)); /* Illegal length */

   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 32, 0));  /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 32, 1));  /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 32, 32)); /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 32, 33)); /* Illegal position and
                                                         illegal length */

   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 0xFF, 0xFF)); /* Illegal position
                                                             and illegal length
                                                           */

   /* 00000000 00000000 00000000 00010000 */
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 0, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 1, 3));
   EXPECT_EQ (0x00000004ul, pf_get_bits (0x00000010, 2, 3));
   EXPECT_EQ (0x00000002ul, pf_get_bits (0x00000010, 3, 3));
   EXPECT_EQ (0x00000001ul, pf_get_bits (0x00000010, 4, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 5, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 28, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 29, 3));
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 30, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 31, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 32, 3)); /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 33, 3)); /* Illegal position */

   /* 00000000 00000000 00000000 00000001 */
   EXPECT_EQ (0x00000001ul, pf_get_bits (0x00000001, 0, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000001, 1, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000001, 2, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000001, 3, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000001, 4, 3));
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 30, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 31, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 32, 3)); /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 33, 3)); /* Illegal position */

   /* 10000000 00000000 00000000 00000000 */
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x80000000, 0, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x80000000, 1, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x80000000, 28, 3));
   EXPECT_EQ (0x00000004ul, pf_get_bits (0x80000000, 29, 3));
   EXPECT_EQ (0ul, pf_get_bits (0x80000000, 30, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x80000000, 31, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x80000000, 32, 3)); /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0x80000000, 33, 3)); /* Illegal position */
}

TEST_F (BlockReaderUnitTest, GetArParamsTest)
{
   /* Use connect request from ART tester Security Class 1 - Scenario 1 */

   // clang-format off

   const uint8_t connect_req[] = {
      0xd8, 0x3a, 0xdd, 0x72, 0x50, 0xad, 0xa8, 0xbb, 0xcc, 0x00, 0x00, 0x01, 0x08, 0x00, 0x45, 0x00,
      0x02, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x80, 0x11, 0xb6, 0xc4, 0xc0, 0xa8, 0x00, 0x19, 0xc0, 0xa8,
      0x00, 0x32, 0xc0, 0x5b, 0x88, 0x94, 0x02, 0x79, 0x21, 0x0d, 0x04, 0x00, 0x28, 0x00, 0x00, 0x00,
      0x00, 0x00, 0xde, 0xa0, 0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0x00, 0x02,
      0x04, 0x93, 0xde, 0xa0, 0x00, 0x01, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
      0xdf, 0x7d, 0x7e, 0x52, 0x72, 0x09, 0xa2, 0xb3, 0x11, 0xf0, 0x8b, 0xac, 0xa8, 0xbb, 0xcc, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xff, 0xff, 0xff, 0xff, 0x02, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x6c, 0x00, 0x00,
      0x02, 0x0d, 0x00, 0x01, 0x03, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0d, 0x01, 0x01,
      0x00, 0x3b, 0x01, 0x00, 0x00, 0x01, 0x4d, 0xef, 0xbb, 0x00, 0xdc, 0xa8, 0x4c, 0x65, 0x8c, 0x51,
      0x94, 0xbf, 0xd9, 0x6b, 0x48, 0x8b, 0x00, 0x01, 0xa8, 0xbb, 0xcc, 0x00, 0x00, 0x01, 0xde, 0xa0,
      0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x01, 0x48, 0x00,
      0x00, 0x11, 0x02, 0x58, 0x88, 0x92, 0x00, 0x05, 0x70, 0x6e, 0x2d, 0x69, 0x6f, 0x01, 0x02, 0x00,
      0x68, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x88, 0x92, 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x80,
      0x95, 0x00, 0x20, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00,
      0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80,
      0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x03, 0x00, 0x01, 0x00, 0x05, 0x00,
      0x04, 0x00, 0x01, 0x00, 0x07, 0x00, 0x03, 0x00, 0x02, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00,
      0x01, 0x00, 0x11, 0x00, 0x04, 0x00, 0x01, 0x00, 0x12, 0x01, 0x02, 0x00, 0x68, 0x01, 0x00, 0x00,
      0x02, 0x00, 0x02, 0x88, 0x92, 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0xff, 0xff, 0x00, 0x20, 0x00,
      0x20, 0x00, 0x01, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x02, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00,
      0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0d, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x80,
      0x01, 0x00, 0x0f, 0x00, 0x01, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00, 0x01, 0x00, 0x11, 0x00,
      0x04, 0x00, 0x01, 0x00, 0x12, 0x01, 0x04, 0x00, 0x3c, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x80, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x04, 0x00, 0x20, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
      0x30, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x04, 0x00, 0x20, 0x01, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x01, 0x00, 0x00, 0x01, 0x31, 0x00, 0x02, 0x00, 0x02, 0x00, 0x01, 0x01, 0x01, 0x01, 0x04, 0x00,
      0x26, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x32, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x32, 0x00, 0x03, 0x00, 0x02, 0x00, 0x01, 0x01,
      0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x04, 0x00, 0x26, 0x01, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
      0x00, 0x01, 0x40, 0x00, 0x03, 0x00, 0x02, 0x00, 0x08, 0x01, 0x01, 0x00, 0x01, 0x00, 0x08, 0x01,
      0x01, 0x01, 0x03, 0x00, 0x16, 0x01, 0x00, 0x00, 0x01, 0x88, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x03, 0x80, 0x16, 0x00, 0xc8, 0xc0, 0x00, 0xa0, 0x00
   };
   // clang-format on

   uint8_t req_initiator_addr[] = {0xa8, 0xbb, 0xcc, 0x00, 0x00, 0x01};

   pf_get_info_t info = {
      .result = PF_PARSE_OK,
      .is_big_endian = true,
      .p_frame = NULL,
      .p_array = connect_req,
      .len = sizeof (connect_req)};
   pf_ar_t ar;

   uint16_t pos = 0x8E + 6; /* Offset of ARBlockReq,
                             * blockheader already consumed (+ 6)
                             */

   pf_get_ar_param (&info, &pos, &ar.ar_param);
   EXPECT_EQ (0xCD, pos);

   EXPECT_EQ (0x0001, ar.ar_param.ar_type);
   EXPECT_EQ (0x0001, ar.ar_param.session_key);
   EXPECT_EQ (
      0,
      memcmp (
         req_initiator_addr,
         ar.ar_param.cm_initiator_mac_add.addr,
         sizeof (ar.ar_param.cm_initiator_mac_add.addr)));

   EXPECT_EQ (1, ar.ar_param.ar_properties.state);
   EXPECT_FALSE (ar.ar_param.ar_properties.supervisor_takeover_allowed);
   EXPECT_TRUE (ar.ar_param.ar_properties.parameterization_server);
   EXPECT_FALSE (ar.ar_param.ar_properties.device_access);
   EXPECT_EQ (0x0000, ar.ar_param.ar_properties.companion_ar);
   EXPECT_FALSE (ar.ar_param.ar_properties.acknowledge_companion_ar);
   EXPECT_TRUE (ar.ar_param.ar_properties.reject_dcp_set_requests);
   EXPECT_FALSE (ar.ar_param.ar_properties.combined_object_container);
   EXPECT_TRUE (ar.ar_param.ar_properties.startup_mode);
   EXPECT_FALSE (ar.ar_param.ar_properties.pull_module_alarm_allowed);

   EXPECT_EQ (600, ar.ar_param.cm_initiator_activity_timeout_factor);
   EXPECT_EQ (0x8892, ar.ar_param.cm_initiator_udp_rt_port);
   EXPECT_EQ (5, ar.ar_param.cm_initiator_station_name_len);
   EXPECT_EQ ('\0', ar.ar_param.cm_initiator_station_name[5]);
   EXPECT_STREQ ("pn-io", ar.ar_param.cm_initiator_station_name);
}

TEST_F (BlockReaderUnitTest, BlockReaderTestGetCheckPeers)
{
   pf_check_peers_t check_peers;
   pf_get_info_t get_info;
   uint16_t pos;

   get_info.p_frame = NULL;

   /* Valid data */
   memset (&check_peers, 0, sizeof (check_peers));
   memset (&get_info.result, 0, sizeof (get_info.result));
   get_info.is_big_endian = true;
   get_info.p_array = (uint8_t *)&check_peers_data_valid;
   get_info.len = sizeof (check_peers_data_valid);
   pos = 0;
   pf_get_port_data_check_check_peers (&get_info, &pos, 1, &check_peers);
   EXPECT_EQ (PF_PARSE_OK, get_info.result);
   EXPECT_EQ (1, check_peers.number_of_peers);
   EXPECT_EQ (
      VALID_PEER_PORT_NAME_LENGTH,
      check_peers.peers[0].length_peer_port_name);
   EXPECT_EQ (
      0,
      memcmp (
         check_peers.peers[0].peer_port_name,
         check_peers_data_valid.peer_port_name,
         VALID_PEER_PORT_NAME_LENGTH));
   EXPECT_EQ (
      VALID_PEER_STATION_NAME_LENGTH,
      check_peers.peers[0].length_peer_station_name);
   EXPECT_EQ (
      0,
      memcmp (
         check_peers.peers[0].peer_station_name,
         check_peers_data_valid.peer_station_name,
         VALID_PEER_STATION_NAME_LENGTH));

   /* Invalid LengthPeerPortName */
   memset (
      check_peers_data_invalid_peer_port_name.peer_port_name,
      'p',
      INVALID_PEER_PORT_NAME_LENGTH);
   memset (&check_peers, 0, sizeof (check_peers));
   memset (&get_info.result, 0, sizeof (get_info.result));
   get_info.is_big_endian = true;
   get_info.p_array = (uint8_t *)&check_peers_data_invalid_peer_port_name;
   get_info.len = sizeof (check_peers_data_invalid_peer_port_name);
   pos = 0;
   pf_get_port_data_check_check_peers (&get_info, &pos, 1, &check_peers);
   EXPECT_EQ (PF_PARSE_ERROR, get_info.result);
   EXPECT_EQ (0, check_peers.number_of_peers);

   /* Invalid LengthPeerStationName */
   memset (
      check_peers_data_invalid_peer_station_name.peer_station_name,
      's',
      INVALID_PEER_STATION_NAME_LENGTH);
   memset (&check_peers, 0, sizeof (check_peers));
   memset (&get_info.result, 0, sizeof (get_info.result));
   get_info.is_big_endian = true;
   get_info.p_array = (uint8_t *)&check_peers_data_invalid_peer_station_name;
   get_info.len = sizeof (check_peers_data_invalid_peer_station_name);
   pos = 0;
   pf_get_port_data_check_check_peers (&get_info, &pos, 1, &check_peers);
   EXPECT_EQ (PF_PARSE_ERROR, get_info.result);
   EXPECT_EQ (0, check_peers.number_of_peers);
}

#if PNET_OPTION_AR_VENDOR_BLOCKS
TEST_F (BlockReaderUnitTest, BlockReaderTestGetARVendorRequest)
{
   pf_ar_vendor_request_t ar_vendor_request;
   pf_get_info_t get_info;
   uint16_t pos;

   get_info.p_frame = NULL;

   memset (&ar_vendor_request, 0, sizeof (ar_vendor_request));
   memset (&get_info.result, 0, sizeof (get_info.result));
   get_info.is_big_endian = true;
   get_info.p_array = (uint8_t *)&ar_vendor_block_req_dummy;
   get_info.len = sizeof (ar_vendor_block_req_dummy);
   pos = 0;
   ar_vendor_request.length_data_request = UINT16_MAX;
   pf_get_ar_vendor_request (
      &get_info,
      &pos,
      sizeof (ar_vendor_block_req_dummy),
      &ar_vendor_request);
   EXPECT_EQ (
      sizeof (ar_vendor_block_req_dummy.data),
      ar_vendor_request.length_data_request);
   EXPECT_EQ (
      DUMMY_AP_STRUCTURE_IDENTIFIER,
      ar_vendor_request.ap_structure_identifier);
   EXPECT_EQ (DUMMY_API, ar_vendor_request.api);
   EXPECT_EQ (
      0,
      memcmp (
         ar_vendor_block_req_dummy.data,
         ar_vendor_request.data_request,
         sizeof (ar_vendor_block_req_dummy.data)));

   memset (&ar_vendor_request, 0, sizeof (ar_vendor_request));
   memset (&get_info.result, 0, sizeof (get_info.result));
   get_info.is_big_endian = true;
   get_info.p_array = (uint8_t *)&ar_vendor_block_req_dummy;
   get_info.len = sizeof (ar_vendor_block_req_dummy);
   pos = 0;
   ar_vendor_request.length_data_request = UINT16_MAX;
   pf_get_ar_vendor_request (
      &get_info,
      &pos,
      INVALID_AR_VENDOR_BLOCK_REQ_LENGTH,
      &ar_vendor_request);
   EXPECT_EQ (0, ar_vendor_request.length_data_request);
}
#endif /* PNET_OPTION_AR_VENDOR_BLOCKS */

TEST_F (BlockReaderUnitTest, BlockReaderTestGetMemS)
{
   const uint8_t src[] = {1, 2, 3, 4, 5, 6, 7, 8};
   uint8_t dest[4];
   pf_get_info_t info;
   uint16_t pos;

   /* Fits in both source and destination */
   memset (&info, 0, sizeof (info));
   info.p_array = src;
   info.len = sizeof (src);
   info.result = PF_PARSE_OK;
   memset (dest, 0xff, sizeof (dest));
   pos = 2;
   pf_get_mem_s (&info, &pos, sizeof (dest), dest, sizeof (dest));
   EXPECT_EQ (PF_PARSE_OK, info.result);
   EXPECT_EQ (6, pos);
   EXPECT_EQ (3, dest[0]);
   EXPECT_EQ (6, dest[3]);

   /* Destination too small. Nothing is copied, and destination is zeroed */
   info.result = PF_PARSE_OK;
   memset (dest, 0xff, sizeof (dest));
   pos = 0;
   pf_get_mem_s (&info, &pos, sizeof (dest) + 1, dest, sizeof (dest));
   EXPECT_EQ (PF_PARSE_END_OF_OUTPUT, info.result);
   EXPECT_EQ (0, pos);
   EXPECT_EQ (0, dest[0]);
   EXPECT_EQ (0, dest[3]);

   /* Size is larger than the input */
   info.result = PF_PARSE_OK;
   memset (dest, 0xff, sizeof (dest));
   pos = 6;
   pf_get_mem_s (&info, &pos, sizeof (dest), dest, sizeof (dest));
   EXPECT_EQ (PF_PARSE_END_OF_INPUT, info.result);
   EXPECT_EQ (6, pos);
   EXPECT_EQ (0, dest[0]);
   EXPECT_EQ (0, dest[3]);

   /* Length wrapped around, as for a too small alarm variable part length */
   info.result = PF_PARSE_OK;
   memset (dest, 0xff, sizeof (dest));
   pos = 0;
   pf_get_mem_s (&info, &pos, (uint16_t)(2 - 40), dest, sizeof (dest));
   EXPECT_NE (PF_PARSE_OK, info.result);
   EXPECT_EQ (0, dest[0]);

   /* An earlier error is preserved */
   info.result = PF_PARSE_ERROR;
   memset (dest, 0xff, sizeof (dest));
   pos = 0;
   pf_get_mem_s (&info, &pos, 1, dest, sizeof (dest));
   EXPECT_EQ (PF_PARSE_ERROR, info.result);
   EXPECT_EQ (0, dest[0]);
}
