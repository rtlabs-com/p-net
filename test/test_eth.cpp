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

#include "pf_includes.h"

#include <gtest/gtest.h>

static uint8_t eth_frame_short[] = {0x88};
static uint8_t eth_frame_vlan_only[] =
   {0x81, 0x00, 0xab, 0xcd, 0x81, 0x00, 0xfe, 0xdc};
static uint8_t eth_frame_cutoff_vlan[] =
   {0x81, 0x00, 0xab};
static uint8_t eth_frame_short_vlan[] =
   {0x81, 0x00, 0xab, 0xcd, 0x88};
static uint8_t eth_frame_short_profinet[] = {0x88, 0x92, 0xfe};
static uint8_t eth_frame_short_vlan_profinet[] =
   {0x81, 0x00, 0x55, 0xaa, 0x88, 0x92, 0xfe};

class EthUnitTest : public PnetUnitTest
{
};

class EthTest : public PnetIntegrationTest
{
};

TEST_F (EthTest, EthRunTest)
{
}

TEST_F (EthUnitTest, EthRecvShortFrames)
{
   pnal_buf_t frame;

   frame.payload = eth_frame_short;
   frame.len = sizeof (eth_frame_short);
   EXPECT_EQ (0, pf_eth_recv (NULL, NULL, &frame));

   frame.payload = eth_frame_vlan_only;
   frame.len = sizeof (eth_frame_vlan_only);
   EXPECT_EQ (0, pf_eth_recv (NULL, NULL, &frame));

   frame.payload = eth_frame_cutoff_vlan;
   frame.len = sizeof (eth_frame_cutoff_vlan);
   EXPECT_EQ (0, pf_eth_recv (NULL, NULL, &frame));

   frame.payload = eth_frame_short_vlan;
   frame.len = sizeof (eth_frame_short_vlan);
   EXPECT_EQ (0, pf_eth_recv (NULL, NULL, &frame));

   frame.payload = eth_frame_short_profinet;
   frame.len = sizeof (eth_frame_short_profinet);
   EXPECT_EQ (0, pf_eth_recv (NULL, NULL, &frame));

   frame.payload = eth_frame_short_vlan_profinet;
   frame.len = sizeof (eth_frame_short_vlan_profinet);
   EXPECT_EQ (0, pf_eth_recv (NULL, NULL, &frame));
}
