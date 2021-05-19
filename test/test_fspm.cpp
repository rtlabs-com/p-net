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

class FspmTest : public PnetIntegrationTest
{
};

class FspmUnitTest : public PnetUnitTest
{
};

TEST_F (FspmUnitTest, FspmCheckValidateConfiguration)
{
   pnet_cfg_t cfg;

   /* Known good values */
   memset (&cfg, 0, sizeof (pnet_cfg_t));
   cfg.tick_us = 1000;
   cfg.min_device_interval = 1;
   cfg.im_0_data.im_supported = 0;
   cfg.if_cfg.main_netif_name = "eth0";
   cfg.num_physical_ports = PNET_MAX_PHYSICAL_PORTS;

   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   /* Check pointer validity */
   EXPECT_EQ (pf_fspm_validate_configuration (NULL), -1);

   /* Check number of ports */
   cfg.num_physical_ports = PNET_MAX_PHYSICAL_PORTS + 1;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.num_physical_ports = 0;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.num_physical_ports = 1;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.num_physical_ports = PNET_MAX_PHYSICAL_PORTS;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   /* Check minimum stack update interval */
   cfg.min_device_interval = 0;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.min_device_interval = 0x1000;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.min_device_interval = 0x1001;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.min_device_interval = 0xFFFF;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);
   cfg.min_device_interval = 1;

   /* Check supported I&M fields */
   cfg.im_0_data.im_supported = PNET_SUPPORTED_IM1;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.im_0_data.im_supported = PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.im_0_data.im_supported = PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2 |
                                PNET_SUPPORTED_IM3 | PNET_SUPPORTED_IM4;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.im_0_data.im_supported = PNET_SUPPORTED_IM4;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.im_0_data.im_supported = 1;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.im_0_data.im_supported = 0x0020;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.im_0_data.im_supported = 0x8000;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.im_0_data.im_supported = 0xFFFF;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);
   cfg.im_0_data.im_supported = 0;
}

TEST_F (FspmTest, FspmGetImLocation)
{
   const char expected[PNET_LOCATION_MAX_SIZE] = "                      ";
   char actual[PNET_LOCATION_MAX_SIZE];

   memset (actual, 0xff, sizeof (actual));

   /* Note that as loading from file failed in pf_fspm_init(), location was set
    * to its default value, which is all spaces.
    */
   pf_fspm_get_im_location (net, actual);
   EXPECT_STREQ (actual, expected);
   EXPECT_EQ (strlen (actual), 22u);
}

TEST_F (FspmTest, FspmSaveImLocation)
{
   const char written[PNET_LOCATION_MAX_SIZE] = "location of device";
   const char expected[PNET_LOCATION_MAX_SIZE] = "location of device    ";
   char actual[PNET_LOCATION_MAX_SIZE];

   memset (actual, 0xff, sizeof (actual));

   /* Note that extra spaces are added at the end */
   pf_fspm_save_im_location (net, written);

   /* The file save operation below is performed by the background worker
    *  during normal execution of the stack.
    */
   pf_fspm_save_im (net);

   pf_fspm_get_im_location (net, actual);
   EXPECT_STREQ (actual, expected);
   EXPECT_EQ (strlen (actual), 22u);
   EXPECT_STREQ (mock_file_data.filename, PF_FILENAME_IM);
}

TEST_F (FspmTest, FspmSaveImLocationShouldAddTermination)
{
   char not_terminated[PNET_LOCATION_MAX_SIZE];
   char actual[PNET_LOCATION_MAX_SIZE];

   memset (actual, 0xff, sizeof (actual));
   memset (not_terminated, 'n', sizeof (not_terminated));

   pf_fspm_save_im_location (net, not_terminated);

   /* The file save operation below is performed by the background worker
    *  during normal execution of the stack.
    */
   pf_fspm_save_im (net);

   pf_fspm_get_im_location (net, actual);
   EXPECT_EQ (actual[sizeof (actual) - 1], '\0');
   EXPECT_EQ (strlen (actual), 22u);
   EXPECT_STREQ (mock_file_data.filename, PF_FILENAME_IM);
}

TEST_F (FspmTest, FspmSaveImLocationShouldTruncateLargeStrings)
{
   const char large[] = "123456789012345678901234567890";
   const char expected[] = "1234567890123456789012";
   char actual[PNET_LOCATION_MAX_SIZE];

   memset (actual, 0xff, sizeof (actual));

   pf_fspm_save_im_location (net, large);

   /* The file save operation below is performed by the background worker
    *  during normal execution of the stack.
    */
   pf_fspm_save_im (net);

   pf_fspm_get_im_location (net, actual);
   EXPECT_STREQ (actual, expected);
   EXPECT_EQ (strlen (actual), 22u);
   EXPECT_STREQ (mock_file_data.filename, PF_FILENAME_IM);
}
