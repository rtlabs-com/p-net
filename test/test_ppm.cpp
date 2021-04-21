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

class PpmTest : public PnetIntegrationTest
{
};

class PpmUnitTest : public PnetUnitTest
{
};

TEST_F (PpmTest, PpmTestCalculateCycle)
{
   /* Cycle length 31.25 us, transmission period 31.25 us */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 1, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (20, 1, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (40, 1, 1), 1);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (70, 1, 1), 2);

   /* Cycle length 62.5 us, transmission period 62.5 us */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 2, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (10, 2, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (50, 2, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (80, 2, 1), 2);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (100, 2, 1), 2);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (140, 2, 1), 4);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (160, 2, 1), 4);

   /* Cycle length 62.5 us, transmission period 500 us */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 2, 8), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (250, 2, 8), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (750, 2, 8), 16);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (1250, 2, 8), 32);

   /* Cycle length 125 us, transmission period 500 us */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 4, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (250, 4, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (750, 4, 4), 16);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (1250, 4, 4), 32);

   /* Cycle length 1 ms, transmission period 4 ms */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 32, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (3000, 32, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (5000, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (7000, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (9000, 32, 4), 256);
}

TEST_F (PpmTest, PpmTestCalculateNextCycle)
{
   /* Cycle length 31.25 us, transmission period 31.25 us */
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0, 1, 1), 1);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (1, 1, 1), 2);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (2, 1, 1), 3);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (3, 1, 1), 4);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (4, 1, 1), 5);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (5, 1, 1), 6);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (6, 1, 1), 7);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFF, 1, 1), 0);

   /* Cycle length 62.5 us, transmission period 62.5 us */
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0, 2, 1), 2);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (1, 2, 1), 2);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (2, 2, 1), 4);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (3, 2, 1), 4);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (4, 2, 1), 6);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (5, 2, 1), 6);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (6, 2, 1), 8);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (7, 2, 1), 8);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (8, 2, 1), 10);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (9, 2, 1), 10);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (10, 2, 1), 12);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFD, 2, 1), 0xFFFE);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFE, 2, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFF, 2, 1), 0);

   /* Cycle length 62.5 us, transmission period 250 us */
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0, 2, 4), 8);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (7, 2, 4), 8);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (8, 2, 4), 16);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (9, 2, 4), 16);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFE, 2, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFF, 2, 4), 0);

   /* Cycle length 1 ms, transmission period 4 ms */
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (1, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (2, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (3, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (127, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (128, 32, 4), 256);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (129, 32, 4), 256);

   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0x00, 32, 4), 0x080);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0x7F, 32, 4), 0x080);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0x80, 32, 4), 0x100);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0x81, 32, 4), 0x100);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFE, 32, 4), 0x100);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFF, 32, 4), 0x100);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0x100, 32, 4), 0x180);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0x101, 32, 4), 0x180);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFEFF, 32, 4), 0xFF00);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFF00, 32, 4), 0xFF80);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFF01, 32, 4), 0xFF80);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFF7E, 32, 4), 0xFF80);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFF7F, 32, 4), 0xFF80);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFF80, 32, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFF81, 32, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFE, 32, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFF, 32, 4), 0);

   /* Cycle length 4 ms, transmission period 2.048 s */
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0, 128, 512), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (1, 128, 512), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (2, 128, 512), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFE, 128, 512), 0);
   EXPECT_EQ (pf_ppm_calculate_next_cyclecounter (0xFFFF, 128, 512), 0);
}
