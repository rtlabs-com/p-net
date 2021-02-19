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

#ifndef PF_DIAG_H
#define PF_DIAG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Masks for the different channel levels */
#define PF_DIAG_QUAL_FAULT_MASK  0xf8000000
#define PF_DIAG_QUAL_M_DEM_MASK  0x07fe0000
#define PF_DIAG_QUAL_M_REQ_MASK  0x0001ff80
#define PF_DIAG_QUAL_ADVICE_MASK 0x00000078

/* Digest bits in maint_status */
#define PF_DIAG_MAINT_STATUS_REQ_BIT 0
#define PF_DIAG_MAINT_STATUS_DEM_BIT 1

/**
 * Initialize the diag component.
 *
 * @return  0  Always
 */
int pf_diag_init (void);

/**
 * Reset the diag component.
 *
 * @return  0  Always
 */
int pf_diag_exit (void);

/************************** General diagnosis functions **********************/

/**
 * Add a diagnosis entry, in standard or USI format.
 *
 * Typically it is easier to use these instead:
 *  - pf_diag_std_add()
 *  - pf_diag_usi_add()
 *
 * Note that the PLC can set the max alarm payload length at startup, and
 * that affects how large diagnosis entries can be sent via alarms.
 *
 * This sends a diagnosis alarm.
 *
 * @param net                 InOut: The p-net stack instance.
 * @param p_diag_source       In:    Slot, subslot, channel, direction etc.
 * @param ch_bits             In:    Number of bits in the channel.
 * @param severity            In:    Diagnosis severity.
 * @param ch_error_type       In:    The channel error type.
 * @param ext_ch_error_type   In:    The extended channel error type.
 * @param ext_ch_add_value    In:    The extended channel error additional
 *                                   value.
 * @param qual_ch_qualifier   In:    The qualified channel qualifier.
 * @param usi                 In:    The USI.
 * @param manuf_data_len      In:    Length in bytes of the
 *                                   manufacturer specific diagnosis data.
 *                                   Max PNET_MAX_DIAG_MANUF_DATA_SIZE  or
 *                                   value from PLC.
 *                                   (Only needed if USI <= 0x7fff,
 *                                    and may still be 0).
 * @param p_manuf_data        In:    The manufacturer specific diagnosis data.
 *                                   (Only needed if USI <= 0x7fff).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_add (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   pnet_diag_ch_prop_type_values_t ch_bits,
   pnet_diag_ch_prop_maint_values_t severity,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint32_t qual_ch_qualifier,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data);

/**
 * Update a diagnosis entry, in standard or USI format.
 *
 * Typically it is easier to use these instead:
 *  - pf_diag_std_update()
 *  - pf_diag_usi_update()
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 *  - p_diag_source (all fields)
 *  - Channel error type.
 *  - Extended error type.
 *
 * When the item is found either the manufacturer data is updated (for
 * USI in manufacturer-specific range) or the extended channel additional
 * value is updated.
 *
 * Note that the PLC can set the max alarm payload length at startup, and
 * that affects how large diagnosis entries can be sent via alarms.
 *
 * This sends a diagnosis alarm.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type, or 0.
 * @param ext_ch_add_value  In:    New extended channel error additional value.
 * @param usi               In:    The USI.
 * @param manuf_data_len    In:    Length in bytes of the
 *                                 manufacturer specific diagnosis data.
 *                                 Max PNET_MAX_DIAG_MANUF_DATA_SIZE or
 *                                 value from PLC.
 *                                (Only needed if USI <= 0x7fff,
 *                                 and may still be 0).
 * @param p_manuf_data      In:    New manufacturer specific diagnosis data.
 *                                 (Only needed if USI <= 0x7fff).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_update (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data);

/**
 * Remove a diagnosis entry, in standard or USI format.
 *
 * Typically it is easier to use these instead:
 *  - pf_diag_std_remove()
 *  - pf_diag_usi_remove()
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 *  - p_diag_source (all fields)
 *  - Channel error type.
 *  - Extended error type.
 *
 * This sends a diagnosis alarm.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type, or 0.
 * @param usi               In:    The USI.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_remove (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint16_t usi);

/************************** Diagnosis in standard format *********************/

/**
 * Add a diagnosis entry, in standard format.
 *
 * This sends a diagnosis alarm.
 *
 * @param net                 InOut: The p-net stack instance.
 * @param p_diag_source       In:    Slot, subslot, channel, direction etc.
 * @param ch_bits             In:    Number of bits in the channel.
 * @param severity            In:    Diagnosis severity.
 * @param ch_error_type       In:    The channel error type.
 * @param ext_ch_error_type   In:    The extended channel error type.
 * @param ext_ch_add_value    In:    The extended channel error additional
 *                                   value.
 * @param qual_ch_qualifier   In:    The qualified channel qualifier.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_std_add (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   pnet_diag_ch_prop_type_values_t ch_bits,
   pnet_diag_ch_prop_maint_values_t severity,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value,
   uint32_t qual_ch_qualifier);

/**
 * Update a diagnosis entry, in standard format.
 *
 * This sends a diagnosis alarm.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type.
 * @param ext_ch_add_value  In:    New extended channel error additional value.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_std_update (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type,
   uint32_t ext_ch_add_value);

/**
 * Remove a diagnosis entry, in the standard format.

 * This sends a diagnosis alarm.
 *
 * @param net               InOut: The p-net stack instance.
 * @param p_diag_source     In:    Slot, subslot, channel, direction etc.
 * @param ch_error_type     In:    The channel error type.
 * @param ext_ch_error_type In:    The extended channel error type.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_std_remove (
   pnet_t * net,
   const pnet_diag_source_t * p_diag_source,
   uint16_t ch_error_type,
   uint16_t ext_ch_error_type);

/************************** Diagnosis in USI format **************************/

/**
 * Add a diagnosis entry in manufacturer-specified ("USI") format.
 *
 * If the diagnosis already exists, it is updated.
 *
 * A diagnosis in USI format is assigned to the channel "whole submodule"
 * (not individual channels). The severity is always "Fault".
 *
 * Note that the PLC can set the max alarm payload length at startup, and
 * that affects how large diagnosis entries can be sent via alarms.
 *
 * This sends a diagnosis alarm.
 *
 * @param net              InOut: The p-net stack instance.
 * @param api_id           In:    The API.
 * @param slot_nbr         In:    The slot.
 * @param subslot_nbr      In:    The sub-slot.
 * @param usi              In:    The USI. Range 0..0x7fff
 * @param manuf_data_len   In:    Length in bytes of the
 *                                manufacturer specific diagnosis data.
 *                                Max PNET_MAX_DIAG_MANUF_DATA_SIZE or
 *                                value from PLC.
 * @param p_manuf_data     In:    The manufacturer specific diagnosis data.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_usi_add (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data);

/**
 * Update a diagnosis entry in manufacturer-specified ("USI") format.
 *
 * An error is returned if the diagnosis doesn't exist.
 *
 * Note that the PLC can set the max alarm payload length at startup, and
 * that affects how large diagnosis entries can be sent via alarms.
 *
 * This sends a diagnosis alarm.
 *
 * @param net              InOut: The p-net stack instance.
 * @param api_id           In:    The API.
 * @param slot_nbr         In:    The slot.
 * @param subslot_nbr      In:    The sub-slot.
 * @param usi              In:    The USI. Range 0..0x7fff
 * @param manuf_data_len   In:    Length in bytes of the
 *                                manufacturer specific diagnosis data.
 *                                Max PNET_MAX_DIAG_MANUF_DATA_SIZE or
 *                                value from PLC.
 * @param p_manuf_data     In:    New manufacturer specific diagnosis data.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_usi_update (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t usi,
   uint16_t manuf_data_len,
   const uint8_t * p_manuf_data);

/**
 * Remove a diagnosis entry in manufacturer-specified ("USI") format.
 *
 * This sends a diagnosis alarm.
 *
 * @param net              InOut: The p-net stack instance.
 * @param api_id           In:    The API.
 * @param slot_nbr         In:    The slot.
 * @param subslot_nbr      In:    The sub-slot.
 * @param usi              In:    The USI. Range 0..0x7fff
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_usi_remove (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint16_t usi);

/* Not used yet */
int pf_diag_get_maintenance_status (
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint32_t * p_maint_status);

#ifdef __cplusplus
}
#endif

#endif /* PF_DIAG_H */
