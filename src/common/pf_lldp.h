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
 * @brief Send LLDP frame on startup.
 */

#ifndef PF_LLDP_H
#define PF_LLDP_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Initialize the LLDP component.
 *
 * This function initializes the LLDP component and
 * sends the initial LLDP message.
 */
void pf_lldp_init(
   int                     if_id);

/**
 * Send an LLDP message.
 */
void pf_lldp_send(void);

#ifdef __cplusplus
}
#endif

#endif /* PF_LLDP_H */
