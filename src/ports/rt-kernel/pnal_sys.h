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

#ifndef PNAL_SYS_H
#define PNAL_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/dev.h>
#include <sys/socket.h> /* For htonl etc */
#include <lwip/pbuf.h>

#define PNAL_BUF_MAX_SIZE PBUF_POOL_BUFSIZE

/* Re-use lwIP pbuf for rt-kernel */
typedef struct pbuf pnal_buf_t;

/* TODO: Integrate in standard drivers?
 * Local handling to enable the NIC drivers to support a RX hook
 */
#define IOCTL_NET_SET_RX_HOOK 0x601

typedef int (pnal_eth_sys_recv_callback_t) (
   struct netif * netif,
   void * arg,
   pnal_buf_t * p_buf);

int eth_ioctl (drv_t * drv, void * arg, int req, void * param);

#ifdef __cplusplus
}
#endif

#endif /* PNAL_SYS_H */
