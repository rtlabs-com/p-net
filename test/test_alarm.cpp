/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
*/

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


TEST_F (AlarmUnitTest, AlarmCheckQueueHandling)
{
   pf_alarm_queue_t queue;
   pf_alarm_data_t alarm_input;
   pf_alarm_data_t alarm_output;
   int ix = 0;
   int err = 0;
   const uint16_t SEQUENCE_START_NUMBER = 100;

   /* Prepare default data */
   memset (&queue, 0, sizeof (queue));
   memset (&alarm_input, 0, sizeof (alarm_input));
   memset (&alarm_output, 0, sizeof (alarm_output));
   EXPECT_EQ (queue.count, 0);

   /* Fill the queue */
   for (ix = 0; ix < PNET_MAX_ALARMS; ix++)
   {
      alarm_input.sequence_number = SEQUENCE_START_NUMBER + ix;
      err = pf_alarm_add_send_queue(&queue, &alarm_input);
      EXPECT_EQ (err, 0);
   }
   EXPECT_EQ (queue.count, PNET_MAX_ALARMS);

   /* Add to full queue */
   alarm_input.sequence_number += 1;
   err = pf_alarm_add_send_queue(&queue, &alarm_input);
   EXPECT_EQ (err, -1);
   EXPECT_EQ (queue.count, PNET_MAX_ALARMS);

   /* Fetch from the queue */
   for (ix = 0; ix < PNET_MAX_ALARMS; ix++)
   {
      err = pf_alarm_fetch_send_queue(&queue, &alarm_output);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (alarm_output.sequence_number, SEQUENCE_START_NUMBER + ix);
   }
   EXPECT_EQ (queue.count, 0);

   /* Fetch from empty queue */
   err = pf_alarm_fetch_send_queue(&queue, &alarm_output);
   EXPECT_EQ (err, -1);
   EXPECT_EQ (queue.count, 0);

   /* Wrap read_index and write_index */
   for (ix = 0; ix < PNET_MAX_ALARMS * 5; ix++)
   {
      err = pf_alarm_add_send_queue(&queue, &alarm_input);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (queue.count, 1);

      err = pf_alarm_fetch_send_queue(&queue, &alarm_output);
      EXPECT_EQ (err, 0);
      EXPECT_EQ (queue.count, 0);
   }
}
