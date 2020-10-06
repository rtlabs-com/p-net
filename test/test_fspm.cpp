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

class FspmUnitTest : public PnetUnitTest
{
};

TEST_F (FspmUnitTest, FspmCheckValidateConfiguration)
{
   pnet_cfg_t cfg;

   /* Known good values */
   memset (&cfg, 0, sizeof (pnet_cfg_t));
   cfg.min_device_interval = 1;
   cfg.im_0_data.im_supported = 0;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   /* Check pointer validity */
   EXPECT_EQ (pf_fspm_validate_configuration (NULL), -1);

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
