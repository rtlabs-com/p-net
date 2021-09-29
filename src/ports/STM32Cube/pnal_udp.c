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

#include "pnal.h"
#include "osal_log.h"

#include <lwip/sockets.h>
#include <string.h>

int pnal_udp_open (pnal_ipaddr_t addr, pnal_ipport_t port)
{
   struct sockaddr_in local;
   int id;
   const int enable = 1;

   id = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if (id == -1)
   {
      return -1;
   }

   if (setsockopt (id, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof (enable)) != 0)
   {
      goto error;
   }

   /* set IP and port number */
   local = (struct sockaddr_in){
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl (addr),
      .sin_port = htons (port),
   };

   if (bind (id, (struct sockaddr *)&local, sizeof (local)) != 0)
   {
      goto error;
   }

   return id;

error:
   close (id);
   return -1;
}

int pnal_udp_sendto (
   uint32_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size)
{
   struct sockaddr_in remote;
   int len;

   remote = (struct sockaddr_in){
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl (dst_addr),
      .sin_port = htons (dst_port),
   };
   len =
      sendto (id, data, size, 0, (struct sockaddr *)&remote, sizeof (remote));

   return len;
}

int pnal_udp_recvfrom (
   uint32_t id,
   pnal_ipaddr_t * src_addr,
   pnal_ipport_t * src_port,
   uint8_t * data,
   int size)
{
   struct sockaddr_in remote;
   socklen_t addr_len = sizeof (remote);
   int len;

   memset (&remote, 0, sizeof (remote));
   len = recvfrom (
      id,
      data,
      size,
      MSG_DONTWAIT,
      (struct sockaddr *)&remote,
      &addr_len);
   if (len > 0)
   {
      *src_addr = ntohl (remote.sin_addr.s_addr);
      *src_port = ntohs (remote.sin_port);
   }

   return len;
}

void pnal_udp_close (uint32_t id)
{
   close (id);
}
