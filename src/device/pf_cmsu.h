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

#ifndef PF_CMSU_H
#define PF_CMSU_H

#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Initialize the CMSU component.
 */
void pf_cmsu_init(
   pnet_t                  *net);

/**
 * Handle CMDEV events.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param event            In:   The CMDEV event.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_cmdev_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event);

/**
 * Start all state machines for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_stat           Out:  The result information.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_start_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_result_t           *p_stat);

/**
 * Handle CPM error indications for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param err_cls          In:   ERR_CLS.
 * @param err_code         In:   ERR_CODE.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_cpm_error_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_cls,
   uint8_t                 err_code);

/**
 * Handle PPM error indications for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param err_cls          In:   ERR_CLS.
 * @param err_code         In:   ERR_CODE.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_ppm_error_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_cls,
   uint8_t                 err_code);

/**
 * Handle alarm error indications for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param err_cls          In:   ERR_CLS.
 * @param err_code         In:   ERR_CODE
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_alarm_error_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_cls,
   uint8_t                 err_code);

/* Not used */
/**
 * Handle DMC error indications for a specific AR.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param err_code         In:   ERR_CODE
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmsu_cmdmc_error_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint8_t                 err_cls,
   uint8_t                 err_code);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMSU_H */
