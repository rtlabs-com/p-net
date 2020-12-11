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

class PpmTest : public PnetIntegrationTest
{
};

class PpmUnitTest : public PnetUnitTest
{
};

TEST_F (PpmTest, PpmTestCalculateCycle)
{
   /* Cycle length 31.25 us, transmission period 31.25 us */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 1, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (20, 1, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (40, 1, 1), 1);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (70, 1, 1), 2);

   /* Cycle length 62.5 us, transmission period 62.5 us */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 2, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (10, 2, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (50, 2, 1), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (80, 2, 1), 2);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (100, 2, 1), 2);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (140, 2, 1), 4);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (160, 2, 1), 4);

   /* Cycle length 62.5 us, transmission period 500 us */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 2, 8), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (250, 2, 8), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (750, 2, 8), 16);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (1250, 2, 8), 32);

   /* Cycle length 125 us, transmission period 500 us */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 4, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (250, 4, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (750, 4, 4), 16);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (1250, 4, 4), 32);

   /* Cycle length 1 ms, transmission period 4 ms */
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (0, 32, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (3000, 32, 4), 0);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (5000, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (7000, 32, 4), 128);
   EXPECT_EQ (pf_ppm_calculate_cyclecounter (9000, 32, 4), 256);
}
