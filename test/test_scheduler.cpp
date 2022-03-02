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

void test_scheduler_callback_a (pnet_t * net, void * arg, uint32_t current_time)
{
   app_data_for_testing_t * appdata = (app_data_for_testing_t *)arg;

   appdata->call_counters.scheduler_callback_a_calls += 1;
   pf_scheduler_reset_handle (&appdata->scheduler_handle_a);
}

void test_scheduler_callback_b (pnet_t * net, void * arg, uint32_t current_time)
{
   app_data_for_testing_t * appdata = (app_data_for_testing_t *)arg;

   appdata->call_counters.scheduler_callback_b_calls += 1;
   pf_scheduler_reset_handle (&appdata->scheduler_handle_b);
}

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

   input_delay = (uint32_t)(-4); /* Next execution has already passed */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);
}

TEST_F (SchedulerTest, SchedulerAddRemove)
{
   int ret;
   pf_scheduler_handle_t * p_a = &appdata.scheduler_handle_a;
   pf_scheduler_handle_t * p_b = &appdata.scheduler_handle_b;
   const char scheduler_name_a[] = "testhandle_a";
   const char scheduler_name_b[] = "testhandle_b";
   const char * resulting_name = NULL;
   uint32_t value;
   bool is_scheduled;

   pf_scheduler_init (net, TEST_TICK_INTERVAL_US);
   run_stack (TEST_SCHEDULER_RUNTIME);
   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 0);

   /* Initialize and verify handle */
   pf_scheduler_init_handle (p_a, scheduler_name_a);
   pf_scheduler_init_handle (p_b, scheduler_name_b);

   resulting_name = pf_scheduler_get_name (p_a);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_a), 0);
   resulting_name = pf_scheduler_get_name (p_b);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_b), 0);

   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */

   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   /* Reset non-running handle */
   pf_scheduler_reset_handle (p_a);

   resulting_name = pf_scheduler_get_name (p_a);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_a), 0);

   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */

   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   /* Schedule callback A */
   ret = pf_scheduler_add (
      net,
      TEST_SCHEDULER_CALLBACK_DELAY,
      test_scheduler_callback_a,
      &appdata,
      p_a);
   EXPECT_EQ (ret, 0);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 0);
   resulting_name = pf_scheduler_get_name (p_a);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_a), 0);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 1UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 0);
   resulting_name = pf_scheduler_get_name (p_b);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_b), 0);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   /* Verify that it not is triggered */
   run_stack (TEST_TICK_INTERVAL_US);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 0);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 1UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 0);
   resulting_name = pf_scheduler_get_name (p_b);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_b), 0);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   /* Verify that callback A is triggered */
   run_stack (TEST_SCHEDULER_RUNTIME);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 1);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 0);
   resulting_name = pf_scheduler_get_name (p_b);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_b), 0);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   /* Schedule both callbacks */
   ret = pf_scheduler_add (
      net,
      TEST_SCHEDULER_CALLBACK_DELAY,
      test_scheduler_callback_a,
      &appdata,
      p_a);
   EXPECT_EQ (ret, 0);
   ret = pf_scheduler_add (
      net,
      TEST_SCHEDULER_RUNTIME + TEST_SCHEDULER_CALLBACK_DELAY,
      test_scheduler_callback_b,
      &appdata,
      p_b);
   EXPECT_EQ (ret, 0);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 1);
   resulting_name = pf_scheduler_get_name (p_a);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_a), 0);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 1UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 0);
   resulting_name = pf_scheduler_get_name (p_b);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_b), 0);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_TRUE (is_scheduled);

   /* Verify that they not are triggered */
   run_stack (TEST_TICK_INTERVAL_US);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 1);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 1UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 0);
   resulting_name = pf_scheduler_get_name (p_b);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_b), 0);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_TRUE (is_scheduled);

   /* Verify that callback A (but not B) is triggered */
   run_stack (TEST_SCHEDULER_RUNTIME);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 0);
   resulting_name = pf_scheduler_get_name (p_b);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_b), 0);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_TRUE (is_scheduled);

   /* Verify that both callbacks have been triggered */
   run_stack (TEST_SCHEDULER_RUNTIME);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   resulting_name = pf_scheduler_get_name (p_b);
   EXPECT_EQ (strcmp (resulting_name, scheduler_name_b), 0);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   /* Remove non-scheduled event */
   run_stack (TEST_SCHEDULER_RUNTIME);

   pf_scheduler_remove_if_running (net, p_a);
   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   pf_scheduler_remove (net, p_a); /* Will log error message */
   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   /* Remove scheduled event */
   run_stack (TEST_SCHEDULER_RUNTIME);

   ret = pf_scheduler_add (
      net,
      TEST_SCHEDULER_CALLBACK_DELAY,
      test_scheduler_callback_a,
      &appdata,
      p_a);
   EXPECT_EQ (ret, 0);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   run_stack (TEST_TICK_INTERVAL_US);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   pf_scheduler_remove (net, p_a);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   run_stack (TEST_SCHEDULER_RUNTIME);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   /* Remove scheduled event, using utility function */
   run_stack (TEST_SCHEDULER_RUNTIME);

   ret = pf_scheduler_add (
      net,
      TEST_SCHEDULER_CALLBACK_DELAY,
      test_scheduler_callback_a,
      &appdata,
      p_a);
   EXPECT_EQ (ret, 0);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 2UL); /* Implementation detail */ // TODO
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   run_stack (TEST_TICK_INTERVAL_US);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   pf_scheduler_remove_if_running (net, p_a);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   run_stack (TEST_SCHEDULER_RUNTIME);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   value = pf_scheduler_get_value (p_b);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   /* Schedule and restart */
   ret = pf_scheduler_add (
      net,
      TEST_SCHEDULER_CALLBACK_DELAY,
      test_scheduler_callback_a,
      &appdata,
      p_a);
   EXPECT_EQ (ret, 0);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   run_stack (TEST_TICK_INTERVAL_US);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   ret = pf_scheduler_restart (
      net,
      TEST_SCHEDULER_RUNTIME + TEST_SCHEDULER_CALLBACK_DELAY,
      test_scheduler_callback_a,
      &appdata,
      p_a);

   run_stack (TEST_SCHEDULER_RUNTIME);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 2);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, 2UL); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_TRUE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);

   run_stack (TEST_SCHEDULER_RUNTIME);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_a_calls, 3);
   value = pf_scheduler_get_value (p_a);
   EXPECT_EQ (value, UINT32_MAX); /* Implementation detail */
   is_scheduled = pf_scheduler_is_running (p_a);
   EXPECT_FALSE (is_scheduled);

   EXPECT_EQ (appdata.call_counters.scheduler_callback_b_calls, 1);
   is_scheduled = pf_scheduler_is_running (p_b);
   EXPECT_FALSE (is_scheduled);
}
