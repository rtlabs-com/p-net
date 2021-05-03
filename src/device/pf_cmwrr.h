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

#ifndef PF_CMWRR_H
#define PF_CMWRR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the CMWRR component.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmwrr_init (pnet_t * net, pf_ar_t * p_ar);

/**
 * Show the CMWRR part of the specified AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 */
void pf_cmwrr_show (const pnet_t * net, const pf_ar_t * p_ar);

/**
 * Handle CMDEV events.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance. Not used,
 * @param event            In:    The new CMDEV state. Use PNET_EVENT_xxx, not
 *                                PF_CMDEV_STATE_xxx
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmwrr_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event);

/**
 * Handle RPC write requests.
 *
 * If the state is correct, it will trigger pf_cmwrr_write()
 *
 * Triggers the \a pnet_write_ind() user callback for some values.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_write_request  In:    The write request block.
 * @param p_write_result   Out:   The write result block.
 * @param p_result         Out:   Detailed error information if returning != 0
 * @param p_req_buf        In:    The RPC request buffer.
 * @param data_length      In:    The length of the data to write.
 * @param p_req_pos        In:    Position in p_req_buf.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmwrr_rm_write_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_request,
   pf_iod_write_result_t * p_write_result,
   pnet_result_t * p_result,
   const uint8_t * p_req_buf,
   uint16_t data_length,
   uint16_t * p_req_pos);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMWRR_H */
