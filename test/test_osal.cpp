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
 * @brief Unit tests of features from osal; timer, mbox, sem
 *
 */

#include "osal.h"
#include <gtest/gtest.h>

static int expired_calls;
static void * expired_arg;
static void expired (os_timer_t * timer, void * arg)
{
   expired_calls++;
   expired_arg = arg;
}

class Osal : public ::testing::Test
{
protected:
   virtual void SetUp() {
      expired_calls = 0;
   }
};

TEST (Osal, SemShouldTimeoutWhenCountIsZero)
{
   os_sem_t * sem = os_sem_create (2);
   int tmo;

   // Count 2
   tmo = os_sem_wait (sem, 100);
   EXPECT_EQ (0, tmo);

   // Count 1
   tmo = os_sem_wait (sem, 100);
   EXPECT_EQ (0, tmo);

   // Count 0 - timeout
   tmo = os_sem_wait (sem, 100);
   EXPECT_EQ (1, tmo);

   // Count 0 - timeout
   tmo = os_sem_wait (sem, 100);
   EXPECT_EQ (1, tmo);

   // Increase count
   os_sem_signal (sem);

   // Count 1
   tmo = os_sem_wait (sem, 100);
   EXPECT_EQ (0, tmo);

   // Count 0 - timeout
   tmo = os_sem_wait (sem, 100);
   EXPECT_EQ (1, tmo);

   os_sem_destroy (sem);
}

TEST (Osal, EventShouldNotTimeout)
{
   os_event_t * event = os_event_create();
   uint32_t value = 99;
   int tmo;

   os_event_set (event, 1);
   tmo = os_event_wait (event, 1, &value, OS_WAIT_FOREVER);
   EXPECT_EQ (0, tmo);
   EXPECT_EQ (1u, value);

   os_event_destroy (event);
}

TEST (Osal, EventShouldTimeout)
{
   os_event_t * event = os_event_create();
   uint32_t value = 99;
   int tmo;

   os_event_set (event, 2);
   tmo = os_event_wait (event, 1, &value, 100);
   EXPECT_GT (tmo, 0);
   EXPECT_EQ (0u, value);

   os_event_destroy (event);
}

TEST (Osal, MboxShouldNotTimeout)
{
   os_mbox_t * mbox = os_mbox_create(2);
   void * msg;
   int tmo;

   os_mbox_post (mbox, (void *)1, OS_WAIT_FOREVER);
   tmo = os_mbox_fetch (mbox, &msg, OS_WAIT_FOREVER);

   EXPECT_EQ (0, tmo);
   EXPECT_EQ (1, (long)msg);

   os_mbox_destroy (mbox);
}

TEST (Osal, FetchFromEmptyMboxShouldTimeout)
{
   os_mbox_t * mbox = os_mbox_create(2);
   void * msg;
   int tmo;

   tmo = os_mbox_fetch (mbox, &msg, 100);
   EXPECT_EQ (1, tmo);

   os_mbox_destroy (mbox);
}

TEST (Osal, PostToFullMBoxShouldTimeout)
{
   os_mbox_t * mbox = os_mbox_create(2);
   int tmo;

   os_mbox_post (mbox, (void *)1, 100);
   os_mbox_post (mbox, (void *)2, 100);
   tmo = os_mbox_post (mbox, (void *)3, 100);
   EXPECT_EQ (1, tmo);

   os_mbox_destroy (mbox);
}

TEST (Osal, CyclicTimer)
{
   int t0, t1;
   os_timer_t * timer;

   timer = os_timer_create (10 * 1000, expired, (void *)0x42, false);
   os_timer_start (timer);

   t0 = os_get_current_time_us();
   os_usleep (350 * 1000);
   t1 = os_get_current_time_us();
   os_timer_stop (timer);

   EXPECT_NEAR (350 * 1000, t1 - t0, 1000);
   EXPECT_NEAR (35, expired_calls, 2);

   EXPECT_EQ ((void *)0x42, expired_arg);

   // Check that timer is stopped
   // FIXME: on linux, timer_settime does not disarm immediately
   expired_calls = 0;
   os_usleep (100 * 1000);
   EXPECT_NEAR (0, expired_calls, 1);

   os_timer_destroy (timer);
}

TEST (Osal, OneshotTimer)
{
   os_timer_t * timer;

   timer = os_timer_create (20 * 1000, expired, (void *)0x42, true);

   os_timer_start (timer);
   os_usleep (100 * 1000);

   // FIXME: on linux, expired_calls may be 2 because of previous test
   EXPECT_NEAR (1, expired_calls, 1);
   EXPECT_EQ ((void *)0x42, expired_arg);

   os_timer_destroy (timer);
}
