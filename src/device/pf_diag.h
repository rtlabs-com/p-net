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
extern "C"
{
#endif

/* Masks for the different channel levels */
#define PF_DIAG_QUAL_FAULT_MASK        0xf8000000
#define PF_DIAG_QUAL_M_DEM_MASK        0x07fe0000
#define PF_DIAG_QUAL_M_REQ_MASK        0x0001ff80
#define PF_DIAG_QUAL_ADVICE_MASK       0x00000078

/* Digest bits in maint_status */
#define PF_DIAG_MAINT_STATUS_REQ_BIT   0
#define PF_DIAG_MAINT_STATUS_DEM_BIT   1

/**
 * Initialize the diag component.
 *
 * @return  0  Always
 */
int pf_diag_init(void);

/**
 * Reset the diag component.
 *
 * @return  0  Always
 */
int pf_diag_exit(void);

/**
 * Add a diagnosis entry.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API.
 * @param slot_nbr         In:   The slot.
 * @param subslot_nbr      In:   The sub-slot.
 * @param ch_nbr           In:   The channel number
 * @param ch_properties    In:   The channel properties.
 * @param ch_error_type    In:   The channel error type.
 * @param ext_ch_error_type In:  The extended channel error type.
 * @param ext_ch_add_value In:   The extended channel error additional value.
 * @param qual_ch_qualifier In:  The qualified channel qualifier.
 * @param usi              In:   The USI.
 * @param p_manuf_data     In:   The manufacturer specific diagnosis data.
 *                               (Only needed if USI <= 0x7fff).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_add(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                ch_nbr,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                ext_ch_error_type,
   uint32_t                ext_ch_add_value,
   uint32_t                qual_ch_qualifier,
   uint16_t                usi,
   uint8_t                 *p_manuf_data);

/**
 * Update a diagnosis entry.
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 * - API id.
 * - Slot number.
 * - Sub-slot number.
 * - Channel number.
 * - Channel properties (the channel direction part only).
 * - Channel error type.
 *
 * When the item is found either the manufacturer data is updated (for
 * USI in manufacturer-specific range) or the extended channel additional
 * value is updated.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API.
 * @param slot_nbr         In:   The slot.
 * @param subslot_nbr      In:   The sub-slot.
 * @param ch_nbr           In:   The channel number
 * @param ch_properties    In:   The channel properties.
 * @param ch_error_type    In:   The channel error type.
 * @param ext_ch_add_value In:   The extended channel error additional value.
 * @param usi              In:   The USI.
 * @param p_manuf_data     In:   The manufacturer specific diagnosis data.
 *                               (Only needed if USI <= 0x7fff).
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_update(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                ch_nbr,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint32_t                ext_ch_add_value,
   uint16_t                usi,
   uint8_t                 *p_manuf_data);

/**
 * Remove a diagnosis entry.
 *
 * If the USI of the item is in the manufacturer-specified range then
 * the USI is used to identify the item.
 * Otherwise the other parameters are used (all must match):
 * - API id.
 * - Slot number.
 * - Sub-slot number.
 * - Channel number.
 * - Channel properties (the channel direction part only).
 * - Channel error type.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:   The AR instance.
 * @param api_id           In:   The API.
 * @param slot_nbr         In:   The slot.
 * @param subslot_nbr      In:   The sub-slot.
 * @param ch_nbr           In:   The channel number
 * @param ch_properties    In:   The channel properties.
 * @param ch_error_type    In:   The channel error type.
 * @param usi              In:   The USI.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_diag_remove(
   pnet_t                  *net,
   pf_ar_t                 *p_ar,
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint16_t                ch_nbr,
   uint16_t                ch_properties,
   uint16_t                ch_error_type,
   uint16_t                usi);

int pf_diag_get_maintenance_status(
   uint32_t                api_id,
   uint16_t                slot_nbr,
   uint16_t                subslot_nbr,
   uint32_t                *p_maint_status);

#ifdef __cplusplus
}
#endif

#endif /* PF_DIAG_H */
