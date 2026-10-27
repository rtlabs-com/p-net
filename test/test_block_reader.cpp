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

TEST_F (BlockReaderUnitTest, BlockReaderTestGetCheckPeers)
{
   pf_check_peers_t check_peers;
   pf_get_info_t get_info;
   uint16_t pos;

   /* Valid data */
   memset (&check_peers, 0, sizeof (check_peers));
   memset (&get_info.result, 0, sizeof (get_info.result));
   get_info.is_big_endian = true;
   get_info.p_buf = (uint8_t *)&check_peers_data_valid;
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
   get_info.p_buf = (uint8_t *)&check_peers_data_invalid_peer_port_name;
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
   get_info.p_buf = (uint8_t *)&check_peers_data_invalid_peer_station_name;
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

   memset (&ar_vendor_request, 0, sizeof (ar_vendor_request));
   memset (&get_info.result, 0, sizeof (get_info.result));
   get_info.is_big_endian = true;
   get_info.p_buf = (uint8_t *)&ar_vendor_block_req_dummy;
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
   get_info.p_buf = (uint8_t *)&ar_vendor_block_req_dummy;
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
   info.p_buf = src;
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
