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

#ifndef PF_BLOCK_READER_H
#define PF_BLOCK_READER_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ======================== Public functions */

/**
 * @internal
 * Return a byte from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @return
 */
uint8_t pf_get_byte(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos);

/**
 * @internal
 * Return a uint16_t from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 */
uint16_t pf_get_uint16(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos);

/**
 * Extract a NDR header from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_ndr            Out:  Destination buffer.
 */
void pf_get_ndr_data(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_ndr_data_t           *p_ndr);

/**
 * Extract a block header from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_hdr            Out:  Destination buffer.
 */
void pf_get_block_header(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_block_header_t       *p_hdr);

/* Readers of specific blocks (do not read the block header). */

/**
 * Extract an AR param block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_ar_param(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_ar_t                 *p_ar);

/**
 * Extract an IOCR param block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param ix               In:   The current index into p_ar->iocr[].
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_iocr_param(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   uint16_t                ix,
   pf_ar_t                 *p_ar);

/**
 * Extract an expected API block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_exp_api_module(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_ar_t                 *p_ar);

/**
 * Extract an alarm CR block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_alarm_cr_request(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_ar_t                 *p_ar);

/**
 * Extract an AR RPC request block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_ar_rpc_request(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_ar_t                 *p_ar);

#if PNET_OPTION_AR_VENDOR_BLOCKS
/**
 * Extract an AR vendor request block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param ix               In:   The current index into p_ar->ar_vendor_xxx[].
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_ar_vendor_request(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   uint16_t                ix,
   pf_ar_t                 *p_ar);
#endif

#if PNET_OPTION_PARAMETER_SERVER
/**
 * Extract a parameter server block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_ar_prm_server_request(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_ar_t                 *p_ar);
#endif

#if PNET_OPTION_MC_CR
/**
 * Extract an MCR request block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param ix               In:   The current index into p_ar->ar_vendor_xxx[].
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_mcr_request(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   uint16_t                ix,
   pf_ar_t                 *p_ar);
#endif

#if PNET_OPTION_IR
/**
 * Extract an IR info request block from a buffer.
 * @param p_info           In:   The parser state.
 * @param p_pos            InOut:Position in the buffer.
 * @param p_ar             Out:  Contains the destination structure.
 */
void pf_get_ir_info_request(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_ar_t                 *p_ar);
#endif

/**
 * Extract a DCE RPC header from a raw UDP data buffer.
 * @param p_info           In:   The parser information.
 * @param p_pos            InOut:The current parsing position.
 * @param p_rpc            Out:  Destination struture.
 */
void pf_get_dce_rpc_header(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_rpc_header_t         *p_rpc);

/**
 * Extract a control request block from a buffer.
 * @param p_info           In:   The parser information.
 * @param p_pos            InOut:The current parsing position.
 * @param p_req            Out:  Destination structure.
 */
void pf_get_control(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_control_block_t      *p_req);

/**
 * Extract a read request block from a buffer.
 * @param p_info           In:   The parser information.
 * @param p_pos            InOut:The current parsing position.
 * @param p_req            Out:  Destination structure.
 */
void pf_get_read_request(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_iod_read_request_t   *p_req);

/**
 * Extract a write request block from a buffer.
 * @param p_info           In:   The parser information.
 * @param p_pos            InOut:The current parsing position.
 * @param p_req            Out:  Destination structure.
 */
void pf_get_write_request(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_iod_write_request_t  *p_req);

/**
 * Extract a I&M1 data block from a buffer.
 * @param p_info           In:   The parser information.
 * @param p_pos            InOut:The current parsing position.
 * @param p_im_1           Out:  Destination structure.
 */
void pf_get_im_1(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pnet_im_1_t             *p_im_1);

void pf_get_alarm_fixed(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_alarm_fixed_t        *p_alarm_fixed);

void pf_get_alarm_data(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_alarm_data_t         *p_alarm_data);

void pf_get_alarm_ack(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pf_alarm_data_t         *p_alarm_data);

void pf_get_pnio_status(
   pf_get_info_t           *p_info,
   uint16_t                *p_pos,
   pnet_pnio_status_t      *p_status);

#ifdef __cplusplus
}
#endif

#endif /* PF_BLOCK_READER_H */
