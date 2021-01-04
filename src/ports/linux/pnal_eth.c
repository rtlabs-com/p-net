/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief Linux Ethernet related functions that use \a pnal_eth_handle_t
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

   pnal_buf_t * p = pnal_buf_alloc (PNAL_BUF_MAX_SIZE);
   assert (p != NULL);

   while (1)
   {
      readlen = recv (eth_handle->socket, p->payload, PNAL_BUF_MAX_SIZE, 0);
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
         p = pnal_buf_alloc (PNAL_BUF_MAX_SIZE);
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
