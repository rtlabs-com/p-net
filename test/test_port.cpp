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



class PortTest : public PnetIntegrationTest
{
};

TEST_F (PortTest, PortGetPortList)
{
   pf_lldp_port_list_t port_list;

   memset (&port_list, 0xff, sizeof (port_list));

   /* TODO: Add support for multiple ports */
   pf_port_get_list_of_ports (net, &port_list);
   EXPECT_EQ (port_list.ports[0], 0x80);
   EXPECT_EQ (port_list.ports[1], 0x00);
}

TEST_F (PortTest, PortCheckIterator)
{
   pf_port_iterator_t port_iterator;
   int port;
   int ix;

   /* Retrieve first port */
   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 1);

   /* More ports might be available dependent on compile time setting */
   for(ix = 2; ix <= PNET_MAX_PORT; ix++)
   {
      port = pf_port_get_next (&port_iterator);
      EXPECT_EQ (port, ix);
   }

   /* Verify that we return 0 when we are done */
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 0);

   /* Do not restart automatically */
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 0);

   /* Restart the iterator */
   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   EXPECT_EQ (port, 1);
}
