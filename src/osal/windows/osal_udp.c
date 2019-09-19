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
#include "log.h"

int os_udp_socket(void)
{
   int ret = -1;
   return ret;
}

int os_udp_open(os_ipaddr_t addr, os_ipport_t port)
{
   int ret = -1;
   return ret;
}

int os_udp_sendto(uint32_t id,
      os_ipaddr_t dst_addr,
      os_ipport_t dst_port,
      const uint8_t * data,
      int size)
{
   int  len = -1;
   return len;
}

int os_udp_recvfrom(uint32_t id,
      os_ipaddr_t *dst_addr,
      os_ipport_t *dst_port,
      uint8_t * data,
      int size)
{
   int  len = 0;
   return len;
}

void os_udp_close(uint32_t id)
{
}
