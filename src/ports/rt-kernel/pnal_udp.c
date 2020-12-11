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

#include "pnal.h"
#include "osal_log.h"

#include <sys/socket.h>
#include <string.h>

int pnal_udp_open (pnal_ipaddr_t addr, pnal_ipport_t port)
{
   int ret = -1;
   struct sockaddr_in local;
   int id;

   id = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   if (id < 0)
   {
      ret = id;
   }
   else
   {
      /* set IP and port number */
      local = (struct sockaddr_in){
         .sin_family = AF_INET,
         .sin_addr.s_addr = htonl (addr),
         .sin_port = htons (port),
      };
      ret = bind (id, (struct sockaddr *)&local, sizeof (local));
      if (ret != 0)
      {
         close (id);
      }
      else
      {
         ret = id;
      }
   }

   return ret;
}

int pnal_udp_sendto (
   uint32_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size)
{
   struct sockaddr_in remote;
   int len;

   remote = (struct sockaddr_in){
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl (dst_addr),
      .sin_port = htons (dst_port),
   };
   len =
      sendto (id, data, size, 0, (struct sockaddr *)&remote, sizeof (remote));

   return len;
}

int pnal_udp_recvfrom (
   uint32_t id,
   pnal_ipaddr_t * src_addr,
   pnal_ipport_t * src_port,
   uint8_t * data,
   int size)
{
   struct sockaddr_in remote;
   socklen_t addr_len = sizeof (remote);
   int len;

   memset (&remote, 0, sizeof (remote));
   len = recvfrom (
      id,
      data,
      size,
      MSG_DONTWAIT,
      (struct sockaddr *)&remote,
      &addr_len);
   if (len > 0)
   {
      *src_addr = ntohl (remote.sin_addr.s_addr);
      *src_port = ntohs (remote.sin_port);
   }

   return len;
}

void pnal_udp_close (uint32_t id)
{
   close (id);
}
