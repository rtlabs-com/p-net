/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "pf_includes.h"

#include <string.h>

#if PNET_MAX_PHYSICAL_PORTS < 1
#error "PNET_MAX_PHYSICAL_PORTS needs to be at least 1"
#endif

#if PNET_MAX_SLOTS < 1
#error "At least one slot must be available for DAP. Increase PNET_MAX_SLOTS."
#endif

#if PNET_MAX_SUBSLOTS < (2 + PNET_MAX_PHYSICAL_PORTS)
#error                                                                         \
   "DAP requires 2 + PNET_MAX_PHYSICAL_PORTS subslots. Increase PNET_MAX_SUBSLOTS."
#endif

/**
 * @file
 * @brief Manage runtime data for multiple ports
 */

void pf_port_init (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data = NULL;

   net->pf_interface.port_mutex = os_mutex_create();

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);

   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);

      p_port_data->port_num = port;
      snprintf (
         p_port_data->port_name,
         sizeof (p_port_data->port_name),
         "port-%03u",
         (uint8_t)port); /* Cast to avoid format-truncation */

      port = pf_port_get_next (&port_iterator);
   }
}

void pf_port_main_interface_init (pnet_t * net)
{
   /* Format of LLDP frames */
   net->pf_interface.name_of_device_mode.mode =
      PF_LLDP_NAME_OF_DEVICE_MODE_STANDARD;
   net->pf_interface.name_of_device_mode.active = false;

   /* Ethernet link monitor */
   pf_port_init_iterator_over_ports (
      net,
      &net->pf_interface.link_monitor_iterator);
   pf_scheduler_init_handle (
      &net->pf_interface.link_monitor_timeout,
      "link_monitor");
   pf_pdport_start_linkmonitor (net);
}

uint8_t pf_port_get_number_of_ports (const pnet_t * net)
{
   return net->fspm_cfg.num_physical_ports;
}

void pf_port_get_list_of_ports (const pnet_t * net, pf_lldp_port_list_t * p_list)
{
   uint16_t port;
   uint16_t index = 0;
   uint8_t port_bit_mask = 0;
   memset (p_list, 0, sizeof (*p_list));

   for (port = PNET_PORT_1; port <= net->fspm_cfg.num_physical_ports; port++)
   {
      /* See documentation of pf_lldp_port_list_t for details*/
      index = (port - 1) / 8;
      CC_ASSERT (index < sizeof (*p_list));
      port_bit_mask = BIT (7 - ((port - 1) % 8));
      p_list->ports[index] |= port_bit_mask;
   }
}

void pf_port_init_iterator_over_ports (
   const pnet_t * net,
   pf_port_iterator_t * p_iterator)
{
   CC_ASSERT (net != NULL);
   p_iterator->next_port = 1;
   p_iterator->number_of_ports = net->fspm_cfg.num_physical_ports;
}

int pf_port_get_next (pf_port_iterator_t * p_iterator)
{
   int ret = p_iterator->next_port;

   if (
      p_iterator->next_port == p_iterator->number_of_ports ||
      p_iterator->next_port == 0)
   {
      p_iterator->next_port = 0;
   }
   else if (p_iterator->next_port > p_iterator->number_of_ports)
   {
      ret = 0;
      p_iterator->next_port = 0;
   }
   else
   {
      p_iterator->next_port += 1;
   }

   return ret;
}

int pf_port_get_next_repeat_cyclic (pf_port_iterator_t * p_iterator)
{
   int ret;

   if (p_iterator->next_port > p_iterator->number_of_ports)
   {
      p_iterator->next_port = 1;
   }

   ret = p_iterator->next_port;

   if (p_iterator->next_port == p_iterator->number_of_ports)
   {
      p_iterator->next_port = 1;
   }
   else
   {
      p_iterator->next_port += 1;
   }

   return ret;
}

pf_port_t * pf_port_get_state (pnet_t * net, int loc_port_num)
{
   CC_ASSERT (pf_port_is_valid (net, loc_port_num));
   return &net->pf_interface.port[loc_port_num - 1];
}

const pnet_port_cfg_t * pf_port_get_config (pnet_t * net, int loc_port_num)
{
   CC_ASSERT (pf_port_is_valid (net, loc_port_num));
   return &net->fspm_cfg.if_cfg.physical_ports[loc_port_num - 1];
}

uint16_t pf_port_loc_port_num_to_dap_subslot (
   const pnet_t * net,
   int loc_port_num)
{
   CC_ASSERT (pf_port_is_valid (net, loc_port_num));
   return PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT + loc_port_num -
          PNET_PORT_1;
}

int pf_port_dap_subslot_to_local_port (const pnet_t * net, uint16_t subslot)
{
   int port = PNET_PORT_1 + subslot - PNET_SUBSLOT_DAP_INTERFACE_1_PORT_1_IDENT;

   if (pf_port_is_valid (net, port) == false)
   {
      port = 0;
   }

   return port;
}

bool pf_port_subslot_is_dap_port_id (const pnet_t * net, uint16_t subslot)
{
   int port = pf_port_dap_subslot_to_local_port (net, subslot);
   return (port != 0);
}

bool pf_port_is_valid (const pnet_t * net, int loc_port_num)
{
   CC_ASSERT (net != NULL);

   return (
      loc_port_num > 0 && loc_port_num <= net->fspm_cfg.num_physical_ports);
}

int pf_port_get_port_number (const pnet_t * net, pnal_eth_handle_t * eth_handle)
{
   int loc_port_num;

   for (loc_port_num = 1; loc_port_num <= net->fspm_cfg.num_physical_ports;
        loc_port_num++)
   {
      if (net->pf_interface.port[loc_port_num - 1].netif.handle == eth_handle)
      {
         return loc_port_num;
      }
   }

   return 0;
}

pf_mediatype_values_t pf_port_get_media_type (pnal_eth_mau_t mau_type)
{
   switch (mau_type)
   {
   case PNAL_ETH_MAU_RADIO:
      return PF_PD_MEDIATYPE_RADIO;
   case PNAL_ETH_MAU_COPPER_10BaseT:
   case PNAL_ETH_MAU_COPPER_100BaseTX_HALF_DUPLEX:
   case PNAL_ETH_MAU_COPPER_100BaseTX_FULL_DUPLEX:
   case PNAL_ETH_MAU_COPPER_1000BaseT_HALF_DUPLEX:
   case PNAL_ETH_MAU_COPPER_1000BaseT_FULL_DUPLEX:
      return PF_PD_MEDIATYPE_COPPER;
   case PNAL_ETH_MAU_FIBER_100BaseFX_HALF_DUPLEX:
   case PNAL_ETH_MAU_FIBER_100BaseFX_FULL_DUPLEX:
   case PNAL_ETH_MAU_FIBER_1000BaseX_HALF_DUPLEX:
   case PNAL_ETH_MAU_FIBER_1000BaseX_FULL_DUPLEX:
      return PF_PD_MEDIATYPE_FIBER;
   default:
      return PF_PD_MEDIATYPE_UNKNOWN;
   }
}

void pf_port_show (pnet_t * net)
{
   int port;
   pf_port_iterator_t port_iterator;
   pf_port_t * p_port_data;
   const pnet_port_cfg_t * p_port_config;

   printf ("\nPorts\n");

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);
   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);
      p_port_config = pf_port_get_config (net, port);

      printf ("Port %u\n", port);
      printf (" Port name               : \"%s\"\n", p_port_data->port_name);
      printf (" ETH name                : \"%s\"\n", p_port_config->netif_name);
      printf (
         " MAC                     : %02x:%02x:%02x:%02x:%02x:%02x\n",
         p_port_data->netif.mac_address.addr[0],
         p_port_data->netif.mac_address.addr[1],
         p_port_data->netif.mac_address.addr[2],
         p_port_data->netif.mac_address.addr[3],
         p_port_data->netif.mac_address.addr[4],
         p_port_data->netif.mac_address.addr[5]);
      printf (
         " Link is up?             : %u\n",
         p_port_data->netif.previous_is_link_up);
      printf (
         " Peer check active       : %u\n",
         p_port_data->pdport.check.active);
      printf (
         " Peer check station name : \"%s\"\n",
         p_port_data->pdport.check.peer.peer_station_name);
      printf (
         " Peer check port name    : \"%s\"\n",
         p_port_data->pdport.check.peer.peer_port_name);
      printf (
         " Adjust active           : %u\n",
         p_port_data->pdport.adjust.active);
      printf (
         " Do not send LLDP        : %u\n",
         p_port_data->pdport.adjust.peer_to_peer_boundary.peer_to_peer_boundary
            .do_not_send_LLDP_frames);
      printf (
         " Peer info received      : %u\n",
         p_port_data->lldp.is_peer_info_received);
      printf (
         " Peer chassis id         : Valid %u \"%s\"\n",
         p_port_data->lldp.peer_info.chassis_id.is_valid,
         p_port_data->lldp.peer_info.chassis_id.string);
      printf (
         " Peer port id            : Valid %u \"%s\"\n",
         p_port_data->lldp.peer_info.port_id.is_valid,
         p_port_data->lldp.peer_info.port_id.string);
      printf (
         " Peer port descr         : Valid %u \"%s\"\n",
         p_port_data->lldp.peer_info.port_description.is_valid,
         p_port_data->lldp.peer_info.port_description.string);
      printf (
         " Peer MAU type           : Valid %u %u\n",
         p_port_data->lldp.peer_info.phy_config.is_valid,
         p_port_data->lldp.peer_info.phy_config.operational_mau_type);

      port = pf_port_get_next (&port_iterator);
   }
}
