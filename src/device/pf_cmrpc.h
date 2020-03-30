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
 * @param net              InOut: The p-net stack instance
 */
void pf_cmrpc_init(
   pnet_t                  *net);

/**
 * Cleanup the CMRPC component.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmrpc_exit(
   pnet_t                  *net);

/**
 * Handle periodic RPC tasks.
 * Check for DCE RPC requests.
 * Check for DCE RPC confirmations.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmrpc_periodic(
   pnet_t                  *net);

/**
 * Find an AR by its AREP.
 * @param net              InOut: The p-net stack instance
 * @param arep             In:  The AREP.
 * @param pp_ar            Out: The AR (or NULL).
 * @return  0              if AR is found and valid.
 *         -1              if AREP is invalid.
 */
int pf_ar_find_by_arep(
   pnet_t                  *net,
   uint32_t                arep,
   pf_ar_t                 **pp_ar);

/**
 * Return pointer to specific AR.
 * @param net              InOut: The p-net stack instance
 * @param ix               In:   The AR array index.
 * @return                 The AR instance.
 */
pf_ar_t *pf_ar_find_by_index(
   pnet_t                  *net,
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
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param event            In:   The CMDEV event.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrpc_cmdev_state_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pnet_event_values_t     event);

/**
 * Send a DCE RPC request to the controller.
 * The only request handled is the APPL_RDY request.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrpc_rm_ccontrol_req(
   pnet_t                  *net,
   pf_ar_t                 *p_ar);

/**
 * Show AR and session information.
 *
 * Use level 0xFFFF to show everything.
 *
 * @param net              InOut: The p-net stack instance
 * @param level            In: Level of detail.
 */
void pf_cmrpc_show(
   pnet_t                  *net,
   unsigned                level);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMRPC_H */
