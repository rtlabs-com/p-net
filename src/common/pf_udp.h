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

#ifndef PF_UDP_H
#define PF_UDP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open an UDP socket. Listens to all IP addresses (PNAL_IPADDR_ANY).
 *
 * @param net              InOut: The p-net stack instance
 * @param port             In:    UDP port to listen to.
 * @return Socket ID, or -1 if an error occurred. Note that socket ID 0
 *         is valid.
 */
int pf_udp_open (pnet_t * net, pnal_ipport_t port);

/**
 * Send UDP data
 *
 * @param net              InOut: The p-net stack instance
 * @param id               In:    Socket ID
 * @param dst_addr         In:    Destination IP address
 * @param dst_port         In:    Destination UDP port
 * @param data             In:    Data to be sent
 * @param size             In:    Size of data
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int pf_udp_sendto (
   pnet_t * net,
   uint32_t id,
   pnal_ipaddr_t dst_addr,
   pnal_ipport_t dst_port,
   const uint8_t * data,
   int size);

/**
 * Receive UDP data.
 *
 * This is a nonblocking function, and it
 * returns 0 immediately if no data is available.
 *
 * @param net              InOut: The p-net stack instance
 * @param id               In:    Socket ID
 * @param dst_addr         Out:   Source IP address
 * @param dst_port         Out:   Source UDP port
 * @param data             Out:   Received data
 * @param size             In:    Size of buffer for received data
 * @return  The number of bytes received, or -1 if an error occurred.
 */
int pf_udp_recvfrom (
   pnet_t * net,
   uint32_t id,
   pnal_ipaddr_t * src_addr,
   pnal_ipport_t * src_port,
   uint8_t * data,
   int size);

/**
 * Close an UDP socket.
 *
 * @param net              InOut: The p-net stack instance
 * @param id               In:    Socket ID
 */
void pf_udp_close (pnet_t * net, uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* PF_UDP_H */
