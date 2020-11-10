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

#ifndef PF_PDPORT_H
#define PF_PDPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * Init PDPort data.
 * Load port configuration from nvm.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred
 */
int pf_pdport_init (pnet_t * net);

/**
 * Reset PDPort configuration data for all ports.
 * Clear DPPort configuration in nvm.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred
 */
int pf_pdport_reset_all (pnet_t * net);

/**
 * Read a DPPort data record/index
 * Record to read defined by index in request
 * Parse request and write response to output buffer.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_read_request   In:    The read request.
 * @param p_read_result    In:    The result information.
 * @param res_size         In:    The size of the output buffer.
 * @param p_res            Out:   The output buffer.
 * @param p_pos            InOut: Position in the output buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_pdport_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_request,
   const pf_iod_read_result_t * p_read_result,
   uint16_t res_size,
   uint8_t * p_res,
   uint16_t * p_pos);

/**
 * Write a DPPort data record/index
 * Record to write defined by index in request
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param p_write_request  In:   The IODWrite request.
 * @param p_req_buf        In:   The request buffer.
 * @param data_length      In:   Size of the data to write.
 * @param p_req_pos        InOut:Position within the request buffer.
 * @param p_result         Out:  Detailed error information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_pdport_write_req (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_request,
   uint8_t * p_req_buf,
   uint16_t data_length,
   pnet_result_t * p_result);

#ifdef __cplusplus
}
#endif

#endif /* PF_PDPORT_H */
