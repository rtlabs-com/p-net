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
