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

#include <gtest/gtest.h>
#include "osal.h"

OS_MAIN (int argc, char * argv[])
{
   if (argc > 0)
      ::testing::InitGoogleTest (&argc, argv);
   else
      ::testing::InitGoogleTest();

   int result = RUN_ALL_TESTS();
   return result;
}
