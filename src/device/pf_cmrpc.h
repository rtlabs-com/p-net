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

#ifndef PF_CMRPC_H
#define PF_CMRPC_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ================================================
 *       Local primitives
 */

/**
 * Initialize the CMRPC component.
 * @param if_id            The interface id to os_eth_send().
 */
void pf_cmrpc_init(
   int                     if_id);

/**
 * Cleanup the CMRPC component.
 */
void pf_cmrpc_exit(void);

/**
 * Handle periodic RPC tasks.
 * Check for DCE RPC requests.
 * Check for DCE RPC confirmations.
 */
void pf_cmrpc_periodic(void);

/**
 * Find an AR by its AREP.
 * @param arep             In:  The AREP.
 * @param pp_ar            Out: The AR (or NULL).
 * @return  0              if AR is found and valid.
 *         -1              if AREP is invalid.
 */
int pf_ar_find_by_arep(
   uint32_t                arep,
   pf_ar_t                 **pp_ar);

/**
 * Return pointer to specific AR.
 * @param ix               In:   The AR array index.
 * @return                 The AR instance.
 */
pf_ar_t *pf_ar_find_by_index(
   uint16_t                ix);

/**
 * Insert detailed error information into the result structure.
 * @param p_stat           Out:  The result structure.
 * @param code             In:   The error_code.
 * @param decode           In:   The error_decode.
 * @param code_1           In:   The error_code_1.
 * @param code_2           In:   The error_code_2.
 */
void pf_set_error(
   pnet_result_t           *p_stat,
   uint8_t                 code,
   uint8_t                 decode,
   uint8_t                 code_1,
   uint8_t                 code_2);

/**
 * Handle CMDEV events.
 * @param p_ar             In:   The AR instance.
 * @param event            In:   The CMDEV event.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrpc_cmdev_state_ind(
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event);

/**
 * Send a DCE RPC request to the controller.
 * The only request handled is the APPL_RDY request.
 * @param p_ar             In:   The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrpc_rm_ccontrol_req(
   pf_ar_t                 *p_ar);

/**
 * Show AR and session information.
 * @param level            In: Level of detail.
 */
void pf_cmrpc_show(
   unsigned                level);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMRPC_H */
