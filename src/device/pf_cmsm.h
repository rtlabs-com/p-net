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

#ifndef PF_CMSM_H
#define PF_CMSM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the CMSM-specific parts of an AR
 * @param p_ar             In:   The AR instance (not NULL)
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsm_activate (pf_ar_t * p_ar);

/**
 * Handle incoming CMDEV events.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance, or NULL.
 * @param event            In:    The new CMDEV state. Use PNET_EVENT_xxx, not
 *                                PF_CMDEV_STATE_xxx
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsm_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event);

/**
 * Handle incoming RPC read requests, by restarting the timer if running.
 * Only if index is PF_IDX_DEV_CONN_MON_TRIGGER
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance, or NULL.
 * @param p_read_request   In:    The read request.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsm_rm_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_iod_read_request_t * p_read_request);

/**
 * Handle incoming RPC read requests, by restarting the timer if running.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance, or NULL.
 * @param p_read_request   In:    The read request. not used.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsm_cm_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_request);

/**
 * Handle incoming RPC write requests, by restarting the timer if running.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance, or NULL.
 * @param p_write_request  In:    The write request. Not used.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsm_cm_write_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_request);

/**
 * Print information about CMSM for the specified AR.
 * @param p_ar             In:   The AR instance, or NULL.
 */
void pf_cmsm_show (const pf_ar_t * p_ar);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMSM_H */
