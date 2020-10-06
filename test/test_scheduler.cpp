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

class SchedulerTest : public PnetIntegrationTest
{
};

class SchedulerUnitTest : public PnetUnitTest
{
};

TEST_F (SchedulerUnitTest, SchedulerSanitizeDelayTest)
{
   const uint32_t cycle_len = 1000;
   const uint32_t margin = 10u;
   uint32_t result = 0;
   uint32_t input_delay = 0;

   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = 500;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = 1000;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = 1400;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = 1600;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 1500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 2000, margin);

   input_delay = 2000;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 1500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 2000, margin);

   input_delay = 2400;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 1500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 2000, margin);

   input_delay = 2600;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 2500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 3000, margin);

   input_delay = 3000;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 2500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 3000, margin);

   input_delay = 3400;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 2500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 3000, margin);

   input_delay = 1000000; /* 1 second */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 999500u, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000000, margin);

   input_delay = 10000000; /* 10 seconds */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 9999500u, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 10000000, margin);

   input_delay = 1000000000; /* Unrealisticly far in the future (1000 s) */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = (uint32_t) (-4); /* Next execution has already passed */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);
}
