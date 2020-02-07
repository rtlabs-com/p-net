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

#include "options.h"
#include "osal.h"
#include "osal_sys.h"
#include "pf_includes.h"
#include "log.h"
#include "cc.h"
#include <stdlib.h>
#include <errno.h>
#include <net/if.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>

static int os_eth_recv(uint16_t type, os_buf_t * p)
{
   int handled = 0;

   switch (type)
   {
   case OS_ETHTYPE_PROFINET:
   case OS_ETHTYPE_LLDP:
      handled = pf_eth_recv(p, p->len);
      break;
   case OS_ETHTYPE_ETHERCAT:
      handled = 0;
      break;

   default:
      break;
   }

   return handled;
}

static void os_eth_task(void * arg)
{
   int * sock = (int *) arg;
   ssize_t readlen;

   os_buf_t * p = os_buf_alloc (1522);
   assert(p != NULL);

   while (1)
   {
      readlen = recv (*sock, p->payload, p->len, 0);
      if(readlen == -1)
         continue;
      p->len = readlen;
      if (os_eth_recv (OS_ETHTYPE_PROFINET, p) == 1)
      {
         fflush(stdout);
         p = os_buf_alloc (1522);
         assert(p != NULL);
      }
   }
}

int os_eth_init(const char * if_name)
{

   int * sock;
   int i;
   struct ifreq ifr;
   struct sockaddr_ll sll;
   int ifindex;
   struct timeval timeout;

   sock = calloc (1, sizeof(int));
   *sock = -1;

   *sock = socket(PF_PACKET, SOCK_RAW, htons(OS_ETHTYPE_PROFINET));

   timeout.tv_sec =  0;
   timeout.tv_usec = 1;
   setsockopt(*sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

   i = 1;
   setsockopt(*sock, SOL_SOCKET, SO_DONTROUTE, &i, sizeof(i));


   strcpy(ifr.ifr_name, if_name);
   ioctl(*sock, SIOCGIFINDEX, &ifr);

   ifindex = ifr.ifr_ifindex;
   strcpy(ifr.ifr_name, if_name);
   ifr.ifr_flags = 0;
   /* reset flags of NIC interface */
   ioctl(*sock, SIOCGIFFLAGS, &ifr);

   /* set flags of NIC interface, here promiscuous and broadcast */
   ifr.ifr_flags = ifr.ifr_flags | IFF_PROMISC | IFF_BROADCAST;
   ioctl(*sock, SIOCSIFFLAGS, &ifr);

   /* bind socket to protocol, in this case RAW EtherCAT */
   sll.sll_family = AF_PACKET;
   sll.sll_ifindex = ifindex;
   sll.sll_protocol = htons(OS_ETHTYPE_PROFINET);
   bind(*sock, (struct sockaddr *)&sll, sizeof(sll));

   if (*sock > -1)
   {
      os_thread_create ("os_eth_task", 10,
              4096, os_eth_task, sock);
   }

   return *sock;
}

int os_eth_send(uint32_t id, os_buf_t * buf)
{
   int ret = send (id, buf->payload, buf->len, 0);

   if (ret<0)
      printf("os_eth_send sent length: %d errno %d\n", ret, errno);

   fflush(stdout);

   return ret;
}
