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

#ifndef PF_CMRDR_H
#define PF_CMRDR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle a RPC read request.
 * Triggers the \a pnet_read_ind() user callback for some values.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_read_request   In:    The read request.
 * @param p_read_status    Out:   The result information.
 * @param res_size         In:    The size of the output buffer.
 * @param p_res            Out:   The output buffer.
 * @param p_pos            InOut: Position in the output buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrdr_rm_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_request,
   pnet_result_t * p_read_status,
   uint16_t res_size,
   uint8_t * p_res,
   uint16_t * p_pos);

/**
 * Describe an index on a subslot.
 *
 * @param index            In:    The index
 * @return a descriptive string.
 */
const char * pf_index_to_logstring (uint16_t index);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMRDR_H */
