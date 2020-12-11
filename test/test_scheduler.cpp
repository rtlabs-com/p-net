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

class SchedulerTest : public PnetIntegrationTest
{
};

class SchedulerUnitTest : public PnetUnitTest
{
};

TEST_F (SchedulerUnitTest, SchedulerSanitizeDelayTest)
{
   const uint32_t cycle_len = 1000;
   const uint32_t margin = 10u;
   uint32_t result = 0;
   uint32_t input_delay = 0;

   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = 500;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = 1000;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = 1400;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = 1600;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 1500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 2000, margin);

   input_delay = 2000;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 1500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 2000, margin);

   input_delay = 2400;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 1500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 2000, margin);

   input_delay = 2600;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 2500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 3000, margin);

   input_delay = 3000;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 2500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 3000, margin);

   input_delay = 3400;
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 2500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 3000, margin);

   input_delay = 1000000; /* 1 second */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 999500u, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000000, margin);

   input_delay = 10000000; /* 10 seconds */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 9999500u, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 10000000, margin);

   input_delay = 1000000000; /* Unrealisticly far in the future (1000 s) */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);

   input_delay = (uint32_t) (-4); /* Next execution has already passed */
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, true);
   ASSERT_NEAR (result, 500, margin);
   result = pf_scheduler_sanitize_delay (input_delay, cycle_len, false);
   ASSERT_NEAR (result, 1000, margin);
}
