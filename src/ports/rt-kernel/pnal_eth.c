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
 * @brief rt-kernel Ethernet related functions that use \a pnal_eth_handle_t
 */

#include "pnal.h"
#include "osal_log.h"

#include <lwip/netif.h>
#include <lwip/apps/snmp_core.h>
#include <drivers/dev.h>

#define MAX_NUMBER_OF_IF 1
#define NET_DRIVER_NAME  "/net"

typedef struct pnal_eth_handle_t
{
   struct netif * netif;
   pnal_eth_callback_t * eth_rx_callback;
   void * arg;
} pnal_eth_handle_t;

static pnal_eth_handle_t interface[MAX_NUMBER_OF_IF];
static int nic_index = 0;

static int pnal_eth_sys_recv (
   struct netif * netif,
   void * arg,
   pnal_buf_t * p_buf)
{
   int ret = -1;
   pnal_eth_handle_t * handle;
   if (arg != NULL && p_buf != NULL)
   {
      handle = (pnal_eth_handle_t *)arg;
      ret = handle->eth_rx_callback (handle, handle->arg, p_buf);
   }
   return ret;
}

pnal_eth_handle_t * pnal_eth_init (
   const char * if_name,
   pnal_ethertype_t receive_type,
   pnal_eth_callback_t * callback,
   void * arg)
{
   pnal_eth_handle_t * handle = NULL;
   struct netif * found_if;
   drv_t * drv;

   (void)receive_type; /* Ignore, for now all frames will be received. */

   if (nic_index < MAX_NUMBER_OF_IF)
   {
      drv = dev_find_driver (NET_DRIVER_NAME);
      found_if = netif_find (if_name);
      if (drv != NULL && found_if != NULL)
      {
         handle = &interface[nic_index];
         nic_index++;

         handle->arg = arg;
         handle->eth_rx_callback = callback;
         handle->netif = found_if;

         eth_ioctl (drv, handle, IOCTL_NET_SET_RX_HOOK, pnal_eth_sys_recv);
      }
      else
      {
         os_log (LOG_LEVEL_ERROR, "Driver \"%s\" not found!\n", NET_DRIVER_NAME);
      }
   }

   return handle;
}

int pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf)
{
   int ret = -1;

   if (buf != NULL)
   {
      /* TODO: remove tot_len from os_buff */
      buf->tot_len = buf->len;

      if (handle->netif->linkoutput != NULL)
      {
         handle->netif->linkoutput (handle->netif, buf);
         ret = buf->len;
      }
   }
   return ret;
}
