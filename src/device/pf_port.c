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

#include "pf_includes.h"

#include <string.h>

#if PNET_MAX_PORT < 1
#error "PNET_MAX_PORT needs to be at least 1"
#endif

/**
 * @file
 * @brief Manage runtime data for multiple ports
 */

void pf_port_init(pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);

   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);
      p_port_data->port_num = port;
      port = pf_port_get_next (&port_iterator);
   }
}

void pf_port_get_list_of_ports (pnet_t * net, pf_lldp_port_list_t * p_list)
{
   /* TODO: Implement support for multiple ports */
   memset (p_list, 0, sizeof (*p_list));
   p_list->ports[0] = BIT (8 - 1);
}

void pf_port_init_iterator_over_ports (pnet_t * net, pf_port_iterator_t * p_iterator)
{
   p_iterator->next_port = 1;
}

int pf_port_get_next (pf_port_iterator_t * p_iterator)
{
   int ret = p_iterator->next_port;

   if (p_iterator->next_port == PNET_MAX_PORT || p_iterator->next_port == 0)
   {
      p_iterator->next_port = 0;
   }
   else
   {
      p_iterator->next_port += 1;
   }

   return ret;
}

pf_port_t * pf_port_get_state (
   pnet_t * net,
   int loc_port_num)
{
   CC_ASSERT(loc_port_num > 0 && loc_port_num <= PNET_MAX_PORT);
   return &net->port[loc_port_num - 1];
}
