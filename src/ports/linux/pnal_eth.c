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

#include <net/ethernet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>

#include <stdlib.h>
#include <string.h>

/**
 * @internal
 * Run a thread that listens to incoming raw Ethernet sockets.
 * Delegate the actual work to thread_arg->callback
 *
 * This is a function to be passed into os_thread_create()
 * Do not change the argument types.
 *
 * @param thread_arg     InOut: Will be converted to pnal_eth_handle_t
 */
static void os_eth_task (void * thread_arg)
{
   pnal_eth_handle_t * eth_handle = thread_arg;
   ssize_t readlen;
   int handled = 0;

   pnal_buf_t * p = pnal_buf_alloc (OS_BUF_MAX_SIZE);
   assert (p != NULL);

   while (1)
   {
      readlen = recv (eth_handle->socket, p->payload, OS_BUF_MAX_SIZE, 0);
      if (readlen == -1)
         continue;
      p->len = readlen;

      if (eth_handle->callback != NULL)
      {
         handled = eth_handle->callback (eth_handle->arg, p);
      }
      else
      {
         handled = 0; /* Message not handled */
      }

      if (handled == 1)
      {
         p = pnal_buf_alloc (OS_BUF_MAX_SIZE);
         assert (p != NULL);
      }
   }
}

pnal_eth_handle_t * pnal_eth_init (
   const char * if_name,
   pnal_eth_callback_t * callback,
   void * arg)
{
   pnal_eth_handle_t * handle;
   int i;
   struct ifreq ifr;
   struct sockaddr_ll sll;
   int ifindex;
   struct timeval timeout;

   handle = malloc (sizeof (pnal_eth_handle_t));
   if (handle == NULL)
   {
      return NULL;
   }

   handle->arg = arg;
   handle->callback = callback;
   handle->socket = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL));

   /* Adjust send timeout */
   timeout.tv_sec = 0;
   timeout.tv_usec = 1;
   setsockopt (
      handle->socket,
      SOL_SOCKET,
      SO_SNDTIMEO,
      &timeout,
      sizeof (timeout));

   /* Send outgoing messages directly to the interface, without using Linux
    * routing */
   i = 1;
   setsockopt (handle->socket, SOL_SOCKET, SO_DONTROUTE, &i, sizeof (i));

   /* Read interface index */
   strcpy (ifr.ifr_name, if_name);
   ioctl (handle->socket, SIOCGIFINDEX, &ifr);
   ifindex = ifr.ifr_ifindex;

   /* Set flags of NIC interface, here promiscuous and broadcast */
   strcpy (ifr.ifr_name, if_name);
   ifr.ifr_flags = 0;
   ioctl (handle->socket, SIOCGIFFLAGS, &ifr);
   ifr.ifr_flags = ifr.ifr_flags | IFF_PROMISC | IFF_BROADCAST;
   ioctl (handle->socket, SIOCSIFFLAGS, &ifr);

   /* Bind socket to all protocols */
   sll.sll_family = AF_PACKET;
   sll.sll_ifindex = ifindex;
   sll.sll_protocol = htons (ETH_P_ALL);
   bind (handle->socket, (struct sockaddr *)&sll, sizeof (sll));

   if (handle->socket > -1)
   {
      handle->thread =
         os_thread_create ("os_eth_task", 10, 4096, os_eth_task, handle);
      return handle;
   }
   else
   {
      free (handle);
      return NULL;
   }
}

int pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf)
{
   int ret = send (handle->socket, buf->payload, buf->len, 0);

   return ret;
}
