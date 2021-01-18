/*********************************************************************
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include "pnal.h"

int pnal_udp_open (pnal_ipaddr_t addr, pnal_ipport_t port)
{
   return 0;
}

int pnal_udp_sendto (uint32_t id,pnal_ipaddr_t dst_addr,pnal_ipport_t dst_port,const uint8_t * data,int size)
{
   return 0;
}

int pnal_udp_recvfrom (uint32_t id,pnal_ipaddr_t * src_addr,pnal_ipport_t * src_port,uint8_t * data,int size)
{
   return 0;
}

void pnal_udp_close (uint32_t id)
{
   
}
