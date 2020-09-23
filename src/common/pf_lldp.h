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

/**
 * @file
 * @brief Periodical transmission outgoing LLDP frames
 *        Reception of incoming LLDP frames
 */

#ifndef PF_LLDP_H
#define PF_LLDP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the LLDP component.
 *
 * This function initializes the LLDP component and
 * sends the initial LLDP message.
 * @param net              InOut: The p-net stack instance
 */
void pf_lldp_init (pnet_t * net);

/**
 * Build and send an LLDP message.
 * @param net              InOut: The p-net stack instance
 */
void pf_lldp_send (pnet_t * net);

/**
 * Restart LLDP timer and optionally send LLDP frame.
 * @param net              InOut: The p-net stack instance
 * @param send             In: Send LLDP message
 */
void pf_lldp_restart (pnet_t * net, bool send);

/**
 * Receive an LLDP message.
 *
 * Parse LLDP tlv format and store selected information.
 * Trigger alarms if needed.
 * @param net              InOut: The p-net stack instance
 * @param p_frame_buf      In:    The Ethernet frame
 * @param offset           In:    The offset to start of LLDP data
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
int pf_lldp_recv (pnet_t * net, os_buf_t * p_frame_buf, uint16_t offset);

/************ Internal functions, made available for unit testing ************/

int pf_lldp_generate_alias_name (
   const char * port_id,
   const char * chassis_id,
   char * alias,
   uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* PF_LLDP_H */
