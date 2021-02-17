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
 * @brief Map incoming cyclic Profinet data Ethernet frames to frame handlers
 *
 * The frame id map is used to quickly find the function responsible for
 * handling a frame with a specific frame id.
 * Clients may add or remove entries on the fly, but there is no locking of the
 * table. ToDo: This may be a problem in the future. Note that frames may arrive
 * at any time.
 */

#ifdef UNIT_TEST
#define pnal_eth_init mock_pnal_eth_init
#define pnal_eth_send mock_pnal_eth_send
#define pnal_get_macaddress mock_pnal_get_macaddress
#endif

#include <string.h>
#include "pf_includes.h"

static int pf_eth_init_netif (
   pnet_t * net,
   const char * netif_name,
   pnal_ethertype_t eth_receive_type,
   pf_netif_t * netif)
{
   pnal_ethaddr_t pnal_mac_addr;

   snprintf (netif->name, sizeof (netif->name), "%s", netif_name);

   netif->handle =
      pnal_eth_init (netif->name, eth_receive_type, pf_eth_recv, (void *)net);

   if (netif->handle == NULL)
   {
      LOG_ERROR (PNET_LOG, "Failed to init \"%s\"\n", netif_name);
      return -1;
   }

   if (pnal_get_macaddress (netif_name, &pnal_mac_addr) != 0)
   {
      LOG_ERROR (PNET_LOG, "Failed read mac address on \"%s\"\n", netif_name);
      return -1;
   }

   memcpy (
      netif->mac_address.addr,
      pnal_mac_addr.addr,
      sizeof (netif->mac_address.addr));

   return 0;
}

int pf_eth_init (pnet_t * net, const pnet_cfg_t * p_cfg)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data;
   pnal_ethertype_t main_port_receive_type;
#if PNET_NUMBER_OF_PHYSICAL_PORTS > 1
   const pnet_port_cfg_t * p_port_cfg;
#endif

   memset (net->eth_id_map, 0, sizeof (net->eth_id_map));

#if (PNET_NUMBER_OF_PHYSICAL_PORTS == 1)
   main_port_receive_type = PNAL_ETHTYPE_ALL;
#else
   main_port_receive_type = PNAL_ETHTYPE_PROFINET;
#endif

   /* Init management port */
   if (
      pf_eth_init_netif (
         net,
         p_cfg->if_cfg.main_netif_name,
         main_port_receive_type,
         &net->pf_interface.main_port) != 0)
   {
      return -1;
   }

   /* Init physical ports */
   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);

#if PNET_NUMBER_OF_PHYSICAL_PORTS > 1
      p_port_cfg = pf_port_get_config (net, port);

      if (
         pf_eth_init_netif (
            net,
            p_port_cfg->netif_name,
            PNAL_ETHTYPE_LLDP,
            &p_port_data->netif) != 0)
      {
         return -1;
      }
#else
      /* In single port configuration the managed port is also physical port 1
       */
      p_port_data->netif = net->pf_interface.main_port;
#endif

      port = pf_port_get_next (&port_iterator);
   }

   return 0;
}

int pf_eth_send (pnet_t * net, pnal_eth_handle_t * handle, pnal_buf_t * buf)
{
   int sent_len = 0;

   sent_len = pnal_eth_send (handle, buf);
   if (sent_len <= 0)
   {
      LOG_ERROR (PF_ETH_LOG, "ETH(%d): Error from pnal_eth_send\n", __LINE__);
   }
   else
   {
   }
   return sent_len;
}

int pf_eth_recv (pnal_eth_handle_t * eth_handle, void * arg, pnal_buf_t * p_buf)
{
   int ret = 0; /* Means: "Not handled" */
   uint16_t eth_type_pos = 2 * sizeof (pnet_ethaddr_t);
   uint16_t eth_type = 0;
   uint16_t frame_id = 0;
   uint16_t frame_pos = 0;
   const uint16_t * p_data = NULL;
   uint16_t ix = 0;
   pnet_t * net = (pnet_t *)arg;

#ifndef UNIT_TEST
   /* Reject unicast packets with destination not matching
    * mac address of management interface.
    */
   if (
      (((uint8_t *)p_buf->payload)[0] != 1) &&
      (memcmp (p_buf->payload, net->pf_interface.main_port.mac.addr, 6) != 0))
   {
      return 0;
   }
#endif

   /* Skip ALL VLAN tags */
   p_data = (uint16_t *)(&((uint8_t *)p_buf->payload)[eth_type_pos]);
   eth_type = ntohs (p_data[0]);
   while (eth_type == PNAL_ETHTYPE_VLAN)
   {
      eth_type_pos += 4; /* Sizeof VLAN tag */

      p_data = (uint16_t *)(&((uint8_t *)p_buf->payload)[eth_type_pos]);
      eth_type = ntohs (p_data[0]);
   }

   frame_pos = eth_type_pos + sizeof (uint16_t);

   switch (eth_type)
   {
   case PNAL_ETHTYPE_PROFINET:
      p_data = (uint16_t *)(&((uint8_t *)p_buf->payload)[frame_pos]);
      frame_id = ntohs (p_data[0]);

      /* Find the associated frame handler */
      ix = 0;
      while ((ix < NELEMENTS (net->eth_id_map)) &&
             ((net->eth_id_map[ix].in_use == false) ||
              (net->eth_id_map[ix].frame_id != frame_id)))
      {
         ix++;
      }
      if (ix < NELEMENTS (net->eth_id_map))
      {
         /* Call the frame handler */
         ret = net->eth_id_map[ix].frame_handler (
            net,
            frame_id,
            p_buf, /* This cannot be NULL, as seen above */
            frame_pos,
            net->eth_id_map[ix].p_arg);
      }
      break;
   case PNAL_ETHTYPE_LLDP:
      ret = pf_lldp_recv (net, eth_handle, p_buf, frame_pos);
      break;
   case PNAL_ETHTYPE_IP:
      /* IP-packets (UDP) are also received via the UDP sockets. Do not count
       * statistics here. */
      ret = 0;
      break;
   default:
      /* Unknown packet. */

      ret = 0;
      break;
   }

   return ret;
}

void pf_eth_frame_id_map_add (
   pnet_t * net,
   uint16_t frame_id,
   pf_eth_frame_handler_t frame_handler,
   void * p_arg)
{
   uint16_t ix = 0;

   while ((ix < NELEMENTS (net->eth_id_map)) &&
          (net->eth_id_map[ix].in_use == true))
   {
      ix++;
   }

   if (ix < NELEMENTS (net->eth_id_map))
   {
      LOG_DEBUG (
         PF_ETH_LOG,
         "ETH(%d): Add FrameIds %#x at index %u\n",
         __LINE__,
         (unsigned)frame_id,
         (unsigned)ix);
      net->eth_id_map[ix].frame_id = frame_id;
      net->eth_id_map[ix].frame_handler = frame_handler;
      net->eth_id_map[ix].p_arg = p_arg;
      net->eth_id_map[ix].in_use = true;
   }
   else
   {
      LOG_ERROR (PF_ETH_LOG, "ETH(%d): No more room for FrameIds\n", __LINE__);
   }
}

void pf_eth_frame_id_map_remove (pnet_t * net, uint16_t frame_id)
{
   uint16_t ix = 0;

   while ((ix < NELEMENTS (net->eth_id_map)) &&
          ((net->eth_id_map[ix].in_use == false) ||
           (net->eth_id_map[ix].frame_id != frame_id)))
   {
      ix++;
   }

   if (ix < NELEMENTS (net->eth_id_map))
   {
      net->eth_id_map[ix].in_use = false;
      LOG_DEBUG (
         PF_ETH_LOG,
         "ETH(%d): Free room for FrameIds %#x at index %u\n",
         __LINE__,
         (unsigned)frame_id,
         (unsigned)ix);
   }
}
