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

#ifndef PF_CMRPC_EPM_H
#define PF_CMRPC_EPM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle rpc lookup request
 * @param net              InOut: The p-net stack instance
 * @param p_rpc_req        In:    Rpc header.
 * @param p_lookup_req     In:    Lookup request.
 * @param p_read_status    Out:   Read pnio status
 * @param res_size         In:    The size of the output buffer.
 * @param p_res            Out:   The output buffer.
 * @param p_pos            InOut: Position in the output buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmrpc_lookup_request (
   pnet_t * net,
   const pf_rpc_header_t * p_rpc_req,
   const pf_rpc_lookup_req_t * p_lookup_req,
   pnet_result_t * p_read_status,
   uint16_t res_size,
   uint8_t * p_res,
   uint16_t * p_pos);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMRPC_HELPERS_H */
