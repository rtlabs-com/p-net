/*********************************************************************
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include <winsock2.h>
#include "pnal.h"
#include "options.h"
#include "osal_log.h"

int pnal_udp_open (pnal_ipaddr_t addr, pnal_ipport_t port)
{
   int ret = -1;
   struct sockaddr_in local;
   SOCKET id;

   id = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);

   if (id == INVALID_SOCKET)
   {
      ret = -EFAULT;
   }
   else
   {
      /* set IP and port number */
      local = (struct sockaddr_in){
         .sin_family = AF_INET,
         .sin_addr.s_addr = htonl (addr),
         .sin_port = htons (port),
         .sin_zero = {0},
      };
      ret = bind (id, (struct sockaddr *)&local, sizeof (local));
      if (ret != 0)
      {
         closesocket (id);
      }
      else
      {
         ret = id;
         LOG_WARNING (
            PF_PNAL_LOG,
            "PNAL(%d):  win32 Udp sockets cannot be converted to int\n",
            __LINE__);
      }
   }

   return ret;
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
      .sin_zero = {0},
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
   int addr_len = sizeof (remote);
   int len;

   memset (&remote, 0, sizeof (remote));
   len = recvfrom (
      id,
      data,
      size,
      0,
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
   closesocket(id);
}