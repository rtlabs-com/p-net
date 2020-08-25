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
 * @brief Update interface statistics for UDP communications
 */

#ifdef UNIT_TEST
#define os_udp_close mock_os_udp_close
#define os_udp_open mock_os_udp_open
#define os_udp_recvfrom mock_os_udp_recvfrom
#define os_udp_sendto mock_os_udp_sendto
#endif

#include <string.h>
#include "pf_includes.h"


int pf_udp_open(
   pnet_t                  *net,
   os_ipport_t             port)
{
   return os_udp_open(OS_IPADDR_ANY, port);
}

int pf_udp_sendto(
   pnet_t                  *net,
   uint32_t                id,
   os_ipaddr_t             dst_addr,
   os_ipport_t             dst_port,
   const uint8_t           *data,
   int                     size)
{
   int                     sent_len = 0;

   sent_len = os_udp_sendto(id, dst_addr, dst_port, data, size);

   if (sent_len != size)
   {
      net->interface_statistics.if_out_errors++;
      LOG_ERROR(PNET_LOG, "UDP(%d): Failed to send %u UDP bytes payload on the socket.\n", __LINE__, size);
   }
   else
   {
      net->interface_statistics.if_out_octets += sent_len;
   }

   return sent_len;
}

int pf_udp_recvfrom(
   pnet_t                  *net,
   uint32_t                id,
   os_ipaddr_t             *src_addr,
   os_ipport_t             *src_port,
   uint8_t                 *data,
   int                     size)
{
   int                     input_len = 0;

   input_len = os_udp_recvfrom(id, src_addr, src_port, data, size);

   net->interface_statistics.if_in_octets += input_len;

   return input_len;
}

void pf_udp_close(
   pnet_t                  *net,
   uint32_t                id)
{
   os_udp_close(id);
}
