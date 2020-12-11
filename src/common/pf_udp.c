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
 * @brief Update interface statistics for UDP communications
 */

#ifdef UNIT_TEST
#define pnal_udp_close    mock_pnal_udp_close
#define pnal_udp_open     mock_pnal_udp_open
#define pnal_udp_recvfrom mock_pnal_udp_recvfrom
#define pnal_udp_sendto   mock_pnal_udp_sendto
#endif

#include <string.h>
#include "pf_includes.h"

int pf_udp_open (pnet_t * net, pnal_ipport_t port)
{
   return pnal_udp_open (PNAL_IPADDR_ANY, port);
}

int pf_udp_sendto (
   pnet_t * net,
   uint32_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size)
{
   int sent_len = 0;

   sent_len = pnal_udp_sendto (id, dst_addr, dst_port, data, size);

   if (sent_len != size)
   {
      net->interface_statistics.if_out_errors++;
      LOG_ERROR (
         PNET_LOG,
         "UDP(%d): Failed to send %u UDP bytes payload on the socket.\n",
         __LINE__,
         size);
   }
   else
   {
      net->interface_statistics.if_out_octets += sent_len;
   }

   return sent_len;
}

int pf_udp_recvfrom (
   pnet_t * net,
   uint32_t id,
   pnal_ipaddr_t * src_addr,
   pnal_ipport_t * src_port,
   uint8_t * data,
   int size)
{
   int input_len = 0;

   input_len = pnal_udp_recvfrom (id, src_addr, src_port, data, size);

   net->interface_statistics.if_in_octets += input_len;

   return input_len;
}

void pf_udp_close (pnet_t * net, uint32_t id)
{
   pnal_udp_close (id);
}
