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
 * @brief Integration test that a LLDP frame is sent at init.
 *
 */

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>


class LldpTest : public PnetIntegrationTestBase
{
protected:

   virtual void SetUp() override
   {
      mock_init();
      cfg_init();
      appdata_init();
      available_modules_and_submodules_init();

      callcounter_reset();

      pnet_init_only (net, TEST_INTERFACE_NAME, TICK_INTERVAL_US, &pnet_default_cfg);

      /* Do not clear mock or callcounters here - we need to verify send at init from LLDP */
   };
};


TEST_F(LldpTest, LldpInitTest)
{
   /* Verify that LLDP has sent a frame at init */
   EXPECT_EQ(appdata.call_counters.state_calls, 0);
   EXPECT_EQ(appdata.call_counters.connect_calls, 0);
   EXPECT_EQ(appdata.call_counters.release_calls, 0);
   EXPECT_EQ(appdata.call_counters.dcontrol_calls, 0);
   EXPECT_EQ(appdata.call_counters.ccontrol_calls, 0);
   EXPECT_EQ(appdata.call_counters.read_calls, 0);
   EXPECT_EQ(appdata.call_counters.write_calls, 0);
   EXPECT_EQ(mock_os_data.eth_send_count, 1);
}
