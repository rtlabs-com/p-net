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

class CpmUnitTest : public PnetUnitTest
{
};

TEST_F (CpmUnitTest, CpmCheckCycle)
{
   EXPECT_EQ (-1, pf_cpm_check_cycle (1, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (1, 0));
   EXPECT_EQ (-1, pf_cpm_check_cycle (1, 1));
   EXPECT_EQ (0, pf_cpm_check_cycle (1, 2));
   EXPECT_EQ (0, pf_cpm_check_cycle (1, 3));

   /* Previous counter value was 0x4000 */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x0000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x0001));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x2FFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x2FFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x3000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3002));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3FFD));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3FFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3FFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x4000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x4001));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x4002));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0xFFFF));

   /* Previous counter value was 0xFFFF (Forbidden zone ends at 0xFFFF) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0xEFFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0xEFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0xFFFF, 0xF000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0xFFFF, 0xF001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0xFFFF, 0xFFFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0xFFFF, 0xFFFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0x0000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0x0001));
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0x0002));

   /* Previous counter value was 0x0000 (Forbidden zone ends at 0x0000) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0xEFFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0xEFFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0xF000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0000, 0xF001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0000, 0xFFFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0000, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0000, 0x0000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0x0001));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0x0002));

   /* Previous counter value was 0x0FFE (Forbidden zone starts at 0xFFFF) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFE, 0xFFFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFE, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFE, 0x0000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFE, 0x0001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFE, 0x0FFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFE, 0x0FFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFE, 0x1000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFE, 0x1001));

   /* Previous counter value was 0x0FFF (Forbidden zone starts at zero) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFF, 0xFFFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFF, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFF, 0x0000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFF, 0x0001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFF, 0x0FFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFF, 0x0FFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFF, 0x1000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFF, 0x1001));

   /* Previous counter value was 0x0010 (Forbidden zone overlaps zero) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0010, 0xF00F));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0010, 0xF010));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0xF011));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0xFFFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x0000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x0001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x000E));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x000F));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x0010));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0010, 0x0011));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0010, 0x0012));
}
