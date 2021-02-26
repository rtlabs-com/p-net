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
extern "C" {
#endif

/* ================================================
 *       Local primitives
 */

/**
 * Initialize the CMRPC component.
 *
 * Opens the UDP socket for incoming RPC requests.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_cmrpc_init (pnet_t * net);

/**
 * Cleanup the CMRPC component.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmrpc_exit (pnet_t * net);

/**
 * Handle periodic RPC tasks.
 * Check for DCE RPC requests.
 * Check for DCE RPC confirmations.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmrpc_periodic (pnet_t * net);

/**
 * Find an AR by its AREP.
 * @param net              InOut: The p-net stack instance
 * @param arep             In:  The AREP.
 * @param pp_ar            Out: The AR (or NULL).
 * @return  0              if AR is found and valid.
 *         -1              if AREP is invalid.
 */
int pf_ar_find_by_arep (pnet_t * net, uint32_t arep, pf_ar_t ** pp_ar);

/**
 * Return pointer to specific AR.
 * @param net              InOut: The p-net stack instance
 * @param ix               In:   The AR array index.
 * @return                 The AR instance.
 */
pf_ar_t * pf_ar_find_by_index (pnet_t * net, uint16_t ix);

/**
 * Insert detailed error information into the result structure (of a session).
 *
 * If you need to set error information after some function has returned an
 * error (and possibly already has updated the error information), use
 * \a pf_set_error_if_not_already_set() instead.
 *
 * @param p_stat           Out:  The result structure.
 * @param code             In:   The error_code.
 * @param decode           In:   The error_decode.
 * @param code_1           In:   The error_code_1.
 * @param code_2           In:   The error_code_2.
 */
void pf_set_error (
   pnet_result_t * p_stat,
   uint8_t code,
   uint8_t decode,
   uint8_t code_1,
   uint8_t code_2);

/**
 * Insert detailed error information into the result structure (of a session).
 * Does not overwrite existing error information.
 *
 * This function is typically called after some other function has returned an
 * error and possibly have set an error code.
 * For other usecases, use pf_set_error() instead.
 *
 * @param p_stat           Out:  The result structure.
 * @param code             In:   The error_code.
 * @param decode           In:   The error_decode.
 * @param code_1           In:   The error_code_1.
 * @param code_2           In:   The error_code_2.
 */
void pf_set_error_if_not_already_set (
   pnet_result_t * p_stat,
   uint8_t code,
   uint8_t decode,
   uint8_t code_1,
   uint8_t code_2);

/**
 * Handle CMDEV events.
 *
 * For an ABORT event (given some conditions) the related sessions and the AR
 * are released, and the global RPC socket is re-opened.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param event            In:    The new CMDEV state. Use PNET_EVENT_xxx, not
 *                                PF_CMDEV_STATE_xxx
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrpc_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pnet_event_values_t event);

/**
 * Send a DCE RPC request to the controller.
 * The only request handled is the CControl (APPL_RDY) request.
 *
 * Opens a new UDP socket for the session.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrpc_rm_ccontrol_req (pnet_t * net, pf_ar_t * p_ar);

/**
 * Show AR and session information.
 *
 * Use level 0xFFFF to show everything.
 *
 * @param net              InOut: The p-net stack instance
 * @param level            In: Level of detail.
 */
void pf_cmrpc_show (pnet_t * net, unsigned level);

/************************* Utilities ******************************************/

/**
 * Display buffer contents
 *
 * For example:
 *
 *    uint8_t mybuffer[18] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
 * 'K', 'L', 0, 1, 2, 3, 4, 5}; pf_memory_contents_show(mybuffer, 18);
 *
 * will be displayed as:
 *
 *    41 42 43 44 45 46 47 48 49 4a 4b 4c 00 01 02 03 |ABCDEFGHIJKL....|
 *    04 05                                           |..|
 *
 * @param data            In:   Buffer contents to be displayed
 * @param size            In:   Buffer size (or smaller, to only display the
 * beginning)
 */
void pf_memory_contents_show (const uint8_t * data, int size);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMRPC_H */
