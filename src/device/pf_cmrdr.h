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
extern "C"
{
#endif

#include "pnet_api.h"

/**
 * Handle a RPC read request.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_read_request   In:   The read request.
 * @param p_read_result    Out:  The result information.
 * @param res_size         In:   The size of the output buffer.
 * @param p_res            Out:  The output buffer.
 * @param p_pos            InOut:Position in the output buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrdr_rm_read_ind(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_iod_read_request_t   *p_read_request,
   pnet_result_t           *p_read_result,
   uint16_t                res_size,      /** sizeof(output buffer) */
   uint8_t                 *p_res,        /** Output buffer */
   uint16_t                *p_pos);       /** in/out: Current pos in output buffer */

#ifdef __cplusplus
}
#endif

#endif /* PF_CMRDR_H */
