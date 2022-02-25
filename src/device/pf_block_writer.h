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

#ifndef PF_BLOCK_WRITER_H
#define PF_BLOCK_WRITER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Insert a sequence of bytes into a buffer.
 * @param p_src            In:   The start of the byte sequence to insert.
 * @param src_size         In:   Number of bytes to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer. Silently ignored if NULL.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_mem (
   const void * p_src,
   uint16_t src_size,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a sequence of bytes into a buffer, for overlapping source and
 * destinations.
 *
 * Uses memmove() internally.
 *
 * @param p_src            In:   The start of the byte sequence to insert.
 * @param src_size         In:   Number of bytes to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_mem_overlapping (
   const void * p_src,
   uint16_t src_size,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a byte into a buffer.
 * @param val              In:   The byte to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_byte (
   uint8_t val,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a 16-bit entity into a buffer
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param val              In:   The value to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_uint16 (
   bool is_big_endian,
   uint16_t val,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a 32-bit entity into a buffer
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param val              In:   The value to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_uint32 (
   bool is_big_endian,
   uint32_t val,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert fixed length zero padding into a buffer.
 * @param n_bytes          In:   The number of bytes byte to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_padding (
   uint16_t n_bytes,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Pad a buffer with zeroes until block length is aligned
 *
 * @param start_position   In:   Start of block to align.
 * @param align            In:   Alignment. 2 for 16 bit alignment,
 *                               4 for 32bit alignment
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_padding_align (
   uint16_t start_postion,
   uint16_t align,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an AR result block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_ar             In:   Contains the AR result to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_ar_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an IOCR result block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_ar             In:   Contains the AR result to insert.
 * @param ix               In:   The index into p_ar->iocr[].result to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_iocr_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an alarm CR block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_ar             In:   Contains the alarm CR to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_alarm_cr_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert AR diff blocks into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_ar             In:   Contains the AR diff to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_ar_diff (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an AR RPC result block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_ar             In:   Contains the AR RPC result to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_ar_rpc_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an AR server result block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_ar             In:   Contains the AR server result to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_ar_server_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an AR vendor result block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_ar             In:   Contains the AR vendor result to insert.
 * @param ix               In:   The index into p_ar_ar_vendor_result[].
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_ar_vendor_result (
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint16_t ix,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an AR Data block into a buffer.
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_ar             In:   Contains the AR to insert (NULL for "All ARs").
 * @param api_id           In:   Specific API to insert if p_ar != NULL.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_ar_data (
   pnet_t * net,
   bool is_big_endian,
   const pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a RPC control block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param block_type       In:   The control block type.
 * @param p_res            In:   The control block to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_control (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   const pf_control_block_t * p_res,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an RPC read result into a buffer.
 * Return the position of the data_length member of the read result as it must
 * be modified later.
 * @param is_big_endian       In:   Endianness of the destination buffer.
 * @param p_res               In:   The read result to insert.
 * @param res_len             In:   Size of destination buffer.
 * @param p_bytes             Out:  Destination buffer.
 * @param p_pos               InOut:Position in destination buffer.
 * @param p_data_length_pos   Out:  Position of the data length field
 */
void pf_put_read_result (
   bool is_big_endian,
   const pf_iod_read_result_t * p_res,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos,
   uint16_t * p_data_length_pos);

/**
 * Insert read data (from the application) into a buffer.
 * Use a block header with the specified block_type.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param block_type       In:   The block type to use for the data.
 * @param raw_length       In:   The length in bytes of the user data.
 * @param p_raw_data       In:   The data to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_record_data_read (
   bool is_big_endian,
   pf_block_type_values_t block_type,
   uint16_t raw_length,
   const uint8_t * p_raw_data,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert I&M0 filter data into a buffer.
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_im_0_filter_data (
   pnet_t * net,
   bool is_big_endian,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert I&M0 data into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_im_0           In:   The I&M0 data to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_im_0 (
   bool is_big_endian,
   const pnet_im_0_t * p_im_0,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert I&M1 data into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_im_1           In:   The I&M1 data to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_im_1 (
   bool is_big_endian,
   const pnet_im_1_t * p_im_1,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert I&M2 data into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_im_2           In:   The I&M2 data to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_im_2 (
   bool is_big_endian,
   const pnet_im_2_t * p_im_2,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert I&M3 data into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_im_3           In:   The I&M3 data to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_im_3 (
   bool is_big_endian,
   const pnet_im_3_t * p_im_3,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert filtered real or expected ident data into a buffer.
 *
 * filter_level specifies what is being filtered:
 *   If p_ar is != NULL then only select items that belong to this AR.
 *
 *   PF_DEV_FILTER_LEVEL_SUBSLOT means "Only include sub-slot"
 *      specified by api_id, slot_nbr and subslot_nbr.
 *   PF_DEV_FILTER_LEVEL_SLOT means "Include all sub-slots of slot"
 *      specified by api_id and slot_nbr.
 *   PF_DEV_FILTER_LEVEL_API means "Include all slots and sub-slots of API id"
 *      specified by api_id.
 *   PF_DEV_FILTER_LEVEL_DEVICE essentially means "No filtering" on API id,
 *      slot_nbr or subslot_nbr.
 *
 * stop_level specifies how much data is being included:
 *    PF_DEV_FILTER_LEVEL_SUBSLOT means "Include all levels".
 *    PF_DEV_FILTER_LEVEL_SLOT means "Do not include sub-slots".
 *    PF_DEV_FILTER_LEVEL_API means "Do not include slots or sub-slots".
 *    PF_DEV_FILTER_LEVEL_DEVICE means "Only include API count".
 *
 * @param net               InOut: The p-net stack instance
 * @param is_big_endian     In:    Endianness of the destination buffer.
 * @param block_version_low In:    The minor version number of the block to
 *                                   insert.
 * @param block_type        In:    Specifies REAL or EXP ident number to insert.
 * @param filter_level      In:    The filter level.
 * @param stop_level        In:    The amount of detail to include
 *                                 (ending level).
 * @param p_ar              In:    If != NULL then filter by AR.
 * @param api_id            In:    API id to filter by.
 * @param slot_nbr          In:    Slot number to filter by.
 * @param subslot_nbr       In:    Sub-slot number to filter by.
 * @param res_len           In:    Size of destination buffer.
 * @param p_bytes           Out:   Destination buffer.
 * @param p_pos             InOut: Position in destination buffer.
 */
void pf_put_ident_data (
   pnet_t * net,
   bool is_big_endian,
   uint8_t block_version_low,
   pf_block_type_values_t block_type,
   pf_dev_filter_level_t filter_level,
   pf_dev_filter_level_t stop_level,
   const pf_ar_t * p_ar,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a pnet status into a buffer
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_status         In:   The pnet status to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_pnet_status (
   bool is_big_endian,
   const pnet_pnio_status_t * p_status,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a DCE RPC header into a buffer.
 * The endianness is determined from a member of the rpc header.
 * Return the position of the length_of_body member of the read result as it
 * must be modified later.
 * @param p_rpc            In:   The DCE RPC header to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 * @param p_pos_body_len   Out:  Position of the length_of_body member.
 */
void pf_put_dce_rpc_header (
   pf_rpc_header_t * p_rpc,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos,
   uint16_t * p_pos_body_len);

/**
 * Insert a write result into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_res            In:   The write result to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_write_result (
   bool is_big_endian,
   const pf_iod_write_result_t * p_res,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a Log Book entry block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param p_log_book       In:   The log book entry to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_log_book_data (
   bool is_big_endian,
   const pf_log_book_t * p_log_book,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert the fixed part of the RTA-PDU into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param pf_alarm_fixed_t In:   The fixed alarm part to insert.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_alarm_fixed (
   bool is_big_endian,
   const pf_alarm_fixed_t * p_alarm_fixed,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert an AlarmNotification or AlarmAck block into a buffer.
 *
 * This function can insert a complete AlarmNotification-PDU.
 *
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param bh_type          In:    The block type.
 * @param p_alarm_data     In:    Alarm type, slot, subslot etc. Mandatory.
 *                                The payload part of it (if any) is not used.
 * @param maint_status     In:    The Maintenance status used for specific USI
 *                                values (inserted only if not zero).
 * @param payload_usi      In:    The payload USI (may be 0). Only used for
 *                                block type = alarm notify.
 *                                Use 0 for no payload.
 * @param payload_len      In:    Number of bytes of manufaturer data.
 *                                Only used for manufacturer data
 *                                (payload USI <= 0x7FFF).
 *                                May be 0 for manufacturer data.
 * @param p_payload        In:    pf_diag_item_t, manufacturer data or NULL.
 *                                Mandatory if payload_len > 0 or for USI
 *                                values that expect a pf_diag_item_t
 * @param p_status         In:    PNIO status. Only used for block type = alarm
 *                                ack.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_alarm_block (
   bool is_big_endian,
   pf_block_type_values_t bh_type,
   const pf_alarm_data_t * p_alarm_data,
   uint32_t maint_status,
   uint16_t payload_usi,
   uint16_t payload_len,
   const uint8_t * p_payload,
   const pnet_pnio_status_t * p_status,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a SubstituteValue block block into a buffer.
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param sub_mode         In:   The substitution mode.
 * @param iocs_len         In:   Length of IOCS data.
 * @param p_iocs           In:   The IOCS data.
 * @param iops_len         In:   Length of IOPS data.
 * @param p_iops           In:   IOPS data.
 * @param data_len         In:   Length of output data.
 * @param p_data           In:   Substitution data.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_substitute_data (
   bool is_big_endian,
   uint16_t sub_mode,
   uint8_t iocs_len,
   const uint8_t * p_iocs,
   uint8_t iops_len,
   const uint8_t * p_iops,
   uint16_t data_len,
   const uint8_t * p_data,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a RecordOutputDataObjectElement block into a buffer.
 *
 * This block inserts a complete RecordOutputDataObjectElement block into a
 * buffer. It includes the substitution data block if p_sub_data != NULL. The
 * IOCS and IOPS of the substitution data value is the same as the ordinary IOCS
 * and IOPS. The length of the substitution data is the same as data_len.
 *
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param sub_active       In:   true if the substitution data are used.
 * @param iocs_len         In:   Length of IOCS data.
 * @param p_iocs           In:   The IOCS data.
 * @param iops_len         In:   Length of IOPS data.
 * @param p_iops           In:   IOPS data.
 * @param data_len         In:   Length of output data.
 * @param p_data           In:   Output data.
 * @param sub_mode         In:   The substitution mode.
 * @param p_sub_data       In:   The substitution data.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_output_data (
   bool is_big_endian,
   bool sub_active,
   uint8_t iocs_len,
   const uint8_t * p_iocs,
   uint8_t iops_len,
   const uint8_t * p_iops,
   uint16_t data_len,
   const uint8_t * p_data,
   uint16_t sub_mode,
   const uint8_t * p_sub_data,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a RecordInputDataObjectElement block into a buffer.
 *
 * This block inserts a complete RecordInputDataObjectElement block into a
 * buffer.
 *
 * @param is_big_endian    In:   Endianness of the destination buffer.
 * @param iocs_len         In:   Length of IOCS data.
 * @param p_iocs           In:   The IOCS data.
 * @param iops_len         In:   Length of IOPS data.
 * @param p_iops           In:   IOPS data.
 * @param data_len         In:   Length of output data.
 * @param p_data           In:   Output data.
 * @param res_len          In:   Size of destination buffer.
 * @param p_bytes          Out:  Destination buffer.
 * @param p_pos            InOut:Position in destination buffer.
 */
void pf_put_input_data (
   bool is_big_endian,
   uint8_t iocs_len,
   const uint8_t * p_iocs,
   uint8_t iops_len,
   const uint8_t * p_iops,
   uint16_t data_len,
   const uint8_t * p_data,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert a DiagnosisData block into a buffer.
 *
 * Insert filtered Diagnosis, Maintenance, Qualifiers and Status for one
 * sub-slot/slot/ar/api.
 *
 * filter_level specifies what is being filtered:
 *    If p_ar is != NULL then only select items that belong to this AR.
 *    PF_DEV_FILTER_LEVEL_SUBSLOT:  Only include sub-slot specified by api_id,
 *                                  slot_nbr and subslot_nbr.
 *    PF_DEV_FILTER_LEVEL_SLOT:     Include all sub-slots of slot specified by
 *                                  api_id and slot_nbr.
 *    PF_DEV_FILTER_LEVEL_API:      Include all slots and sub-slots of API id
 *                                  specified by api_id.
 *    PF_DEV_FILTER_LEVEL_DEVICE:   No filtering on API id, slot_nbr or
 *                                  subslot_nbr (i.e. all diags)
 *
 * diag_filter is an ortogonal filter that selects only specific diag types:
 *    PF_DIAG_FILTER_FAULT_STD:  Only STD, severity FAULT.
 *    PF_DIAG_FILTER_FAULT_ALL:  Both USI and STD, severity FAULT.
 *    PF_DIAG_FILTER_ALL:        All types of diag, both USI and STD.
 *    PF_DIAG_FILTER_M_REQ:      Only Maintenance required.
 *    PF_DIAG_FILTER_M_DEM:      Only Maintenance demanded.
 *
 * Implemented using:
 *    pf_put_diag_device()
 *       pf_put_diag_api()              for all APIs
 *          pf_put_diag_slot()          for all slots
 *             pf_put_diag_subslot()    for all subslots
 *               pf_put_diag_list()     Header insertion for a USI value
 *                  pf_put_diag_item()  Insertion of diag item
 *
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param filter_level     In:    The filter level.
 * @param diag_filter      In:    The diag type filter.
 * @param p_ar             In:    If != NULL then filter by AR.
 * @param api_id           In:    The API id to filter by.
 * @param slot_nbr         In:    The slot number to filter by.
 * @param subslot_nbr      In:    The sub-slot number to filter by.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_diag_data (
   pnet_t * net,
   bool is_big_endian,
   pf_dev_filter_level_t filter_level,
   pf_diag_filter_level_t diag_filter,
   const pf_ar_t * p_ar, /* If != NULL only include those belonging to p_ar */
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert PDport data check block into a buffer.
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param slot             In:    Slot number
 * @param subslot          In:    Subslot number
 * @param check_peer       In:    check_peer data. ToDo - add support for misc
 *                                port data checks
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_pdport_data_check (
   bool is_big_endian,
   uint16_t slot,
   uint16_t subslot,
   const pf_check_peer_t * check_peer,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert PDport data adjust block into a buffer.
 *
 * ToDo - Add support for other adjust properties
 *
 * @param is_big_endian             In:    Endianness of the destination buffer.
 * @param subslot                   In:    DAP subslot identifying the port.
 * @param p_peer_to_peer_boundary   In:    Peer to peer boundary
 * @param res_len                   In:    Size of destination buffer.
 * @param p_bytes                   Out:   Destination buffer.
 * @param p_pos                     InOut: Position in destination buffer.
 */
void pf_put_pdport_data_adj (
   bool is_big_endian,
   uint16_t subslot,
   const pf_adjust_peer_to_peer_boundary_t * p_peer_to_peer_boundary,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert pd interface adjust block into a buffer.
 *
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param subslot          In:    DAP subslot identifying the interface
 * @param mode             In:    Multiple interface mode.
 *                                (name of device mode)
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_pd_interface_adj (
   bool is_big_endian,
   uint16_t subslot,
   pf_lldp_name_of_device_mode_t name_of_device_mode,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert PDport real data block into a buffer.
 *
 * Includes peer chassis ID, peer MAC address and peer MAU type.
 *
 * @param is_big_endian       In:    Endianness of the destination buffer.
 * @param subslot             In:    Subslot
 * @param p_peer_station_name In:    Peer station name
 * @param p_peer_port_name    In:    Peer port name
 * @param p_port_data         In:    Port data
 * @param media_type          In:    Media type
 * @param p_eth_status        In:    Ethernet status
 * @param res_len             In:    Size of destination buffer.
 * @param p_bytes             Out:   Destination buffer.
 * @param p_pos               InOut: Position in destination buffer.
 */
void pf_put_pdport_data_real (
   bool is_big_endian,
   uint16_t subslot,
   const pf_lldp_station_name_t * p_peer_station_name,
   const pf_lldp_port_name_t * p_peer_port_name,
   const pf_port_t * p_port_data,
   pf_mediatype_values_t media_type,
   const pnal_eth_status_t * p_eth_status,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert PD port statistics block into a buffer.
 *
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_port_stats     In:    Port statistics
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_pdport_statistics (
   bool is_big_endian,
   const pnal_port_stats_t * p_port_stats,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert PD interface real data block into a buffer.
 *
 * This includes chassis ID, MAC address, IP address, subnet and gateway.
 *
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param mac_address      In:    MAC address for DAP
 * @param ip_address       In:    IP address
 * @param netmask          In:    Netmask
 * @param gateway          In:    Gateway address
 * @param station_name     In:    Station name, terminated string, buffer
 *                                size PNET_STATION_NAME_MAX_SIZE
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_pdinterface_data_real (
   bool is_big_endian,
   const pnet_ethaddr_t * mac_address,
   pnal_ipaddr_t ip_address,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway,
   const char * station_name,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert multiblock port and statistics for one port
 *
 * @param is_big_endian       In:    Endianness of the destination buffer.
 * @param api                 In:    API number
 * @param subslot             In:    DAP subslot for port
 * @param p_peer_station_name In:    Peer station name
 * @param p_peer_port_name    In:    Peer port name
 * @param p_port_data         In:    Port data
 * @param media_type          In:    Media type
 * @param p_eth_status        In:    Ethernet status
 * @param p_port_statistics   In:    Port statistics
 * @param res_len             In:    Size of destination buffer.
 * @param p_bytes             Out:   Destination buffer.
 * @param p_pos               InOut: Position in destination buffer.
 */
void pf_put_pd_multiblock_port_and_statistics (
   bool is_big_endian,
   uint32_t api,
   uint16_t subslot,
   const pf_lldp_station_name_t * p_peer_station_name,
   const pf_lldp_port_name_t * p_peer_port_name,
   const pf_port_t * p_port_data,
   pf_mediatype_values_t media_type,
   const pnal_eth_status_t * p_eth_status,
   const pnal_port_stats_t * p_port_statistics,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert multiblock interface and statistics
 *
 * @param is_big_endian     In:    Endianness of the destination buffer.
 * @param api               In:    API number
 * @param mac_address       In:    MAC address
 * @param ip_address        In:    IP address
 * @param netmask           In:    Netmask
 * @param gateway           In:    Gateway address
 * @param station_name      In:    Station name, terminated string, buffer
 *                                 size PNET_STATION_NAME_MAX_SIZE
 * @param p_port_statistics In:    Port statistics
 * @param res_len           In:    Size of destination buffer.
 * @param p_bytes           Out:   Destination buffer.
 * @param p_pos             InOut: Position in destination buffer.
 */
void pf_put_pd_multiblock_interface_and_statistics (
   bool is_big_endian,
   uint32_t api,
   const pnet_ethaddr_t * mac_address,
   pnal_ipaddr_t ip_address,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway,
   const char * station_name,
   const pnal_port_stats_t * p_port_statistics,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

/**
 * Insert epm lookup response data block into a buffer.
 * @param net              InOut: The p-net stack instance
 * @param is_big_endian    In:    Endianness of the destination buffer.
 * @param p_lookup_rsp     In:    Epm lookup response including entry and
 *                                protocol tower configuration.
 * @param res_len          In:    Size of destination buffer.
 * @param p_bytes          Out:   Destination buffer.
 * @param p_pos            InOut: Position in destination buffer.
 */
void pf_put_lookup_response_data (
   pnet_t * net,
   bool is_big_endian,
   const pf_rpc_lookup_rsp_t * p_lookup_rsp,
   uint16_t res_len,
   uint8_t * p_bytes,
   uint16_t * p_pos);

#ifdef __cplusplus
}
#endif

#endif /* PF_BLOCK_WRITER_H */
