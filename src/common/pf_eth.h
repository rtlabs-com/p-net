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
extern "C"
{
#endif

/**
 * This is the prototype for the frame handler.
 *
 * @param frame_id         In:   The frame ID.
 * @param p_buf            In:   The Ethernet frame.
 * @param frame_id_pos     In:   The position of the frame ID.
 * @param p_arg            In:   User-defined argument.
 */
typedef int (*pf_eth_frame_handler_t)(
   uint16_t                frame_id,
   os_buf_t                *p_buf,
   uint16_t                frame_id_pos,
   void                    *p_arg);

/**
 * Initialize the ETH component.
 * @return  0  if the ETH component could be initialized.
 *          -1 if an error occurred.
 */
int pf_eth_init(void);

/**
 * Add a frame_id entry to the frame id filter map.
 *
 * This function adds an entry to the frame id table.
 * This table is used to map incoming frames to the right handler functions,
 * based on the frame id.
 *
 * @param frame_id         In:   The frame ID to look for.
 * @param frame_handler    In:   The handler function to call.
 * @param p_arg            In:   Argument to handler function.
 */
void pf_eth_frame_id_map_add(
   uint16_t                frame_id,
   pf_eth_frame_handler_t  frame_handler,
   void                    *p_arg);

/**
 * Remove an entry from the frame id filter map.
 * @param frame_id         In:   The frame ID to remove.
 */
void pf_eth_frame_id_map_remove(
   uint16_t                frame_id);

/**
 * Inspect and possibly handle Ethernet frames:
 *
 * Find the packet type. If it is for Profinet then send it to the right handler,
 * depending on the frame_id within the packet.
 * The frame_id is located right after the packet type.
 * Take care of handling the VLAN tag!!
 *
 * @param p_buf            In:   The Ethernet frame.
 * @param len              In:   The length of the frame.
 * @return  0  If the frame was NOT handled by this function.
 *          1  If the frame was handled and the buffer freed.
 */
int pf_eth_recv(
   os_buf_t                *buf,
   size_t                  len);

#ifdef __cplusplus
}
#endif

#endif /* PF_DCPMCS_H */
