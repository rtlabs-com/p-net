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

/**
 * Get status of network interface
 *
 * \code
 * eth_status_t status;
 * int error = eth_ioctl (drv, netif, IOCTL_NET_GET_STATUS, &status);
 * \endcode
 */
#define IOCTL_NET_GET_STATUS 0x602

/**
 * Status of Ethernet link
 */
typedef struct eth_status
{
   /** Contents of PHY register ANAR
    *
    * The ANAR (Auto-Negotiation Advertisement Register) contains a bit
    * for each link type the physical layer supports, as advertised to
    * link partner when establishing a link:
    * - Bit 8: 100BASE-TX full duplex,
    * - Bit 7: 100BASE-TX,
    * - Bit 6: 10BASE-T full duplex,
    * - Bit 5: 10BASE-T.
    */
   uint16_t anar;

   /** Is auto-negotiation process supported by physical layer? */
   bool is_autonegotiation_supported;

   /** Is auto-negotiation process enabled for physical layer? */
   bool is_autonegotiation_enabled;

   /**
    * Is link operational?
    *
    * True if link is up and network interface is administratively up.
    * False if any of those are down.
    */
   bool is_operational;

   /**
    * Link state
    *
    * Bit pattern of the following:
    * - PHY_LINK_OK is set if link is up,
    * - PHY_LINK_10MBIT, PHY_LINK_100MBIT or PHY_LINK_1000MBIT is the speed,
    * - PHY_LINK_FULL_DUPLEX is set if link is full-duplex and cleared if
    *   it is half-duplex.
    */
   uint8_t state;
} eth_status_t;

typedef int (pnal_eth_sys_recv_callback_t) (
   struct netif * netif,
   void * arg,
   pnal_buf_t * p_buf);

int eth_ioctl (drv_t * drv, void * arg, int req, void * param);

#ifdef __cplusplus
}
#endif

#endif /* PNAL_SYS_H */
