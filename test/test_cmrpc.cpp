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
 * @brief Integration testing of CMRPC.
 *
 * For example
 *  Connect
 *  Read IM0
 *  Release connection
 *  Timeout in CPM
 *  Fragmented connect
 *  ConnectReleaseIOSAR_DA ?
 */

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

extern "C" {
int pf_cmrpc_rm_read_ind_test (
   pnet_t * net,
   pf_session_info_t * si,
   uint16_t opnum,
   uint16_t req_pos,
   uint16_t res_size,
   uint8_t * res,
   uint16_t * res_pos);
}

class CmrpcUnitTest : public PnetUnitTest
{
};
class CmrpcTest : public PnetIntegrationTest
{
};

// clang-format off

/**
 * Connect request data
 * This is UDP payload, so the first 42 bytes (in dec) have been removed from
 * the Wireshark output.
 */
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
 0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x05, 0x00, 0x00, 0x20, 0x00,
 0x00, 0x00, 0xdc, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x81, 0x12,
 0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00
};

static uint8_t write_req[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x44, 0x00,
 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x08,
 0x00, 0x3c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
 0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xad, 0xa0,
 0xbe, 0xda
};

static uint8_t read_im0_req[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xde, 0xa0, 0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0xde, 0xa0, 0x00, 0x01, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02,
 0xff, 0xff, 0xff, 0xff, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x40, 0x00, 0x00,
 0x00, 0x40, 0x00, 0x00, 0x10, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x09,
 0x00, 0x3c, 0x01, 0x00, 0x00, 0x02, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
 0xaf, 0xf0, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*
 * In order to create a fragmented connect using the connect request above:
 * 1) Copy the connect request to both fragments.
 * 2) Strip away the ETH header, the IP header and the UDP header (0x2a bytes) from both packets (already done!!!)
 * 3) In fragment 1 change byte [0x2c] from 0x28 to 0x2c.   (means fragment)
 * 4) In fragment 2 change byte [0x2c] from 0x28 to 0x2e.   (means last fragment)
 * 5) Modify fragment length member of both frames:
 *    a) Fragment 1: bytes [0x74..0x75] from {0x86 0x01} to [0x00 0x01] (means 0x100 bytes).
 *    b) Fragment 2: bytes [0x74..0x75] from {0x86 0x01} to [0x86 0x00] (means 0x86  bytes).
 * 6) Modify fragment number of fragment 2:
 *    Fragment 2: bytes [0x76..0x77] from {0x00 0x00} to {0x01 0x00} (means fragment 1).
 * 7) Modify serial_low member of fragment 2:
 *    Fragment 2: byte [0x79] from 0x00 to 0x01.
 * 7) Cut fragment 1 at 256 bytes of connect request (i.e., at position 0x16a).
 * 8) Remove the first 256 bytes of the connect request from fragment 2 (i.e. bytes 0x7a to 0x17a)
 */
static uint8_t connect_frag_1_req[] =
{
 /*  20 */                                                    /* 2a */0x04, 0x00, 0x2c, 0x00, 0x10, 0x00,
 /*  30 */0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 /*  40 */0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 /*  50 */0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 /*  60 */0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*  70 */0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0x00, 0x00, 0x72, 0x01,
 /*  80 */0x00, 0x00, 0x24, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72, 0x01, 0x00, 0x00, 0x01, 0x01,
 /*  90 */0x00, 0x42, 0x01, 0x00, 0x00, 0x01, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 /*  a0 */0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, 0xde, 0xa0,
 /*  b0 */0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x01, 0x40, 0x00,
 /*  c0 */0x00, 0x11, 0x02, 0x58, 0x88, 0x92, 0x00, 0x0c, 0x72, 0x74, 0x2d, 0x6c, 0x61, 0x62, 0x73, 0x2d,
 /*  d0 */0x64, 0x65, 0x6d, 0x6f, 0x01, 0x02, 0x00, 0x50, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x88, 0x92,
 /*  e0 */0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x80, 0x01, 0x00, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
 /*  f0 */0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /* 100 */0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
 /* 110 */0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03,
 /* 120 */0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x05, 0x01, 0x02, 0x00, 0x50, 0x01, 0x00, 0x00, 0x02,
 /* 130 */0x00, 0x02, 0x88, 0x92, 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0x80, 0x00, 0x00, 0x20, 0x00, 0x01,
 /* 140 */0x00, 0x01, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00,
 /* 150 */0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
 /* 160 */0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01,
 /* 170 */0x00, 0x00, 0x80, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01
};

static uint8_t connect_frag_2_req[] =
{
 /*  20 */                                                    /* 2a */0x04, 0x00, 0x2e, 0x00, 0x10, 0x00,
 /*  30 */0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 /*  40 */0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 /*  50 */0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 /*  60 */0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 /*  70 */0xff, 0xff, 0xff, 0xff, 0x86, 0x00, 0x01, 0x00, 0x00, 0x01,
 /*  80 */
 /*  90 */
 /*  a0 */
 /*  b0 */
 /*  c0 */
 /*  d0 */
 /*  e0 */
 /*  f0 */
 /* 100 */
 /* 110 */
 /* 120 */
 /* 130 */
 /* 140 */
 /* 150 */
 /* 160 */
 /* 170 */                                                            0x00, 0x03, 0x01, 0x04, 0x00, 0x3c,
 /* 180 */0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
 /* 190 */0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01,
 /* 1a0 */0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x80, 0x01,
 /* 1b0 */0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x04, 0x00, 0x26,
 /* 1c0 */0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
 /* 1d0 */0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01,
 /* 1e0 */0x00, 0x02, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x16, 0x01, 0x00, 0x00, 0x01, 0x88, 0x92,
 /* 1f0 */0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x02, 0x00, 0xc8, 0xc0, 0x00, 0xa0, 0x00
};

static uint8_t connect_req_iosar_da[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x56, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x10, 0x00, 0x00, 0x42, 0x00,
 0x00, 0x00, 0x24, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x01, 0x01,
 0x00, 0x3e, 0x01, 0x00, 0x00, 0x06, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, 0xde, 0xa0,
 0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xf0, 0x00, 0x00, 0x01, 0x40, 0x00,
 0x01, 0x11, 0x02, 0x58, 0x88, 0x92, 0x00, 0x08, 0x72, 0x74, 0x2d, 0x6c, 0x61, 0x62, 0x73, 0x31
};

static uint8_t release_req_iosar_da[] =
{
                                                             0x04, 0x00, 0x28, 0x00, 0x10, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
 0xfe, 0xed, 0x01, 0x00, 0xa0, 0xde, 0x97, 0x6c, 0xd1, 0x11, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
 0xdf, 0x7d, 0xbb, 0xac, 0x97, 0xe2, 0x76, 0x54, 0x9f, 0x47, 0xa5, 0xbd, 0xa5, 0xe3, 0x7d, 0x98,
 0xe5, 0xda, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
 0xff, 0xff, 0xff, 0xff, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x20, 0x00,
 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x14,
 0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0x30, 0xab, 0xa9, 0xa3, 0xf7, 0x64, 0xb7, 0x44, 0xb3, 0xb6,
 0x7e, 0xe2, 0x8a, 0x1a, 0x02, 0xcb, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00
};

static uint8_t connect_req_siemens[] =
{
                                                            0x04, 0x00, 0x20, 0x00, 0x00, 0x00,
0x00, 0x00, 0xde, 0xa0, 0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
0xfe, 0xed, 0xde, 0xa0, 0x00, 0x01, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
0xdf, 0x7d, 0x0a, 0x07, 0x10, 0x32, 0x1b, 0xd0, 0x1f, 0x1f, 0x8a, 0x8c, 0xe0, 0xdc, 0xa0, 0x8a,
0xba, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff, 0xff, 0xff, 0xff, 0x01, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00,
0x01, 0x70, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x70, 0x01, 0x01,
0x00, 0x40, 0x01, 0x00, 0x00, 0x01, 0xf1, 0x07, 0x05, 0x4c, 0x1e, 0x41, 0x43, 0x4f, 0x8b, 0xc8,
0xcc, 0x5f, 0x1c, 0x76, 0x93, 0xf8, 0x00, 0x01, 0xe0, 0xdc, 0xa0, 0x8a, 0xba, 0x33, 0xde, 0xa0,
0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x64, 0x01, 0x0d, 0x00, 0x2a, 0x40, 0x00,
0x00, 0x11, 0x00, 0xc8, 0x88, 0x92, 0x00, 0x0a, 0x70, 0x6c, 0x63, 0x78, 0x62, 0x31, 0x64, 0x30,
0x65, 0x64, 0x01, 0x02, 0x00, 0x50, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x88, 0x92, 0x00, 0x00,
0x00, 0x02, 0x00, 0x28, 0x80, 0x00, 0x00, 0x20, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0xff, 0xff,
0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
0x00, 0x01, 0x00, 0x00, 0x80, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01,
0x00, 0x01, 0x00, 0x01, 0x00, 0x05, 0x01, 0x02, 0x00, 0x50, 0x01, 0x00, 0x00, 0x02, 0x00, 0x02,
0x88, 0x92, 0x00, 0x00, 0x00, 0x02, 0x00, 0x28, 0xff, 0xff, 0x00, 0x20, 0x00, 0x02, 0x00, 0x01,
0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x03, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x04,
0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x00,
0x80, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03, 0x01, 0x04, 0x00, 0x3c, 0x01, 0x00,
0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03,
0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x80, 0x00,
0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x80, 0x01, 0x00, 0x00,
0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x04, 0x00, 0x26, 0x01, 0x00,
0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x01,
0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x02,
0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x16, 0x01, 0x00, 0x00, 0x01, 0x88, 0x92, 0x00, 0x00,
0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x01, 0x00, 0xc0, 0x00, 0xa0, 0x00
};

static uint8_t write_req_siemens[] =
{
                                                            0x04, 0x00, 0x20, 0x00, 0x00, 0x00,
0x00, 0x00, 0xde, 0xa0, 0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
0xfe, 0xed, 0xde, 0xa0, 0x00, 0x01, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
0xdf, 0x7d, 0x0a, 0x07, 0x10, 0x32, 0x1b, 0xd0, 0x1f, 0x1f, 0x8a, 0x8c, 0xe0, 0xdc, 0xa0, 0x8a,
0xba, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
0xff, 0xff, 0xff, 0xff, 0x01, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00,
0x01, 0x14, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x14, 0x00, 0x08,
0x00, 0x3c, 0x01, 0x00, 0x00, 0x00, 0xf1, 0x07, 0x05, 0x4c, 0x1e, 0x41, 0x43, 0x4f, 0x8b, 0xc8,
0xcc, 0x5f, 0x1c, 0x76, 0x93, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
0xe0, 0x40, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
0x00, 0x3c, 0x01, 0x00, 0x00, 0x01, 0xf1, 0x07, 0x05, 0x4c, 0x1e, 0x41, 0x43, 0x4f, 0x8b, 0xc8,
0xcc, 0x5f, 0x1c, 0x76, 0x93, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
0x80, 0x71, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x50,
0x00, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x3c, 0x01, 0x00,
0x00, 0x02, 0xf1, 0x07, 0x05, 0x4c, 0x1e, 0x41, 0x43, 0x4f, 0x8b, 0xc8, 0xcc, 0x5f, 0x1c, 0x76,
0x93, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00,
0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
0x00, 0x3c, 0x01, 0x00, 0x00, 0x03, 0xf1, 0x07, 0x05, 0x4c, 0x1e, 0x41, 0x43, 0x4f, 0x8b, 0xc8,
0xcc, 0x5f, 0x1c, 0x76, 0x93, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
0x00, 0x7c, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00
};

static uint8_t prm_end_req_siemens[] =
{
                                                            0x04, 0x00, 0x20, 0x00, 0x00, 0x00,
0x00, 0x00, 0xde, 0xa0, 0x00, 0x00, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0x01, 0xbe, 0xef,
0xfe, 0xed, 0xde, 0xa0, 0x00, 0x01, 0x6c, 0x97, 0x11, 0xd1, 0x82, 0x71, 0x00, 0xa0, 0x24, 0x42,
0xdf, 0x7d, 0x0a, 0x07, 0x10, 0x32, 0x1b, 0xd0, 0x1f, 0x1f, 0x8a, 0x8c, 0xe0, 0xdc, 0xa0, 0x8a,
0xba, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04,
0xff, 0xff, 0xff, 0xff, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x01, 0x10,
0x00, 0x1c, 0x01, 0x00, 0x00, 0x00, 0xf1, 0x07, 0x05, 0x4c, 0x1e, 0x41, 0x43, 0x4f, 0x8b, 0xc8,
0xcc, 0x5f, 0x1c, 0x76, 0x93, 0xf8, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
};

/* This is a raw Ethernet packet (not UDP payload) */
static uint8_t data_packet[] =
{
 0x1e, 0x30, 0x6c, 0xa2, 0x45, 0x5e, 0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, 0x88, 0x92, 0x80, 0x00,
 0x80, 0x80, 0x80, 0x20, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf3, 0x35, 0x00
};

// clang-format on

/**
 * High priority RTA alarm notification frame (raw Ethernet), sent to the
 * first AR. The source MAC is the initiator MAC of connect_req, and the source
 * alarm reference is the local alarm reference of its AlarmCRBlockReq.
 */
// clang-format off
static uint8_t alarm_notification_frame[] =
{
   // Ethernet header
   0x02, 0x67, 0x00, 0x00, 0x00, 0x01, // [00-05] Destination MAC address
   0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, // [06-11] Source MAC address
   0x88, 0x92,                         // [12-13] EtherType: PROFINET
   // RTA header
   0xfc, 0x01,                         // [14-15] Frame ID: Alarm, high priority
   0x00, 0x01,                         // [16-17] Destination alarm reference: 1 (IO-device)
   0x00, 0x02,                         // [18-19] Source alarm reference: 2 (IO-controller)
   0x11,                               // [20]    PDU type: DATA, version 1
   0x10,                               // [21]    Add flags: Window size 0, TACK
   0xff, 0xff,                         // [22-23] Send sequence number
   0x00, 0x00,                         // [24-25] Ack sequence number
   0x00, 0x20,                         // [26-27] Variable part length: 32 bytes
   // Alarm notification block
   0x00, 0x01,                         // [28-29] Block type: Alarm notification high
   0x00, 0x1c,                         // [30-31] Block length: 28
   0x01, 0x00,                         // [32-33] Block version: 1.0
   0x00, 0x01,                         // [34-35] Alarm type: Diagnosis
   0x00, 0x00, 0x00, 0x00,             // [36-39] API
   0x00, 0x00,                         // [40-41] Slot number
   0x00, 0x00,                         // [42-43] Subslot number
   0x00, 0x00, 0x00, 0x00,             // [44-47] Module ident number
   0x00, 0x00, 0x00, 0x00,             // [48-51] Submodule ident number
   0x00, 0x00,                         // [52-53] Alarm specifier
   0x00, 0x01,                         // [54-55] User structure identifier: 1
   0x44, 0x45, 0x4d, 0x4f              // [56-59] Alarm data: "DEMO"
};
// clang-format on

/**
 * High priority RTA alarm ERROR frame (raw Ethernet), sent to the first AR.
 * The source MAC is the initiator MAC of connect_req, and the source alarm
 * reference is the local alarm reference of its AlarmCRBlockReq.
 */
// clang-format off
static uint8_t alarm_error_frame[] =
{
   // Ethernet header
   0x02, 0x67, 0x00, 0x00, 0x00, 0x01, // [00-05] Destination MAC address
   0xc8, 0x5b, 0x76, 0xe6, 0x89, 0xdf, // [06-11] Source MAC address
   0x88, 0x92,                         // [12-13] EtherType: PROFINET
   // RTA header
   0xfc, 0x01,                         // [14-15] Frame ID: Alarm, high priority
   0x00, 0x01,                         // [16-17] Destination alarm reference: 1 (IO-device)
   0x00, 0x02,                         // [18-19] Source alarm reference: 2 (IO-controller)
   0x14,                               // [20]    PDU type: ERROR, version 1
   0x10,                               // [21]    Add flags: Window size 0, TACK
   0xff, 0xff,                         // [22-23] Send sequence number
   0x00, 0x00,                         // [24-25] Ack sequence number
   0x00, 0x04,                         // [26-27] Variable part length: 4 bytes
   // PNIO status
   0xcf,                               // [28]    Error code
   0x81,                               // [29]    Error decode
   0x00,                               // [30]    Error code 1
   0x01,                               // [31]    Error code 2
   // Padding to minimum Ethernet frame size
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // [32-39]
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // [40-47]
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // [48-55]
   0x00, 0x00, 0x00, 0x00                          // [56-59]
};
// clang-format on

/* Offsets of fields in the frames */
#define ALARM_FRAME_SRC_MAC_POS  6
#define ALARM_FRAME_SRC_REF_POS  18

/**
 * Hand an alarm frame to the stack, as if received on the Ethernet port.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame            In:    The frame
 * @param len              In:    Length of the frame
 */
static void receive_alarm_frame (
   pnet_t * net,
   const uint8_t * frame,
   uint16_t len)
{
   pnal_buf_t * p_buf = pnal_buf_alloc (len);

   ASSERT_NE (p_buf, nullptr);
   memcpy (p_buf->payload, frame, len);
   p_buf->len = len;

   /* The buffer is owned by the alarm queue if the frame was handled */
   if (pf_eth_recv (NULL, net, p_buf) == 0)
   {
      pnal_buf_free (p_buf);
   }
}

static struct
{
   uint8_t block_type[2];
   uint8_t block_len[2];
   uint8_t block_version_high[1];
   uint8_t block_version_low[1];
   uint8_t sequence_number[2];
   uint8_t aruuid[16];
   uint8_t api[4];
   uint8_t slot_number[2];
   uint8_t subslot_number[2];
   uint8_t padding_1[2];
   uint8_t index[2];
   uint8_t record_data_length[4];
   uint8_t target_aruuid[16];
   uint8_t padding_2[8];
} iod_read_req_implicit_invalid = {
   .block_type = {0x00, 0x09},
   .block_len = {0, 60},
   .block_version_high = {1},
   .block_version_low = {0},
   .sequence_number = {0},
   .aruuid = {0},
   .api = {0},
   .slot_number = {0},
   .subslot_number = {0, 1},
   .padding_1 = {0},
   .index = {0x80, 0x00},
   .record_data_length = {0x4, 0x0},
   .target_aruuid = {0},
   .padding_2 = {0}};

TEST_F (CmrpcTest, CmrpcUnknownInterfaceDoesNotLeakSessions)
{
   uint8_t req[sizeof (connect_req)];
   uint32_t ix;

   /* Connect request with unknown interface UUID and unique activity UUID */
   memcpy (req, connect_req, sizeof (req));
   req[24] ^= 0xff; /* Interface UUID */

   /* More requests than there are sessions */
   for (ix = 0; ix < (2 * PF_MAX_SESSION); ix++)
   {
      req[40] = (uint8_t)(ix + 1); /* Activity UUID */
      mock_set_pnal_udp_recvfrom_buffer (req, sizeof (req));
      run_stack (TEST_UDP_DELAY);
   }
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2 * PF_MAX_SESSION);

   /* A valid connect request must still be accepted */
   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
}

TEST_F (CmrpcTest, CmrpcConnectReleaseTest)
{
   int ret;
   uint32_t ix;
   EXPECT_EQ (mock_os_data.udp_sendto_len, 0);

   TEST_TRACE ("\nGenerating mock connection request\n");
   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_GT (mock_os_data.eth_send_count, 0);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 178);

   TEST_TRACE ("\nGenerating mock write request\n");
   mock_set_pnal_udp_recvfrom_buffer (write_req, sizeof (write_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.write_calls, 1);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 164);

   TEST_TRACE ("\nGenerating mock parameter end request\n");
   mock_set_pnal_udp_recvfrom_buffer (prm_end_req, sizeof (prm_end_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 2);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_PRMEND);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (appdata.call_counters.write_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 3);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 132);

   TEST_TRACE ("\nSimulate application calling APPL_RDY\n");
   ret = pnet_application_ready (net, appdata.main_arep);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (appdata.call_counters.state_calls, 3);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 4);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 132);

   TEST_TRACE ("\nGenerating mock application ready response\n");
   mock_set_pnal_udp_recvfrom_buffer (appl_rdy_rsp, sizeof (appl_rdy_rsp));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 3);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 4);

   TEST_TRACE ("\nGenerating I&M0 request\n");
   mock_set_pnal_udp_recvfrom_buffer (read_im0_req, sizeof (read_im0_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 3);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 5);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 224);

   TEST_TRACE ("\nGenerating cyclic data\n");
   for (ix = 0; ix < 100; ix++)
   {
      send_data (data_packet, sizeof (data_packet));
      run_stack (TEST_DATA_DELAY);
   }
   EXPECT_EQ (appdata.call_counters.state_calls, 4);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_DATA);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 5);

   TEST_TRACE ("\nGenerating mock release request\n");
   mock_set_pnal_udp_recvfrom_buffer (release_req, sizeof (release_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.release_calls, 1);
   EXPECT_EQ (appdata.call_counters.state_calls, 5);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 6);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 132);
}

TEST_F (CmrpcTest, CmrpcConnectionTimeoutTest)
{
   int ret;
   bool new_flag = false;
   uint8_t in_data[10];
   uint16_t in_len = sizeof (in_data);
   uint8_t out_data[] = {
      0x33, /* Slot 1, subslot 1 Data */
   };
   uint8_t iops = PNET_IOXS_BAD;
   uint32_t ix;

   TEST_TRACE ("\nGenerating mock connection request\n");
   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_GT (mock_os_data.eth_send_count, 0);

   TEST_TRACE ("\nGenerating mock write request\n");
   mock_set_pnal_udp_recvfrom_buffer (write_req, sizeof (write_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);

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

   TEST_TRACE ("\nGenerate a couple of data packets to move pf_cpm to RUN "
               "state\n");
   for (ix = 0; ix < 100; ix++)
   {
      send_data (data_packet, sizeof (data_packet));
      run_stack (TEST_DATA_DELAY);
   }
   EXPECT_EQ (appdata.call_counters.state_calls, 4);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_DATA);

   TEST_TRACE ("\nRead data just to see that it works\n");
   in_len = sizeof (in_data);
   ret = pnet_output_get_data_and_iops (
      net,
      TEST_API_IDENT,
      1,
      1,
      &new_flag,
      in_data,
      &in_len,
      &iops);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (new_flag, true);
   EXPECT_EQ (in_len, 1);
   EXPECT_EQ (iops, PNET_IOXS_GOOD);

   TEST_TRACE ("\nGenerate new data to move PPM to RUN state\n");
   ret = pnet_input_set_data_and_iops (
      net,
      TEST_API_IDENT,
      1,
      1,
      out_data,
      sizeof (out_data),
      PNET_IOXS_GOOD);
   EXPECT_EQ (ret, 0);

   TEST_TRACE ("\nWait for timeout in CPM\n");
   run_stack (TEST_TIMEOUT_DELAY);
   EXPECT_EQ (appdata.call_counters.release_calls, 0);
   EXPECT_EQ (appdata.call_counters.state_calls, 5);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);

   TEST_TRACE ("\nGenerating mock release request\n");
   mock_set_pnal_udp_recvfrom_buffer (release_req, sizeof (release_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.release_calls, 0);
   EXPECT_EQ (appdata.call_counters.state_calls, 5);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);
}

/**
 *  Replicate startup with a Siemens S7-1200 controller
 */
TEST_F (CmrpcTest, CmrpcSiemensConnectTest)
{
   EXPECT_EQ (mock_os_data.udp_sendto_len, 0);

   TEST_TRACE ("\nGenerating mock connection request\n");
   mock_set_pnal_udp_recvfrom_buffer (
      connect_req_siemens,
      sizeof (connect_req_siemens));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (appdata.call_counters.write_calls, 0);
   EXPECT_EQ (appdata.call_counters.read_calls, 0);
   EXPECT_EQ (appdata.call_counters.ccontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.dcontrol_calls, 0);
   EXPECT_GT (mock_os_data.eth_send_count, 0);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 178); // Actual value. OK in
                                                 // Wireshark (232 bytes)

   TEST_TRACE ("\nGenerating mock write request\n");
   mock_set_pnal_udp_recvfrom_buffer (
      write_req_siemens,
      sizeof (write_req_siemens));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (appdata.call_counters.write_calls, 2);
   EXPECT_EQ (appdata.call_counters.read_calls, 0);
   EXPECT_EQ (appdata.call_counters.ccontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.dcontrol_calls, 0);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 356); // Actual value. OK in
                                                 // Wireshark (398 bytes)

   TEST_TRACE ("\nGenerating mock parameter end request\n");
   mock_set_pnal_udp_recvfrom_buffer (
      prm_end_req_siemens,
      sizeof (prm_end_req_siemens));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_PRMEND);
   EXPECT_EQ (appdata.call_counters.state_calls, 2);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (appdata.call_counters.write_calls, 2);
   EXPECT_EQ (appdata.call_counters.read_calls, 0);
   EXPECT_EQ (appdata.call_counters.ccontrol_calls, 0);
   EXPECT_EQ (appdata.call_counters.dcontrol_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 3);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 132); // 20 bytes longer than sent
                                                 // from faulty application
}

TEST_F (CmrpcTest, CmrpcConnectFragmentTest)
{
   int ret;
   uint32_t ix;

   TEST_TRACE ("\nGenerating mock connection request, fragment 1\n");
   mock_set_pnal_udp_recvfrom_buffer (
      connect_frag_1_req,
      sizeof (connect_frag_1_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 0);
   EXPECT_EQ (appdata.call_counters.connect_calls, 0);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);

   TEST_TRACE ("\nGenerating mock connection request, fragment 2\n");
   mock_set_pnal_udp_recvfrom_buffer (
      connect_frag_2_req,
      sizeof (connect_frag_2_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_GT (mock_os_data.eth_send_count, 0);

   TEST_TRACE ("\nGenerating mock connection write request\n");
   mock_set_pnal_udp_recvfrom_buffer (write_req, sizeof (write_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_EQ (appdata.call_counters.write_calls, 1);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);

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

   TEST_TRACE ("\nRead I&M0\n");
   mock_set_pnal_udp_recvfrom_buffer (read_im0_req, sizeof (read_im0_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 3);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   TEST_TRACE ("\nSending data\n");
   for (ix = 0; ix < 100; ix++)
   {
      send_data (data_packet, sizeof (data_packet));
      run_stack (TEST_DATA_DELAY);
   }
   EXPECT_EQ (appdata.call_counters.state_calls, 4);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_DATA);

   TEST_TRACE ("Sending mock release request\n");
   mock_set_pnal_udp_recvfrom_buffer (release_req, sizeof (release_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.release_calls, 1);
   EXPECT_EQ (appdata.call_counters.state_calls, 5);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);
}

TEST_F (CmrpcTest, CmrpcConnectReleaseIOSAR_DA)
{
   // Device-access AR is not yet supported
   TEST_TRACE ("\nGenerating mock connection request IOSAR_DA\n");
   mock_set_pnal_udp_recvfrom_buffer (
      connect_req_iosar_da,
      sizeof (connect_req_iosar_da));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 0);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);
   EXPECT_EQ (appdata.call_counters.connect_calls, 0);
   EXPECT_EQ (mock_os_data.eth_send_count, 0);

   TEST_TRACE ("\nGenerating mock release request IOSAR_DA\n");
   mock_set_pnal_udp_recvfrom_buffer (
      release_req_iosar_da,
      sizeof (release_req_iosar_da));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.state_calls, 0);
   EXPECT_EQ (appdata.call_counters.release_calls, 0);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);
}

/**
 * Set the station name with DCP, so that the device has both a name and an IP
 * address, and waits for a connection.
 */
static void set_station_name (pnet_t * net)
{
   const char name[] = "rt-labs-dev";
   uint8_t block_error = 0;

   ASSERT_EQ (
      0,
      pf_cmina_dcp_set_ind (
         net,
         PF_DCP_OPT_DEVICE_PROPERTIES,
         PF_DCP_SUB_DEV_PROP_NAME,
         1, /* Permanent */
         strlen (name),
         (const uint8_t *)name,
         &block_error));
   ASSERT_EQ (PF_CMINA_STATE_W_CONNECT, net->cmina_state);
}

/**
 * A DCP Set of the IP address is refused while there is an active AR. It must
 * not change the IP suites, and must not be saved. See "Case 26" in the CMINA
 * state table of PN-AL-protocol, as referred to in pf_cmina_dcp_set_ind().
 */
TEST_F (CmrpcTest, CmrpcDcpSetIpRefusedWithActiveAr)
{
   pf_ip_suite_t ip_suite;
   pf_full_ip_suite_t current_ip_suite;
   pf_full_ip_suite_t nonvolatile_ip_suite;
   uint8_t block_error = 0;

   set_station_name (net);
   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);

   memset (&ip_suite, 0, sizeof (ip_suite));
   ip_suite.ip_addr = PNAL_MAKEU32 (192, 168, 1, 199);
   ip_suite.ip_mask = PNAL_MAKEU32 (255, 255, 255, 0);
   ip_suite.ip_gateway = PNAL_MAKEU32 (192, 168, 1, 1);
   current_ip_suite = net->cmina_current_dcp_ase.full_ip_suite;
   nonvolatile_ip_suite = net->cmina_nonvolatile_dcp_ase.full_ip_suite;
   mock_bg_worker_data.save_ase_nvm_data_count = 0;

   /* The request is refused every time it is received */
   for (int ix = 0; ix < 2; ix++)
   {
      EXPECT_NE (
         0,
         pf_cmina_dcp_set_ind (
            net,
            PF_DCP_OPT_IP,
            PF_DCP_SUB_IP_PAR,
            1, /* Permanent */
            sizeof (ip_suite),
            (const uint8_t *)&ip_suite,
            &block_error));
      EXPECT_EQ (
         0,
         memcmp (
            &current_ip_suite,
            &net->cmina_current_dcp_ase.full_ip_suite,
            sizeof (current_ip_suite)));
      EXPECT_EQ (
         0,
         memcmp (
            &nonvolatile_ip_suite,
            &net->cmina_nonvolatile_dcp_ase.full_ip_suite,
            sizeof (nonvolatile_ip_suite)));
      EXPECT_EQ (mock_bg_worker_data.save_ase_nvm_data_count, 0);
   }
}

/**
 * A DCP Set of the IP address is accepted when there is no active AR. The IP
 * suites are changed and saved.
 */
TEST_F (CmrpcTest, CmrpcDcpSetIpAcceptedWithoutAr)
{
   pf_ip_suite_t ip_suite;
   uint8_t block_error = 0;

   set_station_name (net);

   memset (&ip_suite, 0, sizeof (ip_suite));
   ip_suite.ip_addr = PNAL_MAKEU32 (192, 168, 1, 199);
   ip_suite.ip_mask = PNAL_MAKEU32 (255, 255, 255, 0);
   ip_suite.ip_gateway = PNAL_MAKEU32 (192, 168, 1, 1);
   mock_bg_worker_data.save_ase_nvm_data_count = 0;

   EXPECT_EQ (
      0,
      pf_cmina_dcp_set_ind (
         net,
         PF_DCP_OPT_IP,
         PF_DCP_SUB_IP_PAR,
         1, /* Permanent */
         sizeof (ip_suite),
         (const uint8_t *)&ip_suite,
         &block_error));
   EXPECT_EQ (
      0,
      memcmp (
         &ip_suite,
         &net->cmina_current_dcp_ase.full_ip_suite.ip_suite,
         sizeof (ip_suite)));
   EXPECT_EQ (
      0,
      memcmp (
         &ip_suite,
         &net->cmina_nonvolatile_dcp_ase.full_ip_suite.ip_suite,
         sizeof (ip_suite)));
   EXPECT_EQ (mock_bg_worker_data.save_ase_nvm_data_count, 1);
}

/**
 * An alarm channel is addressed by source node, destination node, source SAP
 * and destination SAP, see PN-AL-protocol (IEC 61158-6-10) 4.8.4.1. The nodes
 * are identified by the interface MAC address, which is also the
 * CMInitiatorMacAdd of the connect request (4.2.2.2 table 16). The SAPs are the
 * alarm references exchanged in the connect request and response (4.8.2.1.1).
 */
TEST_F (CmrpcTest, CmrpcAlarmFromOtherSourceIsIgnored)
{
   uint8_t error_frame[sizeof (alarm_error_frame)];
   uint8_t frame[sizeof (alarm_notification_frame)];

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);

   /* An alarm ERROR frame from another MAC address must not abort the AR */
   memcpy (error_frame, alarm_error_frame, sizeof (error_frame));
   error_frame[ALARM_FRAME_SRC_MAC_POS] ^= 0xff;
   receive_alarm_frame (net, error_frame, sizeof (error_frame));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);

   /* Neither must one from another source alarm reference */
   memcpy (error_frame, alarm_error_frame, sizeof (error_frame));
   error_frame[ALARM_FRAME_SRC_REF_POS + 1] ^= 0xff;
   receive_alarm_frame (net, error_frame, sizeof (error_frame));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);

   /* An alarm notification from another MAC address must not reach the
      application */
   memcpy (frame, alarm_notification_frame, sizeof (frame));
   frame[ALARM_FRAME_SRC_MAC_POS] ^= 0xff;
   receive_alarm_frame (net, frame, sizeof (frame));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.alarm_calls, 0);

   /* Neither must one from another source alarm reference */
   memcpy (frame, alarm_notification_frame, sizeof (frame));
   frame[ALARM_FRAME_SRC_REF_POS + 1] ^= 0xff;
   receive_alarm_frame (net, frame, sizeof (frame));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.alarm_calls, 0);

   /* The same alarm notification from the IO-controller is accepted */
   receive_alarm_frame (
      net,
      alarm_notification_frame,
      sizeof (alarm_notification_frame));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.alarm_calls, 1);
}

/**
 * Create a write request to PDPortDataCheck (not a user specific index) from
 * write_req, with the record data replaced by the given number of bytes.
 *
 * The fields that are changed, compared to write_req:
 *
 *   [74]       RPC body length: NDR header (20), IODWriteReq header (64) and
 *              the record data
 *   [84]       NDR args length: IODWriteReq header (64) and the record data
 *   [96]       NDR actual count, same as the args length
 *   [134-135]  Index: 0x802b, PDPortDataCheck
 *   [136-139]  Record data length, which is not necessarily the number of
 *              record data bytes in the request
 *
 * @param req              Out:   Buffer for the request. Must be able to hold
 *                                the request, which is 164 bytes plus the
 *                                record data.
 * @param record_data_length In:  The value of the record data length field
 * @param data_len         In:    Number of record data bytes in the request
 * @return the length of the request
 */
static uint16_t create_write_req (
   uint8_t * req,
   uint32_t record_data_length,
   uint16_t data_len)
{
   memset (req, 0, 164 + data_len);
   memcpy (req, write_req, 164);
   req[74] = 84 + data_len;
   req[84] = 64 + data_len;
   req[96] = 64 + data_len;
   req[134] = 0x80;
   req[135] = 0x2b;
   req[136] = (uint8_t)(record_data_length >> 24);
   req[137] = (uint8_t)(record_data_length >> 16);
   req[138] = (uint8_t)(record_data_length >> 8);
   req[139] = (uint8_t)record_data_length;

   return 164 + data_len;
}

/**
 * The record data length is consistent with the ArgsLength, but the record
 * data is too short to contain the block header of the record. This is
 * reported as an access error with the error code "write length error".
 * See PN-AL-protocol (IEC 61158-6-10) 5.2.6.4.2, table 714.
 */
TEST_F (CmrpcTest, CmrpcIodWriteRecordShorterThanBlockHeader)
{
   uint8_t req[164 + sizeof (pf_block_header_t)];
   const uint32_t lengths[] = {0, sizeof (pf_block_header_t) - 1};
   uint16_t len;
   uint32_t ix;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   for (ix = 0; ix < sizeof (lengths) / sizeof (lengths[0]); ix++)
   {
      len = create_write_req (req, lengths[ix], lengths[ix]);
      mock_set_pnal_udp_recvfrom_buffer (req, len);
      run_stack (TEST_UDP_DELAY);
      EXPECT_EQ (mock_os_data.udp_sendto_count, 2 + ix);

      /* PNIO status follows the 80 byte RPC header in the response */
      EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], PNET_ERROR_CODE_WRITE);
      EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], PNET_ERROR_DECODE_PNIORW);
      EXPECT_EQ (
         mock_os_data.udp_sendto_buffer[82],
         PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR);
      EXPECT_EQ (mock_os_data.udp_sendto_buffer[83], 0);
   }
}

/**
 * The record data length is larger than the record data in the request, so it
 * is not consistent with the ArgsLength, which shall be the length of the
 * IODWriteReq header plus the record data length. This is reported as
 * "CMRPC: ArgsLength invalid". See PN-AL-protocol (IEC 61158-6-10) 5.8.10.1,
 * table 1367, and 5.2.6.4.3.2, table 721.
 */
TEST_F (CmrpcTest, CmrpcIodWriteRecordLongerThanRequest)
{
   uint8_t req[164 + sizeof (pf_block_header_t)];
   /* The request has 6 bytes of record data. The second length would wrap
      around if it was added to the position. */
   const uint32_t lengths[] = {6 + 1, 0xffffffff};
   uint16_t len;
   uint32_t ix;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   for (ix = 0; ix < sizeof (lengths) / sizeof (lengths[0]); ix++)
   {
      len = create_write_req (req, lengths[ix], sizeof (pf_block_header_t));
      mock_set_pnal_udp_recvfrom_buffer (req, len);
      run_stack (TEST_UDP_DELAY);
      EXPECT_EQ (mock_os_data.udp_sendto_count, 2 + ix);

      EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], PNET_ERROR_CODE_WRITE);
      EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], PNET_ERROR_DECODE_PNIO);
      EXPECT_EQ (
         mock_os_data.udp_sendto_buffer[82],
         PNET_ERROR_CODE_1_CMRPC);
      EXPECT_EQ (
         mock_os_data.udp_sendto_buffer[83],
         PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
   }
}

/**
 * Create a multiple write request with one write to PDPortDataCheck (not a user
 * specific index) from write_req, with 4 bytes of record data.
 *
 * The request is:
 *
 *   [000-079]  RPC header
 *   [080-099]  NDR header
 *   [100-163]  IODWriteReq header of the multiple write, index 0xe040
 *   [164-227]  IODWriteReq header of the write, index 0x802b
 *   [228-231]  Record data of the write
 *
 * Other fields than in write_req:
 *
 *   [74]       RPC body length: NDR header (20) and 2 headers (128) and the
 *              record data (4)
 *   [84]       NDR args length: 2 headers and the record data
 *   [96]       NDR actual count, same as the args length
 *   [134-135]  Index of the multiple write: 0xe040
 *   [136-139]  Record data length of the multiple write: header and data
 *   [198-199]  Index of the write: 0x802b
 *   [200-203]  Record data length of the write
 *
 * The result of the write is the second block of the response, after the
 * 80 byte RPC header, 4 byte PNIO status and NDR header (together 100 bytes)
 * and the result block of the multiple write (64 bytes). The PNIO status of the
 * write is in its result block at offset 44, so at 208 to 211.
 *
 * @param req              Out:   Buffer for the request, 232 bytes
 * @param record_data_length In:  The record data length of the write
 */
static void create_write_multiple_req (
   uint8_t * req,
   uint32_t record_data_length)
{
   memset (req, 0, 232);
   memcpy (req, write_req, 164);
   req[74] = 20 + 64 + 64 + 4;
   req[84] = 64 + 64 + 4;
   req[96] = 64 + 64 + 4;
   req[134] = 0xe0;
   req[135] = 0x40;
   req[139] = 64 + 4;
   memcpy (&req[164], &write_req[100], 64);
   req[198] = 0x80;
   req[199] = 0x2b;
   req[200] = (uint8_t)(record_data_length >> 24);
   req[201] = (uint8_t)(record_data_length >> 16);
   req[202] = (uint8_t)(record_data_length >> 8);
   req[203] = (uint8_t)record_data_length;
}

/**
 * A write in a multiple write where the record data is too short to contain the
 * block header of the record. See CmrpcIodWriteRecordShorterThanBlockHeader.
 */
TEST_F (CmrpcTest, CmrpcIodWriteMultipleRecordShorterThanBlockHeader)
{
   uint8_t req[232];

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   create_write_multiple_req (req, 4);
   mock_set_pnal_udp_recvfrom_buffer (req, sizeof (req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2);

   EXPECT_EQ (mock_os_data.udp_sendto_buffer[208], PNET_ERROR_CODE_WRITE);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[209], PNET_ERROR_DECODE_PNIORW);
   EXPECT_EQ (
      mock_os_data.udp_sendto_buffer[210],
      PNET_ERROR_CODE_1_ACC_WRITE_LENGTH_ERROR);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[211], 0);
}

/**
 * A write in a multiple write where the record data length is larger than the
 * rest of the request. The second length would wrap around if it was added to
 * the position. See CmrpcIodWriteRecordLongerThanRequest, and table 1373 in
 * 5.8.12.1 for multiple writes.
 */
TEST_F (CmrpcTest, CmrpcIodWriteMultipleRecordLongerThanRequest)
{
   uint8_t req[232];
   const uint32_t lengths[] = {4 + 1, 0xffffffff};
   uint32_t ix;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   for (ix = 0; ix < sizeof (lengths) / sizeof (lengths[0]); ix++)
   {
      create_write_multiple_req (req, lengths[ix]);
      mock_set_pnal_udp_recvfrom_buffer (req, sizeof (req));
      run_stack (TEST_UDP_DELAY);
      EXPECT_EQ (mock_os_data.udp_sendto_count, 2 + ix);

      EXPECT_EQ (mock_os_data.udp_sendto_buffer[208], PNET_ERROR_CODE_WRITE);
      EXPECT_EQ (mock_os_data.udp_sendto_buffer[209], PNET_ERROR_DECODE_PNIO);
      EXPECT_EQ (mock_os_data.udp_sendto_buffer[210], PNET_ERROR_CODE_1_CMRPC);
      EXPECT_EQ (
         mock_os_data.udp_sendto_buffer[211],
         PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
   }
}

/**
 * The record data length is smaller than the record data in the request, so it
 * is not consistent with the ArgsLength, which shall be the length of the
 * IODWriteReq header plus the record data length. This is reported as
 * "CMRPC: ArgsLength invalid", also if the record is too short to contain the
 * block header. See PN-AL-protocol (IEC 61158-6-10) 5.8.10.1, table 1367.
 */
TEST_F (CmrpcTest, CmrpcIodWriteRecordShorterThanRequest)
{
   uint8_t req[164 + sizeof (pf_block_header_t)];
   const uint32_t lengths[] = {0, sizeof (pf_block_header_t) - 1};
   uint16_t len;
   uint32_t ix;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   for (ix = 0; ix < sizeof (lengths) / sizeof (lengths[0]); ix++)
   {
      len = create_write_req (req, lengths[ix], sizeof (pf_block_header_t));
      mock_set_pnal_udp_recvfrom_buffer (req, len);
      run_stack (TEST_UDP_DELAY);
      EXPECT_EQ (mock_os_data.udp_sendto_count, 2 + ix);

      EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], PNET_ERROR_CODE_WRITE);
      EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], PNET_ERROR_DECODE_PNIO);
      EXPECT_EQ (mock_os_data.udp_sendto_buffer[82], PNET_ERROR_CODE_1_CMRPC);
      EXPECT_EQ (
         mock_os_data.udp_sendto_buffer[83],
         PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
   }
}

/**
 * The ArgsLength is shorter than the IODWriteReq header, so the arguments end
 * before the position where the record data starts. The number of bytes from
 * there to the end is then negative, which as an unsigned number is the
 * record data length of the request. This is reported as
 * "CMRPC: ArgsLength invalid".
 */
TEST_F (CmrpcTest, CmrpcIodWriteArgsLengthShorterThanHeader)
{
   uint8_t req[164 + sizeof (pf_block_header_t)];
   const uint16_t args_length = 10;
   uint16_t len;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   /* 64 bytes header, ends 54 bytes before the position after the header */
   len = create_write_req (
      req,
      (uint32_t)(args_length - 64),
      sizeof (pf_block_header_t));
   req[84] = args_length; /* NDR args length */
   req[96] = args_length; /* NDR actual count */
   mock_set_pnal_udp_recvfrom_buffer (req, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2);

   EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], PNET_ERROR_CODE_WRITE);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], PNET_ERROR_DECODE_PNIO);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[82], PNET_ERROR_CODE_1_CMRPC);
   EXPECT_EQ (
      mock_os_data.udp_sendto_buffer[83],
      PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
}

/**
 * The ArgsLength is checked before the header of the request, so a record data
 * length that is not consistent with it is reported as "CMRPC: ArgsLength
 * invalid" also if the AR UUID is unknown. With a consistent record data
 * length the unknown AR UUID is reported. See PN-AL-protocol (IEC 61158-6-10)
 * 5.8.10, where table 1367 says "check IODWriteReq-PDU" for a valid ArgsLength.
 */
TEST_F (CmrpcTest, CmrpcIodWriteArgsLengthCheckedBeforeArUuid)
{
   uint8_t req[164 + sizeof (pf_block_header_t)];
   uint16_t len;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   /* Record data length larger than the record data in the request */
   len = create_write_req (req, 7, sizeof (pf_block_header_t));
   req[108] ^= 0xff; /* AR UUID of the IODWriteReq header */
   mock_set_pnal_udp_recvfrom_buffer (req, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], PNET_ERROR_CODE_WRITE);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], PNET_ERROR_DECODE_PNIO);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[82], PNET_ERROR_CODE_1_CMRPC);
   EXPECT_EQ (
      mock_os_data.udp_sendto_buffer[83],
      PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);

   /* Consistent record data length */
   len = create_write_req (req, 4, 4);
   req[108] ^= 0xff;
   mock_set_pnal_udp_recvfrom_buffer (req, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 3);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], PNET_ERROR_CODE_WRITE);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], PNET_ERROR_DECODE_PNIO);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[82], PNET_ERROR_CODE_1_CMRPC);
   EXPECT_EQ (
      mock_os_data.udp_sendto_buffer[83],
      PNET_ERROR_CODE_2_CMRPC_AR_UUID_UNKNOWN);
}

/**
 * A write in a multiple write must be within the arguments of the request,
 * which end at the end of the request. The ArgsLength is the same as for
 * create_write_multiple_req().
 */
TEST_F (CmrpcTest, CmrpcIodWriteMultipleRecordBeyondArguments)
{
   uint8_t req[232];

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   /* The record data length is one more than the record data in the
      arguments */
   create_write_multiple_req (req, 4 + 1);
   mock_set_pnal_udp_recvfrom_buffer (req, sizeof (req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2);

   EXPECT_EQ (mock_os_data.udp_sendto_buffer[208], PNET_ERROR_CODE_WRITE);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[209], PNET_ERROR_DECODE_PNIO);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[210], PNET_ERROR_CODE_1_CMRPC);
   EXPECT_EQ (
      mock_os_data.udp_sendto_buffer[211],
      PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
}

/**
 * The ArgsLength is larger than the request, so the arguments are not in the
 * request. This is reported as "CMRPC: ArgsLength invalid", and nothing is
 * written.
 */
TEST_F (CmrpcTest, CmrpcIodWriteArgsLengthLongerThanRequest)
{
   uint8_t req[164 + sizeof (pf_block_header_t)];
   uint16_t len;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   len = create_write_req (req, 4, 4);
   req[80] = 250; /* NDR args maximum */
   req[84] = 200; /* NDR args length */
   req[88] = 250; /* NDR maximum count */
   req[96] = 200; /* NDR actual count */
   mock_set_pnal_udp_recvfrom_buffer (req, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2);
   EXPECT_EQ (appdata.call_counters.write_calls, 0);

   EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], PNET_ERROR_CODE_WRITE);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], PNET_ERROR_DECODE_PNIO);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[82], PNET_ERROR_CODE_1_CMRPC);
   EXPECT_EQ (
      mock_os_data.udp_sendto_buffer[83],
      PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
}

/**
 * The ArgsLength is smaller than the request, so the request has bytes after
 * the arguments. This is reported as "CMRPC: ArgsLength invalid", and nothing
 * is written.
 */
TEST_F (CmrpcTest, CmrpcIodWriteArgsLengthShorterThanRequest)
{
   uint8_t req[164 + sizeof (pf_block_header_t)];
   uint16_t len;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 1);

   len = create_write_req (req, 4, 4);
   req[84] = 64 + 4 - 1; /* NDR args length, one byte less than the request */
   req[96] = 64 + 4 - 1; /* NDR actual count */
   mock_set_pnal_udp_recvfrom_buffer (req, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (mock_os_data.udp_sendto_count, 2);
   EXPECT_EQ (appdata.call_counters.write_calls, 0);

   EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], PNET_ERROR_CODE_WRITE);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], PNET_ERROR_DECODE_PNIO);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[82], PNET_ERROR_CODE_1_CMRPC);
   EXPECT_EQ (
      mock_os_data.udp_sendto_buffer[83],
      PNET_ERROR_CODE_2_CMRPC_ARGSLENGTH_INVALID);
}

/**
 * A received alarm that cannot be parsed is dropped before it is acknowledged.
 * The APMR must not move to WCNF, where it waits for the confirmation of an
 * acknowledge, as nothing leaves WCNF other than that confirmation. See the
 * APMR state table, rows 7, 16 and 17, in PN-AL-protocol (IEC 61158-6-10)
 * 4.8.4.3.5.
 */
TEST_F (CmrpcTest, CmrpcAlarmAfterMalformedAlarm)
{
   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);

   /* An alarm notification that is cut off in the variable part is dropped */
   receive_alarm_frame (net, alarm_notification_frame, 50);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.alarm_calls, 0);

   /* The complete alarm notification, with the same sequence number as it is
      when retransmitted, must still be accepted */
   receive_alarm_frame (
      net,
      alarm_notification_frame,
      sizeof (alarm_notification_frame));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.alarm_calls, 1);
}

#if PNET_OPTION_MC_CR
/**
 * pf_ar_t has room for PNET_MAX_MC_CR MCR_REQ blocks (mcr_request[]). A
 * Connect request with more MCR_REQ blocks (type 0x0106) than that must be
 * rejected, instead of writing past the array.
 */
TEST_F (CmrpcTest, CmrpcConnectTooManyMcrBlocks)
{
   /* Two minimal MCR_REQ blocks (no provider station name) */
   const uint8_t mcr_block[14] = {
      0x01,
      0x06,
      0x00,
      0x0a,
      0x01,
      0x00,
      0x00,
      0x01,
      0x00,
      0x01,
      0x00,
      0x01,
      0x00,
      0x00};
   uint8_t req[sizeof (connect_req) + (PNET_MAX_MC_CR + 1) * sizeof (mcr_block)];
   uint16_t extra = (PNET_MAX_MC_CR + 1) * sizeof (mcr_block);
   uint16_t ix;

   memcpy (req, connect_req, sizeof (connect_req));
   for (ix = 0; ix < PNET_MAX_MC_CR + 1; ix++)
   {
      memcpy (
         &req[sizeof (connect_req) + ix * sizeof (mcr_block)],
         mcr_block,
         sizeof (mcr_block));
   }
   req[74] = (uint8_t) (req[74] + extra); /* RPC body length */
   req[84] = (uint8_t) (req[84] + extra); /* NDR args length */
   req[96] = (uint8_t) (req[96] + extra); /* NDR actual count */

   mock_set_pnal_udp_recvfrom_buffer (req, sizeof (req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 0);
}
#endif

/* Offset of the activity UUID in the RPC header */
#define RPC_ACTIVITY_UUID_POS 40

/**
 * Create a connectionless RPC PDU of another type from the RPC header of a
 * request or response, with a body of 8 bytes.
 *
 *   [01]       Packet type
 *   [64-67]    Sequence number
 *   [74-75]    Length of body: 8
 *
 * @param pdu              Out:   Buffer for the PDU, 88 bytes
 * @param header           In:    The first 80 bytes are the RPC header
 * @param packet_type      In:    The packet type
 * @param sequence_nmb     In:    The sequence number
 * @return the length of the PDU
 */
static uint16_t create_rpc_pdu (
   uint8_t * pdu,
   const uint8_t * header,
   uint8_t packet_type,
   uint32_t sequence_nmb)
{
   memset (pdu, 0, 88);
   memcpy (pdu, header, 80);
   pdu[1] = packet_type;
   pdu[64] = (uint8_t)sequence_nmb;
   pdu[65] = (uint8_t)(sequence_nmb >> 8);
   pdu[66] = (uint8_t)(sequence_nmb >> 16);
   pdu[67] = (uint8_t)(sequence_nmb >> 24);
   pdu[74] = 8;
   pdu[75] = 0;

   return 88;
}

/**
 * Reject, fault and cancel PDUs concern one call, which is identified by the
 * activity UUID and the sequence number. They are only handled for the call
 * that the device has sent, which is the application ready request, and
 * abort the AR then. See DCE 1.1: RPC, 9.5.3, 12.5.3.3, 12.5.3.5 and
 * 12.5.3.8.
 */
TEST_F (CmrpcTest, CmrpcRejectForCallOfDevice)
{
   uint8_t pdu[88];
   uint16_t len;
   int ret;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   mock_set_pnal_udp_recvfrom_buffer (prm_end_req, sizeof (prm_end_req));
   run_stack (TEST_UDP_DELAY);
   ret = pnet_application_ready (net, appdata.main_arep);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   /* Another call than the application ready request (sequence number 0) */
   len = create_rpc_pdu (pdu, appl_rdy_rsp, PF_RPC_PT_REJECT, 1);
   mock_set_pnal_udp_recvfrom_buffer (pdu, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   len = create_rpc_pdu (pdu, appl_rdy_rsp, PF_RPC_PT_FAULT, 1);
   mock_set_pnal_udp_recvfrom_buffer (pdu, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   /* A cancel never aborts the AR */
   len = create_rpc_pdu (pdu, appl_rdy_rsp, PF_RPC_PT_CL_CANCEL, 0);
   mock_set_pnal_udp_recvfrom_buffer (pdu, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   /* A reject of the application ready request aborts the AR */
   len = create_rpc_pdu (pdu, appl_rdy_rsp, PF_RPC_PT_REJECT, 0);
   mock_set_pnal_udp_recvfrom_buffer (pdu, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);
}

TEST_F (CmrpcTest, CmrpcFaultForCallOfDevice)
{
   uint8_t pdu[88];
   uint16_t len;
   int ret;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   mock_set_pnal_udp_recvfrom_buffer (prm_end_req, sizeof (prm_end_req));
   run_stack (TEST_UDP_DELAY);
   ret = pnet_application_ready (net, appdata.main_arep);
   EXPECT_EQ (ret, 0);

   len = create_rpc_pdu (pdu, appl_rdy_rsp, PF_RPC_PT_FAULT, 0);
   mock_set_pnal_udp_recvfrom_buffer (pdu, len);
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_ABORT);
}

/**
 * The session of the connect request is the session of the requests of the
 * controller. The device has not sent any call in it, so a reject, fault or
 * cancel for it does not abort the AR, and the session is still used.
 */
TEST_F (CmrpcTest, CmrpcRejectOnSessionOfController)
{
   uint8_t pdu[88];
   uint16_t len;
   const uint8_t types[] =
      {PF_RPC_PT_REJECT, PF_RPC_PT_FAULT, PF_RPC_PT_CL_CANCEL};
   uint32_t ix;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);

   /* The sequence number is 0 for the first request, and 0xffffffff is the
      one that the device would have sent last in a session of its own */
   for (ix = 0; ix < sizeof (types); ix++)
   {
      len = create_rpc_pdu (pdu, connect_req, types[ix], 0);
      mock_set_pnal_udp_recvfrom_buffer (pdu, len);
      run_stack (TEST_UDP_DELAY);
      EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);

      len = create_rpc_pdu (pdu, connect_req, types[ix], 0xffffffff);
      mock_set_pnal_udp_recvfrom_buffer (pdu, len);
      run_stack (TEST_UDP_DELAY);
      EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   }

   mock_set_pnal_udp_recvfrom_buffer (write_req, sizeof (write_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.write_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
}

/**
 * Reject, fault and cancel PDUs that are not the answer to a call of the device
 * are silently dropped. No session is allocated for them, and no session is
 * changed, also not the address of the session of the call of the device.
 */
TEST_F (CmrpcTest, CmrpcRejectDroppedWithoutChangingSessions)
{
   const uint32_t foreign_ip = PNAL_MAKEU32 (192, 168, 1, 172);
   const uint16_t foreign_port = 4321;
   const uint8_t types[] =
      {PF_RPC_PT_REJECT, PF_RPC_PT_FAULT, PF_RPC_PT_CL_CANCEL};
   uint8_t pdu[88];
   uint8_t res[1500];
   uint16_t res_len = sizeof (res);
   bool close_socket = false;
   pf_session_info_t * p_call_sess = NULL;
   uint16_t sessions;
   uint16_t sent;
   uint16_t len;
   uint32_t ix;
   uint32_t iy;
   int ret;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   mock_set_pnal_udp_recvfrom_buffer (prm_end_req, sizeof (prm_end_req));
   run_stack (TEST_UDP_DELAY);
   ret = pnet_application_ready (net, appdata.main_arep);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   sessions = 0;
   for (iy = 0; iy < PF_MAX_SESSION; iy++)
   {
      if (net->cmrpc_session_info[iy].in_use)
      {
         sessions++;
         if (net->cmrpc_session_info[iy].from_me)
         {
            p_call_sess = &net->cmrpc_session_info[iy];
         }
      }
   }
   ASSERT_NE (p_call_sess, nullptr);
   const uint32_t call_ip = p_call_sess->ip_addr;
   const uint16_t call_port = p_call_sess->port;

   for (ix = 0; ix < sizeof (types); ix++)
   {
      /* With the activity UUID of the call of the device, but another
         sequence number */
      len = create_rpc_pdu (pdu, appl_rdy_rsp, types[ix], 1);
      ret = pf_cmrpc_dce_packet (
         net,
         foreign_ip,
         foreign_port,
         pdu,
         len,
         res,
         &res_len,
         &close_socket);
      EXPECT_EQ (ret, 0);
      EXPECT_EQ (p_call_sess->ip_addr, call_ip);
      EXPECT_EQ (p_call_sess->port, call_port);
      EXPECT_TRUE (p_call_sess->in_use);

      /* With an activity UUID that is not used */
      len = create_rpc_pdu (pdu, appl_rdy_rsp, types[ix], 0);
      pdu[40] ^= 0xff;
      ret = pf_cmrpc_dce_packet (
         net,
         foreign_ip,
         foreign_port,
         pdu,
         len,
         res,
         &res_len,
         &close_socket);
      EXPECT_EQ (ret, 0);
   }
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);

   iy = 0;
   for (ix = 0; ix < PF_MAX_SESSION; ix++)
   {
      iy += net->cmrpc_session_info[ix].in_use ? 1 : 0;
   }
   EXPECT_EQ (iy, sessions);

   /* The cancel is dropped also for the call of the device, without a
      cancel_ack, as the device is the client in that session */
   sent = mock_os_data.udp_sendto_count;
   len = create_rpc_pdu (pdu, appl_rdy_rsp, PF_RPC_PT_CL_CANCEL, 0);
   ret = pf_cmrpc_dce_packet (
      net,
      call_ip,
      call_port,
      pdu,
      len,
      res,
      &res_len,
      &close_socket);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (mock_os_data.udp_sendto_count, sent);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_APPLRDY);
}

/**
 * A server answers a cancel for the call that it received last with a
 * cancel_ack. The device can not cancel the processing of a call, so the
 * cancel_ack has no body, which acknowledges that the call is orphaned. A
 * cancel for another call, another activity or a session of the device is
 * silently dropped. See DCE 1.1: RPC, 12.5.3.2 and 10.2.
 */
TEST_F (CmrpcTest, CmrpcCancelAck)
{
   const uint32_t ip = PNAL_MAKEU32 (192, 168, 1, 170);
   const uint16_t port = 49152;
   uint8_t pdu[88];
   uint8_t res[1500];
   uint16_t res_len;
   bool close_socket = false;
   uint16_t len;
   uint16_t sent;
   int ret;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   sent = mock_os_data.udp_sendto_count;

   /* The call that was received last has the sequence number 0 */
   len = create_rpc_pdu (pdu, connect_req, PF_RPC_PT_CL_CANCEL, 0);
   memset (res, 0xee, sizeof (res));
   res_len = sizeof (res);
   ret = pf_cmrpc_dce_packet (
      net,
      ip,
      port,
      pdu,
      len,
      res,
      &res_len,
      &close_socket);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (mock_os_data.udp_sendto_count, sent + 1);
   EXPECT_EQ (mock_os_data.udp_sendto_len, 80); /* The header only */
   EXPECT_EQ (res[1], PF_RPC_PT_CANCEL_ACK);
   EXPECT_EQ (res[74], 0); /* No body */
   EXPECT_EQ (res[75], 0);
   EXPECT_EQ (0, memcmp (&res[40], &connect_req[40], 16)); /* Activity UUID */
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);

   /* Another call */
   len = create_rpc_pdu (pdu, connect_req, PF_RPC_PT_CL_CANCEL, 1);
   res_len = sizeof (res);
   ret = pf_cmrpc_dce_packet (
      net,
      ip,
      port,
      pdu,
      len,
      res,
      &res_len,
      &close_socket);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (mock_os_data.udp_sendto_count, sent + 1);

   /* Another activity */
   len = create_rpc_pdu (pdu, connect_req, PF_RPC_PT_CL_CANCEL, 0);
   pdu[RPC_ACTIVITY_UUID_POS] ^= 0xff;
   res_len = sizeof (res);
   ret = pf_cmrpc_dce_packet (
      net,
      ip,
      port,
      pdu,
      len,
      res,
      &res_len,
      &close_socket);
   EXPECT_EQ (ret, 0);
   EXPECT_EQ (mock_os_data.udp_sendto_count, sent + 1);

   /* The session is still used, and the AR is not aborted */
   mock_set_pnal_udp_recvfrom_buffer (write_req, sizeof (write_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.write_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
}

/**
 * Hand a fragment that is not the first one to the stack, with the RPC header
 * of connect_req but the opposite endianness, and an 8 byte body.
 *
 * The fields that are changed, compared to connect_req:
 *
 *   [02]       Flags: fragment, not the last fragment
 *   [04]       RPCDRep.IntegerEncoding: big endian instead of little endian
 *   [40-47]    Activity UUID: Data1, Data2 and Data3 byte swapped, so that it
 *              is the same UUID in the new endianness
 *   [74-75]    Length of body: 8, big endian
 *   [76-77]    Fragment number: 1, big endian
 *
 * @param net              InOut: The p-net stack instance
 */
static void receive_fragment_with_other_endianness (pnet_t * net)
{
   uint8_t pdu[88];
   uint8_t res[1500];
   uint16_t res_len = sizeof (res);
   bool close_socket = false;
   uint8_t t;

   memset (pdu, 0, sizeof (pdu));
   memcpy (pdu, connect_req, 80);
   pdu[2] |= (1 << PF_RPC_F_FRAGMENT);
   pdu[2] &= ~(uint8_t)(1 << PF_RPC_F_LAST_FRAGMENT);
   pdu[4] ^= 0x10;
   t = pdu[40];
   pdu[40] = pdu[43];
   pdu[43] = t;
   t = pdu[41];
   pdu[41] = pdu[42];
   pdu[42] = t;
   t = pdu[44];
   pdu[44] = pdu[45];
   pdu[45] = t;
   t = pdu[46];
   pdu[46] = pdu[47];
   pdu[47] = t;
   pdu[74] = 0;
   pdu[75] = 8;
   pdu[76] = 0;
   pdu[77] = 1;

   (void)pf_cmrpc_dce_packet (
      net,
      PNAL_MAKEU32 (192, 168, 1, 170),
      49152,
      pdu,
      sizeof (pdu),
      res,
      &res_len,
      &close_socket);
}

/**
 * A fragment with another endianness than the first fragment of the call is
 * not supported, so the call is rejected with nca_proto_error and the fragment
 * is not added. The AR of the session is not affected, so a PDU like this can
 * not remove an AR.
 */
TEST_F (CmrpcTest, CmrpcFragmentEndiannessMismatchKeepsAr)
{
   uint16_t sent;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);

   /* The first fragment of a request on the session of the AR, with the
      header of connect_req and an 8 byte body. The fragment with the other
      endianness has the next fragment number. */
   {
      uint8_t pdu[88];
      uint8_t res[1500];
      uint16_t res_len = sizeof (res);
      bool close_socket = false;

      memset (pdu, 0, sizeof (pdu));
      memcpy (pdu, connect_req, 80);
      pdu[2] |= (1 << PF_RPC_F_FRAGMENT);
      pdu[2] &= ~(uint8_t)(1 << PF_RPC_F_LAST_FRAGMENT);
      pdu[74] = 8; /* Length of body */
      pdu[75] = 0;
      (void)pf_cmrpc_dce_packet (
         net,
         PNAL_MAKEU32 (192, 168, 1, 170),
         49152,
         pdu,
         sizeof (pdu),
         res,
         &res_len,
         &close_socket);
   }
   EXPECT_EQ (net->cmrpc_session_info[0].in_buf_len, 8u);
   sent = mock_os_data.udp_sendto_count;

   receive_fragment_with_other_endianness (net);
   EXPECT_EQ (mock_os_data.udp_sendto_count, sent + 1);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[1], PF_RPC_PT_REJECT);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[80], 0x0B); /* nca_proto_error */
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[81], 0x00);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[82], 0x01);
   EXPECT_EQ (mock_os_data.udp_sendto_buffer[83], 0x1C);
   EXPECT_EQ (net->cmrpc_session_info[0].in_buf_len, 8u);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
   EXPECT_TRUE (net->cmrpc_ar[0].in_use);

   /* The session of the AR is still used */
   mock_set_pnal_udp_recvfrom_buffer (write_req, sizeof (write_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.write_calls, 1);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);
}

/**
 * Hand a fragment of a request to the stack, with the RPC header of
 * connect_req and an 8 byte body.
 *
 * The fields that are changed, compared to connect_req:
 *
 *   [02]       Flags: fragment, and last fragment if last is set
 *   [40]       First octet of the activity UUID, if activity is not 0
 *   [74-75]    Length of body: 8
 *   [76-77]    Fragment number
 *
 * @param net              InOut: The p-net stack instance
 * @param activity         In:    First octet of the activity UUID, or 0 to
 *                                keep the one of connect_req.
 * @param fragment_nmb     In:    The fragment number
 * @param last             In:    true if this is the last fragment
 */
static void receive_request_fragment (
   pnet_t * net,
   uint8_t activity,
   uint16_t fragment_nmb,
   bool last)
{
   uint8_t pdu[88];
   uint8_t res[1500];
   uint16_t res_len = sizeof (res);
   bool close_socket = false;

   memset (pdu, 0, sizeof (pdu));
   memcpy (pdu, connect_req, 80);
   pdu[2] |= (1 << PF_RPC_F_FRAGMENT);
   if (last)
   {
      pdu[2] |= (1 << PF_RPC_F_LAST_FRAGMENT);
   }
   else
   {
      pdu[2] &= ~(uint8_t)(1 << PF_RPC_F_LAST_FRAGMENT);
   }
   if (activity != 0)
   {
      pdu[40] = activity;
   }
   pdu[74] = 8;
   pdu[75] = 0;
   pdu[76] = (uint8_t)fragment_nmb;
   pdu[77] = (uint8_t)(fragment_nmb >> 8);

   (void)pf_cmrpc_dce_packet (
      net,
      PNAL_MAKEU32 (192, 168, 1, 170),
      49152,
      pdu,
      sizeof (pdu),
      res,
      &res_len,
      &close_socket);
}

static unsigned sessions_in_use (pnet_t * net)
{
   unsigned in_use = 0;
   unsigned ix;

   for (ix = 0; ix < PF_MAX_SESSION; ix++)
   {
      in_use += net->cmrpc_session_info[ix].in_use;
   }
   return in_use;
}

/**
 * The first fragment of a request for an unknown activity UUID allocates a
 * session. If the rest of the request is not received in time, the session
 * is released, as it has no AR. Otherwise PF_MAX_SESSION such fragments
 * would leave no session for a Connect.
 *
 * The timeout is 8 s, 4 * Timeout FRAG (PN-AL-protocol, 4.10.5.2), and it
 * is restarted for every fragment.
 */
TEST_F (CmrpcTest, CmrpcSessionReleasedIfFirstRequestIsNotCompleted)
{
   uint8_t ix;

   for (ix = 0; ix < PF_MAX_SESSION; ix++)
   {
      receive_request_fragment (net, (uint8_t)(0x40 + ix), 0, false);
   }
   EXPECT_EQ (sessions_in_use (net), PF_MAX_SESSION);

   /* The next fragment restarts the timeout */
   run_stack (6 * 1000 * 1000);
   receive_request_fragment (net, 0x40, 1, false);
   run_stack (3 * 1000 * 1000);
   EXPECT_EQ (sessions_in_use (net), 1u);

   run_stack (6 * 1000 * 1000);
   EXPECT_EQ (sessions_in_use (net), 0u);

   /* A Connect is now answered */
   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
}

/**
 * A fragment that is not accepted, here one that is not the first fragment
 * of a request for an unknown activity UUID, also allocates a session. It
 * must be released in the same way.
 */
TEST_F (CmrpcTest, CmrpcSessionReleasedAfterUnexpectedFragment)
{
   receive_request_fragment (net, 0x40, 1, false);
   EXPECT_EQ (sessions_in_use (net), 1u);

   run_stack (9 * 1000 * 1000);
   EXPECT_EQ (sessions_in_use (net), 0u);
}

/**
 * The timer for the rest of a fragmented request is stopped when the request
 * is complete, here a Connect request in two fragments.
 */
TEST_F (CmrpcTest, CmrpcSessionTimerStoppedWhenRequestIsComplete)
{
   pf_session_info_t * p_sess = &net->cmrpc_session_info[0];

   mock_set_pnal_udp_recvfrom_buffer (
      connect_frag_1_req,
      sizeof (connect_frag_1_req));
   run_stack (TEST_UDP_DELAY);
   ASSERT_TRUE (p_sess->in_use);
   EXPECT_TRUE (pf_scheduler_is_running (&p_sess->call_timeout));

   mock_set_pnal_udp_recvfrom_buffer (
      connect_frag_2_req,
      sizeof (connect_frag_2_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_TRUE (p_sess->in_use);
   EXPECT_FALSE (pf_scheduler_is_running (&p_sess->call_timeout));
}

/**
 * The session of an EPM lookup is kept for the next lookup request of the
 * sequence, as the EPM timeout releases it.
 */
TEST_F (CmrpcTest, CmrpcSessionOfEpmLookupKept)
{
   /* An ept_lookup request, with the RPC header of connect_req */
   const uint8_t epmap_interface[] = {
      0x08, 0x83, 0xAF, 0xE1, 0x1F, 0x5D, 0xC9, 0x11,
      0x91, 0xA4, 0x08, 0x00, 0x2B, 0x14, 0xA0, 0xFA};
   uint8_t pdu[80 + 76];
   uint8_t res[1500];
   uint16_t res_len = sizeof (res);
   bool close_socket = false;
   pf_session_info_t * p_sess = &net->cmrpc_session_info[0];

   memset (pdu, 0, sizeof (pdu));
   memcpy (pdu, connect_req, 80);
   pdu[2] = 0;  /* Flags */
   memcpy (&pdu[24], epmap_interface, sizeof (epmap_interface));
   pdu[40] = 0x40; /* Activity UUID */
   pdu[68] = 2;    /* Opnum ept_lookup */
   pdu[69] = 0;
   pdu[74] = 76; /* Length of body */
   pdu[75] = 0;
   pdu[80 + 72] = 1; /* max_ents */
   (void)pf_cmrpc_dce_packet (
      net,
      PNAL_MAKEU32 (192, 168, 1, 170),
      49152,
      pdu,
      sizeof (pdu),
      res,
      &res_len,
      &close_socket);
   ASSERT_TRUE (p_sess->in_use);
   EXPECT_TRUE (pf_scheduler_is_running (&p_sess->epm_timeout));
}

/**
 * Receive a PDU without a body, in the activity of connect_req.
 *
 * @param net              InOut: The p-net stack instance
 * @param activity         In:    The first byte of the activity UUID.
 * @param packet_type      In:    The packet type.
 */
static void receive_pdu_without_body (
   pnet_t * net,
   uint8_t activity,
   uint8_t packet_type)
{
   uint8_t pdu[80];
   uint8_t res[1500];
   uint16_t res_len = sizeof (res);
   bool close_socket = false;

   memcpy (pdu, connect_req, sizeof (pdu));
   pdu[1] = packet_type;
   pdu[2] = 0;           /* Flags */
   pdu[40] = activity;   /* Activity UUID */
   pdu[74] = 0;          /* Length of body */
   pdu[75] = 0;
   (void)pf_cmrpc_dce_packet (
      net,
      PNAL_MAKEU32 (192, 168, 1, 170),
      49152,
      pdu,
      sizeof (pdu),
      res,
      &res_len,
      &close_socket);
}

/**
 * A PDU of another type than a request, here an ack, on a new activity does
 * not leave a session behind.
 */
TEST_F (CmrpcTest, CmrpcNoSessionKeptAfterOtherPduOnNewActivity)
{
   receive_pdu_without_body (net, 0x40, PF_RPC_PT_ACK);
   EXPECT_EQ (sessions_in_use (net), 0u);
}

/**
 * A PDU that is not a fragment ends the request that is received. A session
 * without an AR is then not monitored, so it is released after a ping or a
 * PDU of another type.
 */
TEST_F (CmrpcTest, CmrpcSessionWithoutArReleasedAfterPingOrOtherPdu)
{
   receive_request_fragment (net, 0x40, 0, false);
   ASSERT_EQ (sessions_in_use (net), 1u);
   receive_pdu_without_body (net, 0x40, PF_RPC_PT_PING);
   EXPECT_EQ (sessions_in_use (net), 0u);

   receive_request_fragment (net, 0x40, 0, false);
   ASSERT_EQ (sessions_in_use (net), 1u);
   receive_pdu_without_body (net, 0x40, PF_RPC_PT_ACK);
   EXPECT_EQ (sessions_in_use (net), 0u);
}

/**
 * A session that does not get an AR is released after the response, as it is
 * not monitored by an AR. Otherwise PF_MAX_SESSION requests, each with a
 * distinct activity UUID, would leave no session for a Connect.
 */
TEST_F (CmrpcTest, CmrpcSessionWithoutArReleasedAfterResponse)
{
   uint8_t pdu[88];
   uint8_t res[1500];
   uint16_t res_len;
   bool close_socket = false;
   uint8_t ix;

   for (ix = 0; ix < PF_MAX_SESSION; ix++)
   {
      /* A request with an unknown operation number, in one PDU */
      memset (pdu, 0, sizeof (pdu));
      memcpy (pdu, connect_req, 80);
      pdu[40] = (uint8_t)(0x40 + ix); /* Activity UUID */
      pdu[68] = 0x55;                  /* Operation number */
      pdu[74] = 8;                     /* Length of body */
      res_len = sizeof (res);
      (void)pf_cmrpc_dce_packet (
         net,
         PNAL_MAKEU32 (192, 168, 1, 170),
         49152,
         pdu,
         sizeof (pdu),
         res,
         &res_len,
         &close_socket);
   }
   EXPECT_EQ (sessions_in_use (net), 0u);

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (sessions_in_use (net), 1u);
}

/**
 * A session with an AR is not timed out if the rest of a fragmented request
 * is not received, as it is monitored by the AR. The fragments that were
 * received are dropped when the next call starts.
 */
TEST_F (CmrpcTest, CmrpcSessionWithArNotTimedOut)
{
   pf_session_info_t * p_sess = NULL;
   unsigned ix;

   mock_set_pnal_udp_recvfrom_buffer (connect_req, sizeof (connect_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.connect_calls, 1);
   EXPECT_EQ (sessions_in_use (net), 1u);
   for (ix = 0; ix < PF_MAX_SESSION; ix++)
   {
      if (net->cmrpc_session_info[ix].in_use)
      {
         p_sess = &net->cmrpc_session_info[ix];
      }
   }
   ASSERT_NE (p_sess, nullptr);

   /* The first fragment of a request on the session of the Connect */
   receive_request_fragment (net, 0, 0, false);
   EXPECT_EQ (p_sess->in_fragment_nbr, 1u);
   EXPECT_FALSE (pf_scheduler_is_running (&p_sess->call_timeout));

   run_stack (9 * 1000 * 1000);
   EXPECT_TRUE (p_sess->in_use);
   EXPECT_NE (p_sess->p_ar, nullptr);
   EXPECT_EQ (appdata.cmdev_state, PNET_EVENT_STARTUP);

   /* The next call */
   mock_set_pnal_udp_recvfrom_buffer (write_req, sizeof (write_req));
   run_stack (TEST_UDP_DELAY);
   EXPECT_EQ (appdata.call_counters.write_calls, 1);
   EXPECT_EQ (p_sess->in_fragment_nbr, 0u);
}

TEST_F (CmrpcTest, CmrpcImplicitRmReadInd)
{
   static pf_session_info_t session_info;
   uint16_t res_pos;

   memset (&session_info, 0, sizeof (session_info));
   session_info.ndr_data.args_length = sizeof (iod_read_req_implicit_invalid);
   session_info.get_info.is_big_endian = true;
   session_info.get_info.p_buf = (uint8_t *)&iod_read_req_implicit_invalid;
   session_info.get_info.len = sizeof (iod_read_req_implicit_invalid);
   res_pos = 0;
   EXPECT_EQ (
      -1,
      pf_cmrpc_rm_read_ind_test (
         net,
         &session_info,
         PF_RPC_DEV_OPNUM_READ_IMPLICIT,
         0,
         PNET_MAX_SESSION_BUFFER_SIZE,
         session_info.out_buffer,
         &res_pos));
   EXPECT_EQ (PF_PARSE_OK, session_info.get_info.result);
   EXPECT_EQ (
      PNET_ERROR_CODE_READ,
      session_info.rpc_result.pnio_status.error_code);
   EXPECT_EQ (
      PNET_ERROR_DECODE_PNIO,
      session_info.rpc_result.pnio_status.error_decode);
   EXPECT_EQ (
      PNET_ERROR_CODE_1_CONN_FAULTY_FAULTY_RECORD,
      session_info.rpc_result.pnio_status.error_code_1);
   EXPECT_EQ (
      PNET_ERROR_CODE_2_CONN_FAULTY_FAULTY_RECORD_BAD_TARGET_ARUUID,
      session_info.rpc_result.pnio_status.error_code_2);
}

TEST_F (CmrpcUnitTest, CmrpcCheckGenerateUuid)
{
   uint32_t timestamp;
   uint32_t session_number;
   pnet_ethaddr_t mac_address;
   pf_uuid_t uuid;

   timestamp = 0xC1C2C3C4;
   session_number = 0xB1B2B3B4;
   mac_address.addr[0] = 0xA1;
   mac_address.addr[1] = 0xA2;
   mac_address.addr[2] = 0xA3;
   mac_address.addr[3] = 0xA4;
   mac_address.addr[4] = 0xA5;
   mac_address.addr[5] = 0xA6;

   pf_generate_uuid (timestamp, session_number, mac_address, &uuid);

   EXPECT_EQ (uuid.data1, timestamp);
   EXPECT_EQ (uuid.data2, 0xB1B2);
   EXPECT_EQ (uuid.data3, 0x10B3);
   EXPECT_EQ (uuid.data4[0], 0x80);
   EXPECT_EQ (uuid.data4[1], 0xB4);
   EXPECT_EQ (uuid.data4[2], 0xA1);
   EXPECT_EQ (uuid.data4[3], 0xA2);
   EXPECT_EQ (uuid.data4[4], 0xA3);
   EXPECT_EQ (uuid.data4[5], 0xA4);
   EXPECT_EQ (uuid.data4[6], 0xA5);
   EXPECT_EQ (uuid.data4[7], 0xA6);
}
