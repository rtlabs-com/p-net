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
extern "C" {
#endif

/* ======================== Public functions */

/**
 * @internal
 * Return a byte from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @return The byte from the buffer
 */
uint8_t pf_get_byte (pf_get_info_t * p_info, uint16_t * p_pos);

/**
 * @internal
 * Return a uint16_t from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @return The uint16_t from the buffer
 */
uint16_t pf_get_uint16 (pf_get_info_t * p_info, uint16_t * p_pos);

/**
 * @internal
 * Return a uint32_t from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 */
uint32_t pf_get_uint32 (pf_get_info_t * p_info, uint16_t * p_pos);

/**
 * @internal
 * Extract a sequence of bytes from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param dest_size        In:    Number of bytes to copy.
 * @param p_dest           Out:   Destination buffer.
 */
void pf_get_mem (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint16_t dest_size,
   void * p_dest);

/**
 * Extract a NDR header from a buffer.
 *
 * This is the first part of the payload of the incoming DCE/RPC message
 * (which is sent via UDP).
 *
 * Reads args_maximum, args_length, maximum_count, offset and actual_count.
 *
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_ndr            Out:   Destination buffer.
 */
void pf_get_ndr_data (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ndr_data_t * p_ndr);

/**
 * Extract a block header from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_hdr            Out:   Destination buffer.
 */
void pf_get_block_header (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_block_header_t * p_hdr);

/* Readers of specific blocks (do not read the block header). */

/**
 * Extract an AR param block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_ar_param (pf_get_info_t * p_info, uint16_t * p_pos, pf_ar_t * p_ar);

/**
 * Extract an IOCR param block from a buffer.
 *
 * Updates p_ar->iocrs[ix].param
 *
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param ix               In:    The current index into p_ar->iocr[].
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_iocr_param (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint16_t ix,
   pf_ar_t * p_ar);

/**
 * Extract an expected API block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_exp_api_module (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar);

/**
 * Extract an alarm CR block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_alarm_cr_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar);

/**
 * Extract an AR RPC request block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_ar_rpc_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar);

#if PNET_OPTION_AR_VENDOR_BLOCKS
/**
 * Extract an AR vendor request block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param ix               In:    The current index into p_ar->ar_vendor_xxx[].
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_ar_vendor_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint16_t ix,
   pf_ar_t * p_ar);
#endif

#if PNET_OPTION_PARAMETER_SERVER
/**
 * Extract a parameter server block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_ar_prm_server_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar);
#endif

#if PNET_OPTION_MC_CR
/**
 * Extract an MCR request block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param ix               In:    The current index into p_ar->ar_vendor_xxx[].
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_mcr_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint16_t ix,
   pf_ar_t * p_ar);
#endif

#if PNET_OPTION_IR
/**
 * Extract an IR info request block from a buffer.
 * @param p_info           InOut: The parser state.
 * @param p_pos            InOut: Position in the buffer.
 * @param p_ar             Out:   Contains the destination structure.
 */
void pf_get_ir_info_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_ar_t * p_ar);
#endif

/**
 * Extract a DCE RPC header from a raw UDP data buffer.
 * @param p_info           InOut: The parser information. Sets
 *                                p_info->is_big_endian
 * @param p_pos            InOut: The current parsing position.
 * @param p_rpc            Out:   Destination structure.
 */
void pf_get_dce_rpc_header (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_rpc_header_t * p_rpc);

/**
 * Extract a control request block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_req            Out:   Destination structure.
 */
void pf_get_control (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_control_block_t * p_req);

/**
 * Extract a read request block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_req            Out:   Destination structure.
 */
void pf_get_read_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_iod_read_request_t * p_req);

/**
 * Extract a lookup request block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_req            Out:   Destination structure.
 */
void pf_get_epm_lookup_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_rpc_lookup_req_t * p_req);

/**
 * Extract a write request block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_req            Out:   Destination structure.
 */
void pf_get_write_request (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_iod_write_request_t * p_req);

/**
 * Extract a I&M1 data block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_im_1           Out:   Destination structure.
 */
void pf_get_im_1 (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pnet_im_1_t * p_im_1);

/**
 * Extract the fixed part of an alarm frame from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_alarm_fixed    Out:   Destination structure.
 */
void pf_get_alarm_fixed (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_alarm_fixed_t * p_alarm_fixed);

/**
 * Extract an alarm DATA block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_alarm_data     Out:   Destination structure. Contains slot, subslot
 * etc.
 */
void pf_get_alarm_data (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_alarm_data_t * p_alarm_data);

/**
 * Extract an alarm ACK block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_alarm_data     Out:   Destination structure. Contains slot, subslot
 * etc.
 */
void pf_get_alarm_ack (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_alarm_data_t * p_alarm_data);

/**
 * Extract a PNIO status block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param p_status         Out:   Destination structure.
 */
void pf_get_pnio_status (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pnet_pnio_status_t * p_status);

/**
 * Extract PortDataCheck data block from buffer.
 * Use the block_header_type to identify which check is
 * contained in buffer and use corresponding parsing operation
 * to retrieve the check data. For example pf_get_port_data_check_check_peers().
 * @param p_info              InOut: The parser information.
 * @param p_pos               InOut: The current parsing position.
 * @param p_port_data_check:  Out:   Destination structure
 */
void pf_get_port_data_check (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_port_data_check_t * p_port_data_check);

/**
 * Extract CheckPeers data blocks from buffer.
 * @param p_info              InOut: The parser information.
 * @param p_pos               InOut: The current parsing position.
 * @param max_peers:          In:    Number of elements in destination array
 * @param check_peers:        Out:   Destination structure
 */
void pf_get_port_data_check_check_peers (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   uint8_t max_peers,
   pf_check_peers_t * check_peers);

/**
 * Extract PortDataAdjust data block from buffer.
 * Use the block_header_type to identify which adjust data is
 * contained in buffer and use corresponding parsing operation
 * to retrieve the data. For example
 * pf_get_port_data_adjust_peer_to_peer_boundary().
 * @param p_info                   InOut: The parser information.
 * @param p_pos                    InOut: The current parsing position.
 * @param p_port_data_check        Out:    Destination structure
 */
void pf_get_port_data_adjust (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_port_data_adjust_t * p_port_data_check);

/**
 * Extract a AdjustPeerToPeerBoundary data block from a buffer.
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param boundary         Out:   Destination structure.
 */
void pf_get_port_data_adjust_peer_to_peer_boundary (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_adjust_peer_to_peer_boundary_t * boundary);

/**
 * Extract a pd interface adjust block from a buffer.
 *
 * @param p_info           InOut: The parser information.
 * @param p_pos            InOut: The current parsing position.
 * @param mode             Out:   Multiple interface mode.
 */
void pf_get_interface_adjust (
   pf_get_info_t * p_info,
   uint16_t * p_pos,
   pf_lldp_name_of_device_mode_t * name_of_device_mode);

/************ Internal functions, made available for unit testing ************/

uint32_t pf_get_bits (uint32_t bits, uint8_t pos, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* PF_BLOCK_READER_H */
