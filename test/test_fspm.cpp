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

class FspmUnitTest : public PnetUnitTest
{
};

TEST_F (FspmUnitTest, FspmCheckValidateConfiguration)
{
   pnet_cfg_t cfg;

   /* Known good values */
   memset (&cfg, 0, sizeof (pnet_cfg_t));
   cfg.min_device_interval = 1;
   cfg.im_0_data.im_supported = 0;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   /* Check pointer validity */
   EXPECT_EQ (pf_fspm_validate_configuration (NULL), -1);

   /* Check minimum stack update interval */
   cfg.min_device_interval = 0;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.min_device_interval = 0x1000;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.min_device_interval = 0x1001;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.min_device_interval = 0xFFFF;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);
   cfg.min_device_interval = 1;

   /* Check supported I&M fields */
   cfg.im_0_data.im_supported = PNET_SUPPORTED_IM1;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.im_0_data.im_supported = PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.im_0_data.im_supported = PNET_SUPPORTED_IM1 | PNET_SUPPORTED_IM2 |
                                PNET_SUPPORTED_IM3 | PNET_SUPPORTED_IM4;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.im_0_data.im_supported = PNET_SUPPORTED_IM4;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), 0);

   cfg.im_0_data.im_supported = 1;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.im_0_data.im_supported = 0x0020;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.im_0_data.im_supported = 0x8000;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);

   cfg.im_0_data.im_supported = 0xFFFF;
   EXPECT_EQ (pf_fspm_validate_configuration (&cfg), -1);
   cfg.im_0_data.im_supported = 0;
}
