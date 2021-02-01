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
 * @brief UDP communications
 */

#ifdef UNIT_TEST
#define pnal_udp_close    mock_pnal_udp_close
#define pnal_udp_open     mock_pnal_udp_open
#define pnal_udp_recvfrom mock_pnal_udp_recvfrom
#define pnal_udp_sendto   mock_pnal_udp_sendto
#endif

#include <string.h>
#include "pf_includes.h"

int pf_udp_open (pnet_t * net, pnal_ipport_t port)
{
   return pnal_udp_open (PNAL_IPADDR_ANY, port);
}

int pf_udp_sendto (
   pnet_t * net,
   uint32_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size)
{
   int sent_len = 0;

   sent_len = pnal_udp_sendto (id, dst_addr, dst_port, data, size);

   if (sent_len != size)
   {
      LOG_ERROR (
         PNET_LOG,
         "UDP(%d): Failed to send %u UDP bytes payload on the socket.\n",
         __LINE__,
         size);
   }

   return sent_len;
}

int pf_udp_recvfrom (
   pnet_t * net,
   uint32_t id,
   pnal_ipaddr_t * src_addr,
   pnal_ipport_t * src_port,
   uint8_t * data,
   int size)
{
   return pnal_udp_recvfrom (id, src_addr, src_port, data, size);
}

void pf_udp_close (pnet_t * net, uint32_t id)
{
   pnal_udp_close (id);
}
