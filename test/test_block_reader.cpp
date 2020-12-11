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

#include "pf_block_reader.h"
#include "pf_includes.h"

#include <gtest/gtest.h>

class BlockReaderUnitTest : public PnetUnitTest
{
};

TEST_F (BlockReaderUnitTest, BlockReaderTestGetBits)
{

   /* 11111111 11111111 11111111 11111111 */
   EXPECT_EQ (0x00000000ul, pf_get_bits (0xFFFFFFFF, 0, 0));
   EXPECT_EQ (0x00000001ul, pf_get_bits (0xFFFFFFFF, 0, 1));
   EXPECT_EQ (0x00000003ul, pf_get_bits (0xFFFFFFFF, 0, 2));
   EXPECT_EQ (0x00000007ul, pf_get_bits (0xFFFFFFFF, 0, 3));
   EXPECT_EQ (0x0000000Ful, pf_get_bits (0xFFFFFFFF, 0, 4));
   EXPECT_EQ (0x0000001Ful, pf_get_bits (0xFFFFFFFF, 0, 5));
   EXPECT_EQ (0x0000003Ful, pf_get_bits (0xFFFFFFFF, 0, 6));
   EXPECT_EQ (0x0000007Ful, pf_get_bits (0xFFFFFFFF, 0, 7));
   EXPECT_EQ (0x000000FFul, pf_get_bits (0xFFFFFFFF, 0, 8));
   EXPECT_EQ (0x7FFFFFFFul, pf_get_bits (0xFFFFFFFF, 0, 31));
   EXPECT_EQ (0xFFFFFFFFul, pf_get_bits (0xFFFFFFFF, 0, 32));
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 0, 33)); /* Illegal length */

   EXPECT_EQ (0x00000000ul, pf_get_bits (0xFFFFFFFF, 1, 0));
   EXPECT_EQ (0x00000001ul, pf_get_bits (0xFFFFFFFF, 1, 1));
   EXPECT_EQ (0x00000003ul, pf_get_bits (0xFFFFFFFF, 1, 2));
   EXPECT_EQ (0x00000007ul, pf_get_bits (0xFFFFFFFF, 1, 3));
   EXPECT_EQ (0x000000FFul, pf_get_bits (0xFFFFFFFF, 1, 8));
   EXPECT_EQ (0x7FFFFFFFul, pf_get_bits (0xFFFFFFFF, 1, 31));
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 1, 32)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 1, 33)); /* Illegal length */

   EXPECT_EQ (0x00000000ul, pf_get_bits (0xFFFFFFFF, 31, 0));
   EXPECT_EQ (0x00000001ul, pf_get_bits (0xFFFFFFFF, 31, 1));
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 31, 2));  /* Illegal length and
                                                         position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 31, 32)); /* Illegal length and
                                                         position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 31, 33)); /* Illegal length */

   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 32, 0));  /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 32, 1));  /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 32, 32)); /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 32, 33)); /* Illegal position and
                                                         illegal length */

   EXPECT_EQ (0ul, pf_get_bits (0xFFFFFFFF, 0xFF, 0xFF)); /* Illegal position
                                                             and illegal length
                                                           */

   /* 00000000 00000000 00000000 00010000 */
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 0, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 1, 3));
   EXPECT_EQ (0x00000004ul, pf_get_bits (0x00000010, 2, 3));
   EXPECT_EQ (0x00000002ul, pf_get_bits (0x00000010, 3, 3));
   EXPECT_EQ (0x00000001ul, pf_get_bits (0x00000010, 4, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 5, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 28, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000010, 29, 3));
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 30, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 31, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 32, 3)); /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 33, 3)); /* Illegal position */

   /* 00000000 00000000 00000000 00000001 */
   EXPECT_EQ (0x00000001ul, pf_get_bits (0x00000001, 0, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000001, 1, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000001, 2, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000001, 3, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x00000001, 4, 3));
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 30, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 31, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 32, 3)); /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0x00000010, 33, 3)); /* Illegal position */

   /* 10000000 00000000 00000000 00000000 */
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x80000000, 0, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x80000000, 1, 3));
   EXPECT_EQ (0x00000000ul, pf_get_bits (0x80000000, 28, 3));
   EXPECT_EQ (0x00000004ul, pf_get_bits (0x80000000, 29, 3));
   EXPECT_EQ (0ul, pf_get_bits (0x80000000, 30, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x80000000, 31, 3)); /* Illegal length and
                                                        position combination*/
   EXPECT_EQ (0ul, pf_get_bits (0x80000000, 32, 3)); /* Illegal position */
   EXPECT_EQ (0ul, pf_get_bits (0x80000000, 33, 3)); /* Illegal position */
}
