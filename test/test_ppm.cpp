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


class PpmTest : public PnetIntegrationTest {};
class PpmUnitTest : public PnetUnitTest {};


TEST_F (PpmTest, PpmRunTest)
{
}

TEST_F (PpmUnitTest, PpmCalculateCompensatedDelayTest)
{
   uint32_t                result = 0;

   result = pf_ppm_calculate_compensated_delay(0, 500);
   ASSERT_LT(result, 350u);
   result = pf_ppm_calculate_compensated_delay(250, 500);
   ASSERT_LT(result, 350u);
   result = pf_ppm_calculate_compensated_delay(500, 500);
   ASSERT_GT(result, 50u);
   ASSERT_LT(result, 350u);
   result = pf_ppm_calculate_compensated_delay(700, 500);
   ASSERT_GT(result, 50u);
   ASSERT_LT(result, 350u);
   result = pf_ppm_calculate_compensated_delay(800, 500);
   ASSERT_GT(result, 550u);
   ASSERT_LT(result, 850u);
   result = pf_ppm_calculate_compensated_delay(1000, 500);
   ASSERT_GT(result, 550u);
   ASSERT_LT(result, 850u);
   result = pf_ppm_calculate_compensated_delay(1200, 500);
   ASSERT_GT(result, 550u);
   ASSERT_LT(result, 850u);
   result = pf_ppm_calculate_compensated_delay(1300, 500);
   ASSERT_GT(result, 1050u);
   ASSERT_LT(result, 1350u);
   result = pf_ppm_calculate_compensated_delay(1500, 500);
   ASSERT_GT(result, 1050u);
   ASSERT_LT(result, 1350u);
   result = pf_ppm_calculate_compensated_delay(1700, 500);
   ASSERT_GT(result, 1050u);
   ASSERT_LT(result, 1350u);

   result = pf_ppm_calculate_compensated_delay(0, 1000);
   ASSERT_LT(result, 700u);
   result = pf_ppm_calculate_compensated_delay(500, 1000);
   ASSERT_LT(result, 700u);
   result = pf_ppm_calculate_compensated_delay(1000, 1000);
   ASSERT_GT(result, 100u);
   ASSERT_LT(result, 700u);
   result = pf_ppm_calculate_compensated_delay(1400, 1000);
   ASSERT_GT(result, 100u);
   ASSERT_LT(result, 700u);
   result = pf_ppm_calculate_compensated_delay(1600, 1000);
   ASSERT_GT(result, 1100u);
   ASSERT_LT(result, 1700u);
   result = pf_ppm_calculate_compensated_delay(2000, 1000);
   ASSERT_GT(result, 1100u);
   ASSERT_LT(result, 1700u);
   result = pf_ppm_calculate_compensated_delay(2400, 1000);
   ASSERT_GT(result, 1100u);
   ASSERT_LT(result, 1700u);
   result = pf_ppm_calculate_compensated_delay(2600, 1000);
   ASSERT_GT(result, 2100u);
   ASSERT_LT(result, 2700u);
   result = pf_ppm_calculate_compensated_delay(3000, 1000);
   ASSERT_GT(result, 2100u);
   ASSERT_LT(result, 2700u);
   result = pf_ppm_calculate_compensated_delay(3400, 1000);
   ASSERT_GT(result, 2100u);
   ASSERT_LT(result, 2700u);

   result = pf_ppm_calculate_compensated_delay(0, 2000);
   ASSERT_LT(result, 1400u);
   result = pf_ppm_calculate_compensated_delay(1000, 2000);
   ASSERT_LT(result, 1400u);
   result = pf_ppm_calculate_compensated_delay(2000, 2000);
   ASSERT_GT(result, 100u);
   ASSERT_LT(result, 1400u);
   result = pf_ppm_calculate_compensated_delay(2900, 2000);
   ASSERT_GT(result, 100u);
   ASSERT_LT(result, 1400u);
   result = pf_ppm_calculate_compensated_delay(3100, 2000);
   ASSERT_GT(result, 2200u);
   ASSERT_LT(result, 3400u);
   result = pf_ppm_calculate_compensated_delay(4000, 2000);
   ASSERT_GT(result, 2200u);
   ASSERT_LT(result, 3400u);
   result = pf_ppm_calculate_compensated_delay(4900, 2000);
   ASSERT_GT(result, 2200u);
   ASSERT_LT(result, 3400u);
   result = pf_ppm_calculate_compensated_delay(5100, 2000);
   ASSERT_GT(result, 4200u);
   ASSERT_LT(result, 5400u);
   result = pf_ppm_calculate_compensated_delay(6000, 2000);
   ASSERT_GT(result, 4200u);
   ASSERT_LT(result, 5400u);
   result = pf_ppm_calculate_compensated_delay(6900, 2000);
   ASSERT_GT(result, 4200u);
   ASSERT_LT(result, 5400u);
}
