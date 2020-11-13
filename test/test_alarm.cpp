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
   subslot.p_ar = &ar;

   memset (&diag_item, 0, sizeof (diag_item));
   diag_item.usi = PF_USI_EXTENDED_CHANNEL_DIAGNOSIS;
   PNET_DIAG_CH_PROP_SPEC_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_SPEC_APPEARS);
   PNET_DIAG_CH_PROP_MAINT_SET (
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
   subslot.p_ar = NULL;

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
   subslot.p_ar = &ar;

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
   PNET_DIAG_CH_PROP_SPEC_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_SPEC_DISAPPEARS);

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
   PNET_DIAG_CH_PROP_SPEC_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_SPEC_APPEARS);

   /* Maintenance required */
   PNET_DIAG_CH_PROP_MAINT_SET (
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
   PNET_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Maintenance required, via qualified channel diagnosis (qual 7..16) */
   diag_item.usi = PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS;
   PNET_DIAG_CH_PROP_MAINT_SET (
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
   PNET_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Maintenance demanded */
   PNET_DIAG_CH_PROP_MAINT_SET (
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
   PNET_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Maintenance demanded, via qualified channel diagnosis (qual 17..26) */
   diag_item.usi = PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS;
   PNET_DIAG_CH_PROP_MAINT_SET (
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
   PNET_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);

   /* Fault, via qualified channel diagnosis (qual 27..31) */
   diag_item.usi = PF_USI_QUALIFIED_CHANNEL_DIAGNOSIS;
   PNET_DIAG_CH_PROP_MAINT_SET (
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
   PNET_DIAG_CH_PROP_MAINT_SET (
      diag_item.fmt.std.ch_properties,
      PNET_DIAG_CH_PROP_MAINT_FAULT);
}