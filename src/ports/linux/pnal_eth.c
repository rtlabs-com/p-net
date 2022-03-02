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
 * @brief Functions for raw Ethernet frames on Linux
 */

#include "pnal.h"

#include <net/ethernet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>

#include <stdlib.h>
#include <string.h>

typedef struct eth_port_handle
{
   pnal_eth_callback_t * callback;
   void * arg;
   int socket;
   os_thread_t * thread;
   bool is_management_port;
   int loc_port_num;
} eth_port_handle_t;

static uint8_t total_number_of_ports = 0;
static eth_port_handle_t handles[PNET_MAX_PHYSICAL_PORTS + 1] = {0};

/**
 * Get handle to stored port info.
 *
 *  One physical port    Two physical ports
 *  total_ports=1        total_ports=3            Index
 *  ┌─────────────────┐  ┌─────────────────┐
 *  │ Management port │  │ Management port │      0
 *  │ Physical port 1 │  │                 │
 *  └─────────────────┘  ├─────────────────┤
 *                       │ Physical port 1 │      1
 *                       │                 │
 *                       ├─────────────────┤
 *                       │ Physical port 2 │      2
 *                       │                 │
 *                       └─────────────────┘
 * @param is_management_port    In: True if we look for the management port
 * @param loc_port_num          In: Local port number we look for (or 0 when
 *                                  looking for management port)
 * @param total_ports           In: Total number of ports
 * @return  Address, or NULL if an error occurred.
 */
static eth_port_handle_t * get_handle (
   bool is_management_port,
   int loc_port_num,
   uint8_t total_ports)
{
   if (is_management_port)
   {
      return &handles[0];
   }

   if (loc_port_num <= 0 || total_ports == 0 || total_ports == 2)
   {
      return NULL;
   }

   if (total_ports == 1 && loc_port_num > 1)
   {
      return NULL;
   }

   /* When management port is same as physical port 1 */
   if (total_ports == 1 && loc_port_num == 1)
   {
      return &handles[0];
   }

   return &handles[loc_port_num];
}

/**
 * @internal
 * Run a thread that listens to incoming raw Ethernet sockets.
 * Delegate the actual work to thread_arg->callback
 *
 * This is a function to be passed into os_thread_create()
 * Do not change the argument types.
 *
 * @param thread_arg     InOut: Will be converted to eth_port_handle_t
 */
static void os_eth_task (void * thread_arg)
{
   eth_port_handle_t * eth_handle = thread_arg;
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
         handled = eth_handle->callback (
            eth_handle->is_management_port,
            eth_handle->loc_port_num,
            eth_handle->arg,
            p);
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

int pnal_eth_init (
   const char * if_name,
   bool is_management_port,
   int loc_port_num,
   pnal_ethertype_t receive_type,
   const pnal_cfg_t * pnal_cfg,
   pnal_eth_callback_t * callback,
   void * arg,
   uint8_t total_ports)
{
   eth_port_handle_t * handle;
   int i;
   struct ifreq ifr;
   struct sockaddr_ll sll;
   int ifindex;
   struct timeval timeout;
   const uint16_t linux_receive_type =
      (receive_type == PNAL_ETHTYPE_ALL) ? ETH_P_ALL : receive_type;

   handle = get_handle (is_management_port, loc_port_num, total_ports);
   if (handle == NULL)
   {
      return -1;
   }

   handle->is_management_port = is_management_port;
   handle->loc_port_num = loc_port_num;
   handle->callback = callback;
   handle->arg = arg;
   handle->socket = socket (PF_PACKET, SOCK_RAW, htons (linux_receive_type));

   /* Update static variable */
   total_number_of_ports = total_ports;

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

   /* Set flags of NIC interface */
   strcpy (ifr.ifr_name, if_name);
   ifr.ifr_flags = 0;
   ioctl (handle->socket, SIOCGIFFLAGS, &ifr);
   ifr.ifr_flags = ifr.ifr_flags | IFF_MULTICAST | IFF_BROADCAST;
   if (receive_type == PNAL_ETHTYPE_ALL)
   {
      ifr.ifr_flags |= IFF_ALLMULTI; /* Receive all multicasts */
   }
   ioctl (handle->socket, SIOCSIFFLAGS, &ifr);

   /* Bind socket to relevant protocol */
   sll.sll_family = AF_PACKET;
   sll.sll_ifindex = ifindex;
   sll.sll_protocol = htons (linux_receive_type);
   bind (handle->socket, (struct sockaddr *)&sll, sizeof (sll));

   if (handle->socket > -1)
   {
      handle->thread = os_thread_create (
         "os_eth_task",
         pnal_cfg->eth_recv_thread.prio,
         pnal_cfg->eth_recv_thread.stack_size,
         os_eth_task,
         handle);
      return 0;
   }
   else
   {
      return -1;
   }
}

int pnal_eth_send_on_management_port (pnal_buf_t * buf)
{
   eth_port_handle_t * handle = get_handle (true, 0, total_number_of_ports);

   if (handle == NULL)
   {
      return -1;
   }

   return send (handle->socket, buf->payload, buf->len, 0);
}

int pnal_eth_send_on_physical_port (int loc_port_num, pnal_buf_t * buf)
{
   eth_port_handle_t * handle =
      get_handle (false, loc_port_num, total_number_of_ports);

   if (handle == NULL)
   {
      return -1;
   }

   return send (handle->socket, buf->payload, buf->len, 0);
}
