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

#ifndef PF_CMIO_H
#define PF_CMIO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Show CMIO information about an AR.
 * @param p_ar             In:   The AR instance.
 */
void pf_cmio_show (const pf_ar_t * p_ar);

/**
 * Handle CMDEV events.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param event            In:    The new CMDEV state. Use PNET_EVENT_xxx, not
 *                                PF_CMDEV_STATE_xxx
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmio_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event);

/**
 * Handle CPM start/stop events of a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR instance.
 * @param start            In:    Start or Stop information. True if CPM is
 *                                starting.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmio_cpm_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint16_t crep,
   bool start);

/**
 * Handle CPM new data events of a specific AR.
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR instance.
 * @param new_data         In:    true => Data, false => NoData.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmio_cpm_new_data_ind (pf_ar_t * p_ar, uint16_t crep, bool new_data);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMIO_H */
