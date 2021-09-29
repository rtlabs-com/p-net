/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

/**
 * @file
 * @brief STM32Cube Ethernet related functions that use \a pnal_eth_handle_t
 */

#include "pnal.h"
#include "osal_log.h"

#include <lwip/netif.h>
#include <lwip/apps/snmp_core.h>

#define MAX_NUMBER_OF_IF 1

struct pnal_eth_handle
{
   struct netif * netif;
   pnal_eth_callback_t * eth_rx_callback;
   void * arg;
};

static pnal_eth_handle_t interface[MAX_NUMBER_OF_IF];
static int nic_index = 0;

/**
 * Find PNAL network interface handle
 *
 * @param netif            In:    lwip network interface.
 * @return PNAL network interface handle corresponding to \a netif,
 *         NULL otherwise.
 */
static pnal_eth_handle_t * pnal_eth_find_handle (struct netif * netif)
{
   pnal_eth_handle_t * handle;
   int i;

   for (i = 0; i < MAX_NUMBER_OF_IF; i++)
   {
      handle = &interface[i];
      if (handle->netif == netif)
      {
         return handle;
      }
   }

   return NULL;
}

/**
 * Allocate PNAL network interface handle
 *
 * Handles are allocated from a static array and need never be freed.
 *
 * @return PNAL network interface handle if available,
 *         NULL if too many handles were allocated.
 */
static pnal_eth_handle_t * pnal_eth_allocate_handle (void)
{
   pnal_eth_handle_t * handle;

   if (nic_index < MAX_NUMBER_OF_IF)
   {
      handle = &interface[nic_index];
      nic_index++;
      return handle;
   }
   else
   {
      return NULL;
   }
}

/**
 * Process received Ethernet frame
 *
 * Called from lwip when an Ethernet frame is received with an EtherType
 * lwip is not aware of (e.g. Profinet and LLDP).
 *
 * @param p_buf            InOut: Packet buffer containing Ethernet frame.
 * @param netif            InOut: Network interface receiving the frame.
 * @return ERR_OK if frame was processed and freed,
 *         ERR_IF if it was ignored.
 */
static err_t pnal_eth_sys_recv (struct pbuf * p_buf, struct netif * netif)
{
   int processed;
   pnal_eth_handle_t * handle;

   handle = pnal_eth_find_handle (netif);
   if (handle == NULL)
   {
      /* p-net not started yet, let lwIP handle frame */
      return ERR_IF;
   }

   processed = handle->eth_rx_callback (handle, handle->arg, p_buf);
   if (processed)
   {
      /* Frame handled and freed */
      return ERR_OK;
   }
   else
   {
      /* Frame not handled */
      return ERR_IF;
   }
}

err_enum_t lwip_hook_unknown_eth_protocol (
   struct pbuf * pbuf,
   struct netif * netif)
{
   return pnal_eth_sys_recv (pbuf, netif);
}

pnal_eth_handle_t * pnal_eth_init (
   const char * if_name,
   pnal_ethertype_t receive_type,
   const pnal_cfg_t * pnal_cfg,
   pnal_eth_callback_t * callback,
   void * arg)
{
   pnal_eth_handle_t * handle;
   struct netif * netif;

   (void)receive_type; /* Ignore, for now all frames will be received. */

   netif = netif_find (if_name);
   if (netif == NULL)
   {
      os_log (LOG_LEVEL_ERROR, "Network interface \"%s\" not found!\n", if_name);
      return NULL;
   }

   handle = pnal_eth_allocate_handle();
   if (handle == NULL)
   {
      os_log (LOG_LEVEL_ERROR, "Too many network interfaces\n");
      return NULL;
   }

   handle->arg = arg;
   handle->eth_rx_callback = callback;
   handle->netif = netif;

   return handle;
}

int pnal_eth_send (pnal_eth_handle_t * handle, pnal_buf_t * buf)
{
   int ret = -1;

   CC_ASSERT (handle->netif->linkoutput != NULL);

   /* TODO: Determine if buf could ever be NULL here */
   if (buf != NULL)
   {
      /* TODO: remove tot_len from os_buff */
      buf->tot_len = buf->len;

      handle->netif->linkoutput (handle->netif, buf);
      ret = buf->len;
   }
   return ret;
}
