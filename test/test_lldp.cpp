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


const char * chassis_id_test_sample_1 =
"\x53\x43\x41\x4c\x41\x4e\x43\x45\x20\x58\x2d\x32\x30\x30\x20" \
"\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x36\x47\x4b\x35\x20" \
"\x32\x30\x34\x2d\x30\x42\x41\x30\x30\x2d\x32\x42\x41\x33\x20\x20" \
"\x53\x56\x50\x4c\x36\x31\x34\x37\x39\x32\x30\x20\x20\x20\x20\x20" \
"\x20\x20\x20\x20\x20\x39\x20\x56\x20\x20\x35\x20\x20\x34\x20\x20" \
"\x32";

const char * port_id_test_sample_1 =
"\x70\x6f\x72\x74\x2d\x30\x30\x33\x2e\x61\x62\x63\x64\x65\x66\x67" \
"\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77" \
"\x78\x79\x7a\x2d\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c" \
"\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x31\x32" \
"\x33\x34\x35\x36\x37\x38\x39\x30\x2e\x61\x62\x63\x64\x65\x66\x67" \
"\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77" \
"\x78\x79\x7a\x2d\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c" \
"\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x31\x32" \
"\x2e\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f" \
"\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x2d\x61\x62\x63\x64" \
"\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74" \
"\x75\x76\x77\x78\x79\x7a\x31\x32\x33\x2e\x61\x62\x63\x64\x65\x66" \
"\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76" \
"\x77\x78\x79\x7a\x2d\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b" \
"\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x31" \
"\x32\x33\x34\x35\x36\x37\x38\x39\x30";


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

TEST_F(LldpTest, LldpGenerateAliasName)
{
   char alias[256];
   int err; 

   err =  pf_lldp_generate_alias_name(NULL, "dut", alias , 96);
   EXPECT_EQ(err, -1);
   
   err =  pf_lldp_generate_alias_name("port-001", NULL, alias , 96);
   EXPECT_EQ(err, -1);

   err =  pf_lldp_generate_alias_name("port-001", "dut", NULL , 96);
   EXPECT_EQ(err, -1);

   /* Test legacy PN v2.2 
    * PortId string does not include a "." 
    * Alias = PortId.StationName 
    */
   err =  pf_lldp_generate_alias_name("port-001", "dut", alias, 13);
   EXPECT_EQ(err, 0);
   EXPECT_EQ(strcmp(alias, "port-001.dut"), 0);
   memset(alias, 0, sizeof(alias));

   err =  pf_lldp_generate_alias_name("port-001", "dut", alias, 12);
   EXPECT_EQ(err, -1);

   /* Test PN v2.3+ 
    * PortId string does include a "."
    * Alias = PortId
    */
   err =  pf_lldp_generate_alias_name("port-001.dut", "tester", alias , 13);
   EXPECT_EQ(err, 0);
   EXPECT_EQ(strcmp(alias, "port-001.dut"), 0);
   memset(alias, 0, sizeof(alias));

   err =  pf_lldp_generate_alias_name("port-001.dut", "tester", alias , 12);
   EXPECT_EQ(err, -1);

   /* Test data from rt-tester v2.3 style */
   err =  pf_lldp_generate_alias_name(port_id_test_sample_1, chassis_id_test_sample_1, alias , 256);
   EXPECT_EQ(err, 0);
   EXPECT_EQ(strcmp(alias, port_id_test_sample_1), 0);
   memset(alias, 0, sizeof(alias));







}