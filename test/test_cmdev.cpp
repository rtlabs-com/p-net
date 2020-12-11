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

/**
 * @file
 * @brief Unit tests of features from the CMDEV state machine.
 *
 */

#include "utils_for_testing.h"
#include "mocks.h"

#include "pf_includes.h"

#include <gtest/gtest.h>

class CmdevUnitTest : public PnetUnitTest
{
};

TEST_F (CmdevUnitTest, CmdevCalculateDatadirectionInDescriptor)
{
   pf_data_direction_values_t resulting_direction;
   int ret;

   /* Module with no cyclic data */
   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_NO_IO,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_NO_IO,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_NO_IO,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_NO_IO,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   /* Input module */
   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_INPUT,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_INPUT,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_INPUT,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_INPUT,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   /* Output module */
   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_OUTPUT,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_OUTPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_OUTPUT,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_OUTPUT,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_OUTPUT,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_OUTPUT, resulting_direction);

   /* Input and output module */
   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_IO,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_OUTPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_IO,
      PF_DIRECTION_INPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_IO,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOCS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_INPUT, resulting_direction);

   ret = pf_cmdev_calculate_exp_sub_data_descriptor_direction (
      PNET_DIR_IO,
      PF_DIRECTION_OUTPUT,
      PF_DEV_STATUS_TYPE_IOPS,
      &resulting_direction);
   EXPECT_EQ (0, ret);
   EXPECT_EQ (PF_DIRECTION_OUTPUT, resulting_direction);
}

TEST_F (CmdevUnitTest, CmdevCheckZero)
{
   int ret;
   int ix;

   /* Shortest buffer */
   uint8_t buffer_1[1] = {0};
   ret = pf_cmdev_check_zero (buffer_1, 1);
   EXPECT_EQ (0, ret);
   for (ix = 1; ix < 0x100; ix++)
   {
      buffer_1[0] = ix;
      ret = pf_cmdev_check_zero (buffer_1, 1);
      EXPECT_EQ (-1, ret);
   }

   /* Small buffer */
   uint8_t buffer_5[5] = {0};
   ret = pf_cmdev_check_zero (buffer_5, 5);
   EXPECT_EQ (0, ret);
   for (ix = 1; ix < 0x100; ix++)
   {
      buffer_5[3] = ix;
      ret = pf_cmdev_check_zero (buffer_5, 5);
      EXPECT_EQ (-1, ret);
   }

   /* Buffer size larger than uint8_t */
   uint8_t buffer_500[500] = {0};
   ret = pf_cmdev_check_zero (buffer_500, 500);
   EXPECT_EQ (0, ret);
   for (ix = 1; ix < 0x100; ix++)
   {
      buffer_500[260] = ix;
      ret = pf_cmdev_check_zero (buffer_500, 500);
      EXPECT_EQ (-1, ret);
   }
   memset (buffer_500, 0, sizeof (buffer_500));
   ret = pf_cmdev_check_zero (buffer_500, 500);
   EXPECT_EQ (0, ret);
   memset (buffer_500, 9, sizeof (buffer_500));
   ret = pf_cmdev_check_zero (buffer_500, 500);
   EXPECT_EQ (-1, ret);

   /* Pass in length zero */
   ret = pf_cmdev_check_zero (buffer_500, 0);
   EXPECT_EQ (0, ret);
}

TEST_F (CmdevUnitTest, CmdevCheckVisibleString)
{
   int ix;
   int ret;

   ret = pf_cmdev_check_visible_string ("");
   EXPECT_EQ (-1, ret);

   ret = pf_cmdev_check_visible_string ("abc");
   EXPECT_EQ (0, ret);

   /* Short string */
   char buffer_20[20] = {0};
   ret = pf_cmdev_check_visible_string (buffer_20);
   EXPECT_EQ (-1, ret);

   buffer_20[0] = "A"[0];
   ret = pf_cmdev_check_visible_string (buffer_20);
   EXPECT_EQ (0, ret);

   memset (buffer_20, "A"[0], sizeof (buffer_20) - 1);
   ret = pf_cmdev_check_visible_string (buffer_20);
   EXPECT_EQ (0, ret);

   /* Test all character values */
   for (ix = 0x01; ix <= 0x1F; ix++)
   {
      buffer_20[10] = ix;
      ret = pf_cmdev_check_visible_string (buffer_20);
      EXPECT_EQ (-1, ret);
   }
   for (ix = 0x20; ix <= 0x7E; ix++)
   {
      buffer_20[10] = ix;
      ret = pf_cmdev_check_visible_string (buffer_20);
      EXPECT_EQ (0, ret);
   }
   for (ix = 0x7F; ix <= 0xFF; ix++)
   {
      buffer_20[10] = ix;
      ret = pf_cmdev_check_visible_string (buffer_20);
      EXPECT_EQ (-1, ret);
   }

   /* String length larger than uint8_t */
   char buffer_500[500] = {0};
   memset (buffer_500, "A"[0], sizeof (buffer_500) - 1);

   ret = pf_cmdev_check_visible_string (buffer_500);
   EXPECT_EQ (0, ret);

   buffer_500[300] = 0x07;
   ret = pf_cmdev_check_visible_string (buffer_500);
   EXPECT_EQ (-1, ret);
}

TEST_F (CmdevUnitTest, CmdevCheckNoStraddle)
{
   uint16_t start_a;
   uint16_t length_a;
   uint16_t start_b;
   uint16_t length_b;

   /*   x                      Area A                   */
   /*   0 1 2 3 4 5 6 7 8 9                             */
   /*   x                      Area B, initial position */
   start_a = 0;
   length_a = 1;
   length_b = 1;
   EXPECT_EQ (-1, pf_cmdev_check_no_straddle (start_a, length_a, 0, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 1, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 2, length_b));

   /*                          Area A                   */
   /*   0 1 2 3 4 5 6 7 8 9                             */
   /*   x                      Area B, initial position */
   start_a = 0;
   length_a = 0;
   length_b = 1;
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 0, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 1, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 2, length_b));

   /*                          Area A                   */
   /*   0 1 2 3 4 5 6 7 8 9                             */
   /*                          Area B, initial position */
   start_a = 0;
   length_a = 0;
   length_b = 0;
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 0, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 1, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 2, length_b));

   /*       x                  Area A                   */
   /*   0 1 2 3 4 5 6 7 8 9                             */
   /*   x                      Area B, initial position */
   start_a = 2;
   length_a = 1;
   length_b = 1;
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 0, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 1, length_b));
   EXPECT_EQ (-1, pf_cmdev_check_no_straddle (start_a, length_a, 2, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 3, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 4, length_b));

   /*       x x                Area A                   */
   /*   0 1 2 3 4 5 6 7 8 9                             */
   /*   x                      Area B, initial position */
   start_a = 2;
   length_a = 2;
   length_b = 1;
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 0, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 1, length_b));
   EXPECT_EQ (-1, pf_cmdev_check_no_straddle (start_a, length_a, 2, length_b));
   EXPECT_EQ (-1, pf_cmdev_check_no_straddle (start_a, length_a, 3, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 4, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 5, length_b));

   /*           x x x          Area A                   */
   /*   0 1 2 3 4 5 6 7 8 9                             */
   /*   x x                    Area B, initial position */
   start_a = 4;
   length_a = 3;
   length_b = 2;
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 0, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 1, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 2, length_b));
   EXPECT_EQ (-1, pf_cmdev_check_no_straddle (start_a, length_a, 3, length_b));
   EXPECT_EQ (-1, pf_cmdev_check_no_straddle (start_a, length_a, 4, length_b));
   EXPECT_EQ (-1, pf_cmdev_check_no_straddle (start_a, length_a, 5, length_b));
   EXPECT_EQ (-1, pf_cmdev_check_no_straddle (start_a, length_a, 6, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 7, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 8, length_b));
   EXPECT_EQ (0, pf_cmdev_check_no_straddle (start_a, length_a, 9, length_b));

   /* Values larger than uint8_t */
   start_a = 1000;
   length_a = 500;
   length_b = 300;
   for (start_b = 0; start_b <= 700; start_b++)
   {
      EXPECT_EQ (
         0,
         pf_cmdev_check_no_straddle (start_a, length_a, start_b, length_b));
      EXPECT_EQ (
         0,
         pf_cmdev_check_no_straddle (start_b, length_b, start_a, length_a));
   }
   for (start_b = 701; start_b <= 1499; start_b++)
   {
      EXPECT_EQ (
         -1,
         pf_cmdev_check_no_straddle (start_a, length_a, start_b, length_b));
      EXPECT_EQ (
         -1,
         pf_cmdev_check_no_straddle (start_b, length_b, start_a, length_a));
   }
   for (start_b = 1500; start_b < 0xFFFF; start_b++)
   {
      EXPECT_EQ (
         0,
         pf_cmdev_check_no_straddle (start_a, length_a, start_b, length_b));
      EXPECT_EQ (
         0,
         pf_cmdev_check_no_straddle (start_b, length_b, start_a, length_a));
   }
   EXPECT_EQ (
      0,
      pf_cmdev_check_no_straddle (start_a, length_a, 0xFFFF, length_b));
   EXPECT_EQ (
      0,
      pf_cmdev_check_no_straddle (0xFFFF, length_b, start_a, length_a));
}

TEST_F (CmdevUnitTest, CmdevCheckArType)
{
   int ret;

   for (uint16_t ar_type = 0; ar_type < 0xFF; ar_type++)
   {
      ret = pf_cmdev_check_ar_type (ar_type);

      if (ar_type == PF_ART_IOCAR_SINGLE || ar_type == PF_ART_IOSAR)
      {
         EXPECT_EQ (0, ret);
      }
      else
      {
         EXPECT_EQ (-1, ret);
      }
   }

   ret = pf_cmdev_check_ar_type (0xFFFF);
   EXPECT_EQ (-1, ret);
}
