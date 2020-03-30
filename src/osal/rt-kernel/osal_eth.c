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

#include "pf_includes.h"
#include <lwip/netif.h>
#include <dev.h>
#include <uassert.h>

#define MAX_NUMBER_OF_IF   1
#define NET_DRIVER_NAME    "/net"

static struct netif * interface[MAX_NUMBER_OF_IF];
static int nic_index = 0;


os_eth_handle_t* os_eth_init(
   const char              *if_name,
   os_eth_callback_t       *callback,
   void                    *arg)
{
   os_eth_handle_t         *handle;
   struct netif            *found_if;
   drv_t                   *drv;

   handle = malloc(sizeof(os_eth_handle_t));
   UASSERT (handle != NULL, EMEM);

   handle->arg = arg;
   handle->callback = callback;
   handle->if_id = -1;

   if (nic_index < MAX_NUMBER_OF_IF)
   {
      drv = dev_find_driver (NET_DRIVER_NAME);
      if (drv != NULL)
      {
         eth_ioctl(drv, arg, IOCTL_NET_SET_RX_HOOK, callback);

         found_if = netif_find(if_name);
         if (found_if == NULL)
         {
            interface[nic_index] = netif_default;
         }
         else
         {
            interface[nic_index] = found_if;
         }
         handle->if_id = nic_index++;
      }
      else
      {
         os_log(LOG_LEVEL_ERROR, "Driver \"%s\" not found!\n", NET_DRIVER_NAME);
      }
   }

   if (handle->if_id > -1)
   {
      return handle;
   }
   else
   {
      free(handle);
      return NULL;
   }
}

int os_eth_send(
   os_eth_handle_t   *handle,
   os_buf_t          *buf)
{
   int ret = -1;

   if (buf != NULL)
   {
      /* TODO: remove tot_len from os_buff */
      buf->tot_len = buf->len;

      if (interface[handle->if_id]->linkoutput != NULL)
      {
         interface[handle->if_id]->linkoutput(interface[handle->if_id], buf);
         ret = buf->len;
      }
   }
   return ret;
}
