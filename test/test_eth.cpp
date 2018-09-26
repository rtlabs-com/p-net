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

#include "pf_includes.h"

#include <gtest/gtest.h>

#include "mocks.h"
#include "test_util.h"

// Test fixture

class EthTest : public ::testing::Test
{
protected:
   virtual void SetUp() {
      memset (&net, 0, sizeof(net));
   };

   pnet_cfg_t net;
};

// Tests

TEST_F (EthTest, EthRunTest)
{
}
