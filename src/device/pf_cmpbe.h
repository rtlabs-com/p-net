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

#ifndef PF_CMPBE_H
#define PF_CMPBE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Show CMPBE instance information about an AR.
 * @param p_ar             In:   The AR instance.
 */
void pf_cmpbe_show (const pf_ar_t * p_ar);

/**
 * Handle CMDEV events.
 * @param p_ar             InOut: The AR instance.
 * @param event            In:    The new CMDEV state. Use PNET_EVENT_xxx, not
 *                                PF_CMDEV_STATE_xxx
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmpbe_cmdev_state_ind (pf_ar_t * p_ar, pnet_event_values_t event);

/**
 * Handle a CControl request for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmpbe_cm_ccontrol_req (pnet_t * net, pf_ar_t * p_ar);

/**
 * Handle a CControl confirmation for a specific AR.
 * @param p_ar             InOut: The AR instance.
 * @param p_control_io     In:    The CControl block.
 * @param p_result         Out:   The result information. Not used.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmpbe_rm_ccontrol_cnf (
   pf_ar_t * p_ar,
   const pf_control_block_t * p_control_io,
   pnet_result_t * p_result);

/**
 * Handle a DControl indication for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_control_io     InOut: The DControl block.
 * @param p_result         Out:   The result information.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmpbe_rm_dcontrol_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_control_block_t * p_control_io,
   pnet_result_t * p_result);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMPBE_H */
