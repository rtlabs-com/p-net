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

#if PNET_MAX_PORT < 1
#error "PNET_MAX_PORT needs to be at least 1"
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

   pf_port_init_iterator_over_ports (net, &port_iterator);
   port = pf_port_get_next (&port_iterator);

   while (port != 0)
   {
      p_port_data = pf_port_get_state (net, port);
      p_port_data->port_num = port;
      port = pf_port_get_next (&port_iterator);
   }
}

void pf_port_get_list_of_ports (
   pnet_t * net,
   uint16_t max_port,
   pf_lldp_port_list_t * p_list)
{
   uint16_t port;
   uint16_t index = 0;
   uint8_t port_bit_mask = 0;
   memset (p_list, 0, sizeof (*p_list));

   for (port = PNET_PORT_1; port <= max_port; port++)
   {
      /* See documentation of pf_lldp_port_list_t for details*/
      index = (port - 1) / 8;
      CC_ASSERT (index < sizeof (*p_list));
      port_bit_mask = BIT (7 - ((port - 1) % 8));
      p_list->ports[index] |= port_bit_mask;
   }
}

void pf_port_init_iterator_over_ports (
   pnet_t * net,
   pf_port_iterator_t * p_iterator)
{
   p_iterator->next_port = 1;
}

int pf_port_get_next (pf_port_iterator_t * p_iterator)
{
   int ret = p_iterator->next_port;

   if (p_iterator->next_port == PNET_MAX_PORT || p_iterator->next_port == 0)
   {
      p_iterator->next_port = 0;
   }
   else
   {
      p_iterator->next_port += 1;
   }

   return ret;
}

pf_port_t * pf_port_get_state (pnet_t * net, int loc_port_num)
{
   CC_ASSERT (loc_port_num > 0 && loc_port_num <= PNET_MAX_PORT);
   return &net->port[loc_port_num - 1];
}

const pnet_port_cfg_t * pf_port_get_config (pnet_t * net, int loc_port_num)
{
   CC_ASSERT (loc_port_num > 0 && loc_port_num <= PNET_MAX_PORT);
   return &net->fspm_cfg.if_cfg.ports[loc_port_num - 1];
}
