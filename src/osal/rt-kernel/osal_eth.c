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

#define MAX_NUMBER_OF_IF   1
#define NET_DRIVER_NAME    "/net"

static struct netif * interface[MAX_NUMBER_OF_IF];
static int nic_index = 0;

static int os_eth_recv(uint16_t type, struct pbuf *p, struct netif *inp);

int os_eth_init(const char * if_name)
{
   int id = -1;
   struct netif * found_if;
   drv_t * drv;

   if (nic_index < MAX_NUMBER_OF_IF)
   {
      drv = dev_find_driver (NET_DRIVER_NAME);
      if (drv != NULL)
      {
         eth_ioctl(drv, NULL, IOCTL_NET_SET_RX_HOOK, os_eth_recv);

         found_if = netif_find(if_name);
         if (found_if == NULL)
         {
            interface[nic_index] = netif_default;
         }
         else
         {
            interface[nic_index] = found_if;
         }
         id = nic_index++;
      }
      else
      {
         printf("Driver \"%s\" not found!\n", NET_DRIVER_NAME);
      }
   }

   return id;
}

int os_eth_send(uint32_t id, os_buf_t * buf)
{
   int ret = -1;

   if (buf != NULL)
   {
      /* TODO: remove tot_len from os_buff */
      buf->tot_len = buf->len;

      if (interface[id]->linkoutput != NULL)
      {
         interface[id]->linkoutput(interface[id], buf);
         ret = buf->len;
      }
   }
   return ret;
}

static int os_eth_recv(uint16_t type, struct pbuf *p, struct netif *inp)
{

   int handled = 0;

   switch (type)
   {
   case OS_ETHTYPE_PROFINET:
   case OS_ETHTYPE_LLDP:
   case OS_ETHTYPE_VLAN:
      handled = pf_eth_recv(p, p->len);
      break;
   case OS_ETHTYPE_ETHERCAT:
      rprintf("Handle ECAT type\n");
      handled = 1;
      break;

   default:
      break;
   }

   return handled;
}


