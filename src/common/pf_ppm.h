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

#ifndef PF_PPM_H
#define PF_PPM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the PPM component.
 * @param net              InOut: The p-net stack instance
 */
void pf_ppm_init (pnet_t * net);

/**
 * Create a PPM instance.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR index.
 * @return  0  if the PPM instance was created.
 *          -1 if an error occurred.
 */
int pf_ppm_create (pnet_t * net, pf_ar_t * p_ar, uint32_t crep);

/**
 * Instantiate and start a PPM instance.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR index.
 * @return  0  if the PPM instance was activated.
 *          -1 if an error occurred.
 */
int pf_ppm_activate_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep);

/**
 * Close and de-commission a PPM instance.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR index.
 * @return  0  if the PPM instance was closed.
 *          -1 if an error occurred.
 */
int pf_ppm_close_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep);

/**
 * Find the AR, input IOCR and IODATA object instances for the specified
 * sub-slot.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API id.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param pp_ar            Out:   The AR instance.
 * @param pp_iocr          Out:   The IOCR instance.
 * @param pp_iodata        Out:   The IODATA object instance.
 * @param p_crep           Out:   The CREP index
 * @return  0  If the information has been found.
 *          -1 If the information was not found.
 */
int pf_ppm_get_ar_iocr_desc (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   pf_ar_t ** pp_ar,
   pf_iocr_t ** pp_iocr,
   pf_iodata_object_t ** pp_iodata,
   uint32_t * p_crep);

/**
 * Set the data and IOPS for a sub-module.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_data           In:   The application data.
 *                               If NULL is passed, frame data is
 *                               not updated.
 * @param data_len         In:   The length of the application data.
 * @param p_iops           In:   The IOPS of the application data.
 * @param iops_len         In:   The length of the IOPS.
 * @return  0  if the input data and IOPS was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_data_and_iops (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   const uint8_t * p_data,
   uint16_t data_len,
   const uint8_t * p_iops,
   uint8_t iops_len);

/**
 * Set IOCS for a sub-module.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_iocs           In:   The IOCS of the application data.
 * @param iocs_len         In:   The length of the IOCS data.
 * @return  0  if the IOCS was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_iocs (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   const uint8_t * p_iocs,
   uint8_t iocs_len);

/**
 * Retrieve the data and IOPS for a sub-module.
 *
 * Note that this is not from the PLC, but previously set by the application.
 *
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API id.
 * @param slot_nbr         In:    The slot number.
 * @param subslot_nbr      In:    The sub-slot number.
 * @param p_data           Out:   The application data.
 * @param p_data_len       In:    The length of the p_data buffer.
 *                         Out:   Actual length of the sub-module data.
 * @param p_iops           Out:   The IOPS of the application data.
 * @param p_iops_len       In:    Size of buffer at p_iops.
 *                         Out:   Actual length of IOPS data.
 * @return  0  if the input data and IOPS could e retrieved.
 *          -1 if an error occurred.
 */
int pf_ppm_get_data_and_iops (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint8_t * p_data,
   uint16_t * p_data_len,
   uint8_t * p_iops,
   uint8_t * p_iops_len);

/**
 * Retrieve IOCS for a sub-module.
 *
 * Note that this is not from the PLC, but previously set by the application.
 *
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:   The API id.
 * @param slot_nbr         In:   The slot number.
 * @param subslot_nbr      In:   The sub-slot number.
 * @param p_iocs           Out:  The IOCS of the application data.
 * @param p_iocs_len       In:   Size of buffer at p_iocs.
 *                         Out:  Actual length of IOCS data.
 * @return  0  if the IOCS could be retrieved.
 *          -1 if an error occurred.
 */
int pf_ppm_get_iocs (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint8_t * p_iocs,
   uint8_t * p_iocs_len);

/**
 * Set the state to "Primary" or "Backup" in the cyclic data sent to the
 * IO-Controller.
 *
 * Implements the "Local Set State" primitive.
 *
 * See Profinet 2.4 Protocol, section 4.7.2.1.3 "Coding of the field DataStatus"
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR instance.
 * @param primary          In:    true if the state is "primary".
 * @return  0  if the state was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_data_status_state (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep,
   bool primary);

/**
 * Set the redundant bit in the cyclic data sent to the IO-Controller.
 *
 * The interpretation of this bit is dependent on whether the state is
 * "Primary" or "Backup" .
 *
 * Implements the "Local Set Redundancy State" primitive.
 *
 * See Profinet 2.4 Protocol, section 4.7.2.1.3 "Coding of the field DataStatus"
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR instance.
 * @param redundant        In:    true if the state is "redundant".
 * @return  0  if the redundancy state was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_data_status_redundancy (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep,
   bool redundant);

/**
 * Set the stop/running bit in cyclic data sent to the IO-Controller.
 *
 * Implements the "Local set Provider State" primitive.
 *
 * See Profinet 2.4 Protocol, section 4.7.2.1.3 "Coding of the field DataStatus"
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR instance.
 * @param run              In:    true if the application is "running".
 * @return  0  if the provider status was set.
 *          -1 if an error occurred.
 */
int pf_ppm_set_data_status_provider (
   pnet_t * net,
   pf_ar_t * p_ar,
   uint32_t crep,
   bool run);

/**
 * Get the data status of the PPM connection.
 * @param p_ppm            In:   The PPM instance.
 * @param p_data_status    Out:  The PPM data status.
 * @return
 */
int pf_ppm_get_data_status (const pf_ppm_t * p_ppm, uint8_t * p_data_status);

/**
 * Set/Reset the station problem indicator which is included in all data
 * messages.
 *
 * @param net                 InOut: The p-net stack instance
 * @param p_ar                InOut: The AR instance.
 * @param problem_indicator   In:    The problem indicator.
 */
void pf_ppm_set_problem_indicator (
   pnet_t * net,
   pf_ar_t * p_ar,
   bool problem_indicator);

/**
 * Show information about a PPM instance.
 * @param p_ppm            In:   The PPM instance.
 */
void pf_ppm_show (const pf_ppm_t * p_ppm);

/************ Internal functions, used by PPM driver ************/

/**
 * Finalize a PPM transmit message in the send buffer.
 *
 * Insert data, cycle counter, data status and transfer status.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ppm            In:   The PPM instance.
 * @param data_length      In:   The length of the message.
 */
void pf_ppm_finish_buffer (pnet_t * net, pf_ppm_t * p_ppm, uint16_t data_length);

/**
 * Send error indications to other components.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_ppm            In:   The PPM instance.
 * @param error            In:   An error flag.
 * @return  0  always.
 */
int pf_ppm_state_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   pf_ppm_t * p_ppm,
   bool error);

/************ Internal functions, made available for unit testing ************/

uint16_t pf_ppm_calculate_cyclecounter (
   uint32_t timestamp,
   uint16_t send_clock_factor,
   uint16_t reduction_ratio);

uint16_t pf_ppm_calculate_next_cyclecounter (
   uint16_t previous_cyclecounter,
   uint16_t send_clock_factor,
   uint16_t reduction_ratio);

#ifdef __cplusplus
}
#endif

#endif /* PF_PPM_H */
