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

class CpmUnitTest : public PnetUnitTest
{
};

TEST_F (CpmUnitTest, CpmCheckCycle)
{
   EXPECT_EQ (-1, pf_cpm_check_cycle (1, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (1, 0));
   EXPECT_EQ (-1, pf_cpm_check_cycle (1, 1));
   EXPECT_EQ (0, pf_cpm_check_cycle (1, 2));
   EXPECT_EQ (0, pf_cpm_check_cycle (1, 3));

   /* Previous counter value was 0x4000 */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x0000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x0001));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x2FFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x2FFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x3000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3002));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3FFD));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3FFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x3FFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x4000, 0x4000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x4001));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0x4002));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x4000, 0xFFFF));

   /* Previous counter value was 0xFFFF (Forbidden zone ends at 0xFFFF) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0xEFFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0xEFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0xFFFF, 0xF000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0xFFFF, 0xF001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0xFFFF, 0xFFFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0xFFFF, 0xFFFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0x0000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0x0001));
   EXPECT_EQ (0, pf_cpm_check_cycle (0xFFFF, 0x0002));

   /* Previous counter value was 0x0000 (Forbidden zone ends at 0x0000) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0xEFFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0xEFFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0xF000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0000, 0xF001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0000, 0xFFFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0000, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0000, 0x0000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0x0001));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0000, 0x0002));

   /* Previous counter value was 0x0FFE (Forbidden zone starts at 0xFFFF) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFE, 0xFFFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFE, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFE, 0x0000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFE, 0x0001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFE, 0x0FFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFE, 0x0FFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFE, 0x1000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFE, 0x1001));

   /* Previous counter value was 0x0FFF (Forbidden zone starts at zero) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFF, 0xFFFE));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFF, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFF, 0x0000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFF, 0x0001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFF, 0x0FFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0FFF, 0x0FFF));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFF, 0x1000));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0FFF, 0x1001));

   /* Previous counter value was 0x0010 (Forbidden zone overlaps zero) */
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0010, 0xF00F));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0010, 0xF010));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0xF011));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0xFFFE));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0xFFFF));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x0000));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x0001));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x000E));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x000F));
   EXPECT_EQ (-1, pf_cpm_check_cycle (0x0010, 0x0010));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0010, 0x0011));
   EXPECT_EQ (0, pf_cpm_check_cycle (0x0010, 0x0012));
}
