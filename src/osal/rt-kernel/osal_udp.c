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

#include <string.h>
#include "pf_includes.h"

int os_udp_socket(void)
{
   return socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

int os_udp_open(os_ipaddr_t addr, os_ipport_t port)
{
   int ret = -1;
   struct sockaddr_in local;
   int id;

   id = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   if(id < 0)
   {
      ret = id;
   }
   else
   {
      /* set IP and port number */
      local = (struct sockaddr_in) {
         .sin_family = AF_INET,
         .sin_addr.s_addr = htonl(addr),
         .sin_port = htons(port),
      };
      ret = bind(id, (struct sockaddr *)&local, sizeof(local));
      if(ret != 0)
      {
         close(id);
      }
      else
      {
         ret = id;
      }
   }

   return ret;
}

int os_udp_sendto(uint32_t id,
      os_ipaddr_t dst_addr,
      os_ipport_t dst_port,
      const uint8_t * data,
      int size)
{
   struct sockaddr_in remote;
   int  len;

   remote = (struct sockaddr_in)
   {
      .sin_family = AF_INET,
      .sin_addr.s_addr = htonl(dst_addr),
      .sin_port = htons(dst_port),
   };
   len = sendto(id, data, size, 0, (struct sockaddr *)&remote, sizeof(remote));

   return len;
}

int os_udp_recvfrom(uint32_t id,
      os_ipaddr_t *dst_addr,
      os_ipport_t *dst_port,
      uint8_t * data,
      int size)
{
   struct sockaddr_in remote;
   socklen_t addr_len = sizeof(remote);
   int  len;

   memset(&remote, 0, sizeof(remote));
   len = recvfrom(id, data, size, MSG_DONTWAIT, (struct sockaddr *)&remote, &addr_len);
   if(len > 0)
   {
      *dst_addr = ntohl(remote.sin_addr.s_addr);
      *dst_port = ntohs(remote.sin_port);
   }

   return len;
}

void os_udp_close(uint32_t id)
{
   close(id);
}
