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

class AlarmTest : public PnetIntegrationTest
{
};

class AlarmUnitTest : public PnetUnitTest
{
};

TEST_F (AlarmUnitTest, AlarmCheckAddDiagItemToSummary)
{
   pnet_alarm_spec_t alarm_specifier;
   uint32_t maint_status = 0;
   pf_diag_item_t diag_item;
   pf_subslot_t subslot;
   pf_ar_t ar;

   /* Prepare default input data */
   memset (&ar, 0, sizeof (ar));
   memset (&subslot, 0, sizeof (subslot));
   subslot.ownsm_state = PF_OWNSM_STATE_IOC;
   subslot.owner = &ar;

   memset (&diag_item, 0, sizeof (diag_item));
   diag_item.usi = PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;
   PF_DIAG_CH_PROP_SPEC_SET (
      diag_item.fmt.std.ch_properties,
      PF_DIAG_CH_PROP_SPEC_APPEARS);
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Happy case */
   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, 0UL);

   /* Other AR */
   subslot.owner = NULL;

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, 0UL);
   subslot.owner = &ar;

   /* Manufacturer specific (in USI format) */
   diag_item.usi = TEST_DIAG_USI_CUSTOM;

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_FALSE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, 0UL);
   diag_item.usi = PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;

   /* Disappearing diagnosis */
   PF_DIAG_CH_PROP_SPEC_SET (
      diag_item.fmt.std.ch_properties,
      PF_DIAG_CH_PROP_SPEC_DISAPPEARS);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_FALSE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, 0UL);
   PF_DIAG_CH_PROP_SPEC_SET (
      diag_item.fmt.std.ch_properties,
      PF_DIAG_CH_PROP_SPEC_APPEARS);

   /* Maintenance required */
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_REQUIRED);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, BIT (0));
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Maintenance required, via qualified channel diagnosis (qual 7..16) */
   diag_item.usi = PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS;
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_QUALIFIED);
   diag_item.fmt.std.qual_ch_qualifier = BIT (7);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, (BIT (7) | BIT (0)));

   diag_item.fmt.std.qual_ch_qualifier = BIT (16);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, (BIT (16) | BIT (0)));
   diag_item.usi = PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;
   diag_item.fmt.std.qual_ch_qualifier = 0;
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Maintenance demanded */
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_DEMANDED);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, BIT (1));
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Maintenance demanded, via qualified channel diagnosis (qual 17..26) */
   diag_item.usi = PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS;
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_QUALIFIED);
   diag_item.fmt.std.qual_ch_qualifier = BIT (17);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, (BIT (17) | BIT (1)));

   diag_item.fmt.std.qual_ch_qualifier = BIT (26);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, (BIT (26) | BIT (1)));
   diag_item.usi = PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;
   diag_item.fmt.std.qual_ch_qualifier = 0;
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Fault, via qualified channel diagnosis (qual 27..31) */
   diag_item.usi = PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS;
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_QUALIFIED);
   diag_item.fmt.std.qual_ch_qualifier = BIT (27);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, BIT (27));

   diag_item.fmt.std.qual_ch_qualifier = BIT (31);

   memset (&alarm_specifier, 0, sizeof (alarm_specifier));
   maint_status = 0;
   pf_alarm_add_diag_item_to_summary (
      &ar,
      &subslot,
      &diag_item,
      &alarm_specifier,
      &maint_status);
   EXPECT_TRUE ((bool)alarm_specifier.channel_diagnosis);
   EXPECT_FALSE ((bool)alarm_specifier.manufacturer_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.submodule_diagnosis);
   EXPECT_TRUE ((bool)alarm_specifier.ar_diagnosis);
   EXPECT_EQ (maint_status, BIT (31));
   diag_item.usi = PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;
   diag_item.fmt.std.qual_ch_qualifier = 0;
   PF_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);
}

TEST_F (AlarmUnitTest, AlarmCheckSendQueueHandling)
{
   pf_alarm_send_queue_t queue;
   pf_alarm_data_t post_message;
   pf_alarm_data_t fetch_message;
   int ix = 0;
   int err = 0;
   const uint16_t SEQUENCE_START_NUMBER = 100;

   memset (&post_message, 0, sizeof (post_message));
   memset (&fetch_message, 0, sizeof (fetch_message));
   memset (&queue, 0, sizeof (queue));

   /* Set up queue */
   EXPECT_FALSE (pf_alarm_queue_is_available(&queue.accountant));
   pf_alarm_queue_mutex_create (&queue.accountant);
   pf_alarm_queue_mutex_create (&queue.accountant); /* No-op */
   EXPECT_TRUE (pf_alarm_queue_is_available(&queue.accountant));
   pf_alarm_send_queue_reset (&queue);
   EXPECT_EQ (queue.accountant.count, 0); /* Do not bother with mutex
                                            (this test runs in single thread) */

   /* Fill the queue */
   for (ix = 0; ix < PNET_MAX_ALARMS; ix++)
   {
      post_message.sequence_number = SEQUENCE_START_NUMBER + ix;
      err = pf_alarm_send_queue_post (&queue, &post_message);
      EXPECT_EQ (err, 0);
   }
   EXPECT_EQ (queue.accountant.count, PNET_MAX_ALARMS);

   /* Add to full queue */
   post_message.sequence_number += 1;
   err = pf_alarm_send_queue_post (&queue, &post_message);
   EXPECT_EQ (err, -1);
   EXPECT_EQ (queue.accountant.count, PNET_MAX_ALARMS);

   /* Fetch from the queue */
   for (ix = 0; ix < PNET_MAX_ALARMS; ix++)
   {
      err = pf_alarm_send_queue_fetch (&queue, &fetch_message);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (fetch_message.sequence_number, SEQUENCE_START_NUMBER + ix);
   }
   EXPECT_EQ (queue.accountant.count, 0);

   /* Fetch from empty queue */
   err = pf_alarm_send_queue_fetch (&queue, &fetch_message);
   EXPECT_EQ (err, -1);
   EXPECT_EQ (queue.accountant.count, 0);

   /* Reset the queue */
   err = pf_alarm_send_queue_post (&queue, &post_message);
   EXPECT_EQ (err, 0);
   EXPECT_EQ (queue.accountant.count, 1);

   pf_alarm_send_queue_reset (&queue);
   EXPECT_EQ (queue.accountant.count, 0);

   /* Wrap read_index and write_index, by adding and fetching a lot */
   for (ix = 0; ix < PNET_MAX_ALARMS * 5; ix++)
   {
      post_message.sequence_number = SEQUENCE_START_NUMBER + ix;
      err = pf_alarm_send_queue_post (&queue, &post_message);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (queue.accountant.count, 1);

      err = pf_alarm_send_queue_fetch (&queue, &fetch_message);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (queue.accountant.count, 0);
      EXPECT_EQ (fetch_message.sequence_number, SEQUENCE_START_NUMBER + ix);
   }

   /* Close down queue */
   pf_alarm_send_queue_reset (&queue);
   EXPECT_EQ (queue.accountant.count, 0);
   EXPECT_TRUE (pf_alarm_queue_is_available(&queue.accountant));
   pf_alarm_queue_mutex_destroy (&queue.accountant);
   pf_alarm_queue_mutex_destroy (&queue.accountant); /* Should be safe */
   EXPECT_FALSE (pf_alarm_queue_is_available(&queue.accountant));

   /* Post to closed queue */
   post_message.sequence_number = SEQUENCE_START_NUMBER;
   err = pf_alarm_send_queue_post (&queue, &post_message);
   EXPECT_EQ (err, -1);

   /* Fetch from closed queue */
   err = pf_alarm_send_queue_fetch (&queue, &fetch_message);
   EXPECT_EQ (err, -1);
}

TEST_F (AlarmUnitTest, AlarmCheckReceiveQueueHandling)
{
   pf_alarm_receive_queue_t queue;
   pf_apmr_msg_t post_frame;
   pf_apmr_msg_t fetch_frame;
   int ix = 0;
   int err = 0;
   const uint16_t POSITION_START_NUMBER = 100;
   const uintptr_t POINTER_START_NUMBER = 1000;

   memset (&post_frame, 0, sizeof (post_frame));
   memset (&fetch_frame, 0, sizeof (fetch_frame));
   memset (&queue, 0, sizeof (queue));

   /* Set up queue */
   EXPECT_FALSE (pf_alarm_queue_is_available(&queue.accountant));
   pf_alarm_queue_mutex_create (&queue.accountant);
   pf_alarm_queue_mutex_create (&queue.accountant); /* No-op */
   EXPECT_TRUE (pf_alarm_queue_is_available(&queue.accountant));
   pf_alarm_receive_queue_reset (&queue);
   EXPECT_EQ (queue.accountant.count, 0); /* Do not bother with mutex
                                            (this test runs in single thread) */

   /* Fill the queue */
   for (ix = 0; ix < PNET_MAX_ALARMS; ix++)
   {
      post_frame.frame_id_pos = POSITION_START_NUMBER + ix;
      post_frame.p_buf = (pnal_buf_t *)(POINTER_START_NUMBER + ix);
      err = pf_alarm_receive_queue_post (&queue, &post_frame);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (queue.accountant.count, ix + 1);
   }
   EXPECT_EQ (queue.accountant.count, PNET_MAX_ALARMS);

   /* Add to full queue */
   post_frame.frame_id_pos += 1;
   post_frame.p_buf -= 1;
   err = pf_alarm_receive_queue_post (&queue, &post_frame);
   EXPECT_EQ (err, -1);
   EXPECT_EQ (queue.accountant.count, PNET_MAX_ALARMS);

   /* Fetch from the queue */
   for (ix = 0; ix < PNET_MAX_ALARMS; ix++)
   {
      err = pf_alarm_receive_queue_fetch (&queue, &fetch_frame);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (queue.accountant.count, PNET_MAX_ALARMS - ix - 1);
      EXPECT_EQ (fetch_frame.frame_id_pos, POSITION_START_NUMBER + ix);
      EXPECT_EQ ((uintptr_t)fetch_frame.p_buf, POINTER_START_NUMBER + ix);
   }
   EXPECT_EQ (queue.accountant.count, 0);

   /* Fetch from empty queue */
   err = pf_alarm_receive_queue_fetch (&queue, &fetch_frame);
   EXPECT_EQ (err, -1);
   EXPECT_EQ (queue.accountant.count, 0);

   /* Reset the queue (will free allocated buffers) */
   post_frame.frame_id_pos = 42;
   post_frame.p_buf = pnal_buf_alloc (PNAL_BUF_MAX_SIZE);
   err = pf_alarm_receive_queue_post (&queue, &post_frame);
   EXPECT_EQ (err, 0);
   EXPECT_EQ (queue.accountant.count, 1);

   pf_alarm_receive_queue_reset (&queue);
   EXPECT_EQ (queue.accountant.count, 0);

   /* Wrap read_index and write_index, by adding and fetching a lot */
   for (ix = 0; ix < PNET_MAX_ALARMS * 5; ix++)
   {
      post_frame.frame_id_pos = POSITION_START_NUMBER + ix;
      post_frame.p_buf = (pnal_buf_t *)(POINTER_START_NUMBER + ix);
      err = pf_alarm_receive_queue_post (&queue, &post_frame);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (queue.accountant.count, 1);

      err = pf_alarm_receive_queue_fetch (&queue, &fetch_frame);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (queue.accountant.count, 0);
      EXPECT_EQ (fetch_frame.frame_id_pos, POSITION_START_NUMBER + ix);
      EXPECT_EQ ((uintptr_t)fetch_frame.p_buf, POINTER_START_NUMBER + ix);
   }

   /* Close down queue */
   pf_alarm_receive_queue_reset (&queue);
   EXPECT_EQ (queue.accountant.count, 0);
   EXPECT_TRUE (pf_alarm_queue_is_available(&queue.accountant));
   pf_alarm_queue_mutex_destroy (&queue.accountant);
   pf_alarm_queue_mutex_destroy (&queue.accountant); /* Should be safe */
   EXPECT_FALSE (pf_alarm_queue_is_available(&queue.accountant));

   /* Post to closed queue */
   post_frame.frame_id_pos = POSITION_START_NUMBER;
   post_frame.p_buf = (pnal_buf_t *)(POINTER_START_NUMBER);
   err = pf_alarm_receive_queue_post (&queue, &post_frame);
   EXPECT_EQ (err, -1);

   /* Fetch from closed queue */
   err = pf_alarm_receive_queue_fetch (&queue, &fetch_frame);
   EXPECT_EQ (err, -1);
}
