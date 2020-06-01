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


class CminaTest : public PnetIntegrationTest {};


TEST_F (CminaTest, CmdevCheckIsStationNameValid)
{
   EXPECT_EQ (0, pf_cmina_is_stationname_valid("", 0));
   EXPECT_EQ (0, pf_cmina_is_stationname_valid("abc", 3));
   EXPECT_EQ (0, pf_cmina_is_stationname_valid("a1.2.3.4", 8));

   // TODO add lots of testcases

   //EXPECT_EQ (-1, pf_cmina_is_stationname_valid("1.2.3.4"));
}

TEST_F (CminaTest, CmdevCheckIsNetmaskValid)
{
   EXPECT_EQ (0, pf_cmina_is_netmask_valid(OS_MAKEU32(255, 255, 255, 0)));

   // TODO add lots of testcases

   //EXPECT_EQ (-1, pf_cmina_is_netmask_valid(OS_MAKEU32(255, 0, 255, 255)));
}

TEST_F (CminaTest, CmdevCheckIsIPaddressValid)
{
   EXPECT_EQ (0, pf_cmina_is_ipaddress_valid(OS_MAKEU32(255, 255,255, 0), OS_MAKEU32(192, 168, 1, 1)));

   // TODO add lots of testcases

   //EXPECT_EQ (-1, pf_cmina_is_netmask_valid(OS_MAKEU32(255, 0, 255, 255)));
}


TEST_F (CminaTest, CmdevCheckIsGatewayValid)
{
   EXPECT_EQ (0, pf_cmina_is_gateway_valid(OS_MAKEU32(192, 168, 1, 4), OS_MAKEU32(255, 255,255, 0), OS_MAKEU32(192, 168, 1, 1)));

   // TODO add lots of testcases

}
