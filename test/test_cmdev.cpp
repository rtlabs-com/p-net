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
 * @brief Unit tests of features from the CMDEV state machine.
 *
 */


#include "pf_includes.h"

#include <gtest/gtest.h>

#include "mocks.h"
#include "test_util.h"

// Test fixture

class CmdevTest : public ::testing::Test
{
protected:
   virtual void SetUp() {
      memset (&net, 0, sizeof(net));
   };

   pnet_cfg_t net;
};

// Tests

TEST_F (CmdevTest, CmdevCalculateDatadirectionInDescriptor)
{
   pf_data_direction_values_t resulting_direction;
   int ret;

   /* Module with no cyclic data */
   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_NO_IO,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_NO_IO,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_NO_IO,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_NO_IO,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   /* Input module */
   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_INPUT,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_INPUT,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_INPUT,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_INPUT,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   /* Output module */
   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_OUTPUT,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_OUTPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_OUTPUT,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_OUTPUT,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_OUTPUT,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_OUTPUT, resulting_direction);

   /* Input and output module */
   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_IO,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_OUTPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_IO,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_IO,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction(
      PNET_DIR_IO,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_OUTPUT, resulting_direction);
}
