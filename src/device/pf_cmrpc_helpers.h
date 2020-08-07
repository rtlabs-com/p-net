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

#ifndef PF_CMRPC_HELPERS_H
#define PF_CMRPC_HELPERS_H

#ifdef __cplusplus
extern "C"
{
#endif

/************ Internal functions, made available for unit testing ************/

void pf_generate_uuid(
   uint32_t                timestamp,
   uint32_t                session_number,
   pnet_ethaddr_t            mac_address,
   pf_uuid_t               *p_uuid);

int pf_cmrpc_lookup_request(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   pf_rpc_header_t         *p_rpc_req,
   pf_rpc_lookup_req_t     *p_lkup_request,
   pnet_result_t           *p_read_result,
   uint16_t                res_size,      /** sizeof(output buffer) */
   uint8_t                 *p_res,        /** Output buffer */
   uint16_t                *p_pos);       /** in/out: Current pos in output buffer */


#ifdef __cplusplus
}
#endif

#endif /* PF_CMRPC_HELPERS_H */
