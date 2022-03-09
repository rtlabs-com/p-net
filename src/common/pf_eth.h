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

#ifndef PF_ETH_H
#define PF_ETH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ETH component and network interfaces according to configuration
 * If device is configured with one network interface
 * (num_physical_ports==1), the management port and physical port 1
 * refers to same network interface. In a multi port configuration, the
 * management port and physical ports are different network interfaces.
 * @param net              InOut: The p-net stack instance
 * @param p_cfg            In:    Configuration
 * @return   0 on success
 *          -1 on error
 */
int pf_eth_init (pnet_t * net, const pnet_cfg_t * p_cfg);

/**
 * Send raw Ethernet frame on a specific physical port.
 *
 * Not for use with management port.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @param buf              In:    Buffer with data to be sent
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int pf_eth_send_on_physical_port (
   pnet_t * net,
   int loc_port_num,
   pnal_buf_t * buf);

/**
 * Send raw Ethernet frame on management port.
 *
 * @param net              InOut: The p-net stack instance
 * @param buf              In:    Buffer with data to be sent
 * @return  The number of bytes sent, or -1 if an error occurred.
 */
int pf_eth_send_on_management_port (pnet_t * net, pnal_buf_t * buf);

/**
 * Add a frame_id entry to the frame id filter map.
 *
 * This function adds an entry to the frame id table.
 * This table is used to map incoming frames to the right handler functions,
 * based on the frame id.
 *
 * Used only for incoming frames with Ethtype = Profinet.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame ID to look for.
 * @param frame_handler    In:   The handler function to call.
 * @param p_arg            In:   Argument to handler function.
 */
void pf_eth_frame_id_map_add (
   pnet_t * net,
   uint16_t frame_id,
   pf_eth_frame_handler_t frame_handler,
   void * p_arg);

/**
 * Remove an entry from the frame id filter map (if it exists).
 *
 * @param net              InOut: The p-net stack instance
 * @param frame_id         In:   The frame ID to remove.
 */
void pf_eth_frame_id_map_remove (pnet_t * net, uint16_t frame_id);

/**
 * Inspect and possibly handle a raw Ethernet frame
 *
 * Find the packet type. If it is for Profinet then send it to the right
 * handler, depending on the frame_id within the packet. The frame_id is located
 * right after the packet type. Take care of handling the VLAN tag.
 *
 * Note also that this function is a callback and will be passed as an argument
 * to pnal_eth_init().
 *
 * @param eth_handle       InOut: Network interface the frame was received on.
 * @param arg              InOut: User argument, will be converted to pnet_t
 * @param p_buf            InOut: The Ethernet frame. Might be freed.
 * @return  0  If the frame was NOT handled by this function.
 *          1  If the frame was handled and the buffer freed.
 */
int pf_eth_recv (pnal_eth_handle_t * eth_handle, void * arg, pnal_buf_t * p_buf);

#ifdef __cplusplus
}
#endif

#endif /* PF_DCPMCS_H */
