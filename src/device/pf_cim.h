/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2025 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef PF_CIM_H
#define PF_CIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pf_types.h"

/**
 * Writes electric power data for the device to the input pointer, based
 * on data returned by the application.
 *
 * Triggers the user call-back \a pnet_ep_device_ind(). Sets the voltage type
 * to 0 (currently a reserved value) if the call-back returns a failure code.
 *
 * @param net              InOut: The p-net stack instance.
 * @param slot_number      In:    The slot.
 * @param subslot_number   In:    The subslot.
 * @param device_data      InOut: Pointer to device data.
 */
void pf_cim_get_device_data (
   pnet_t * net,
   uint16_t slot_number,
   uint16_t subslot_number,
   pf_electric_power_device_data_t * device_data);

/**
 * Returns electric power data for the ports.
 *
 * The input port array should be zero-initialized.
 *
 * Triggers the user call-back \a pnet_ep_ports_ind(). Sets the voltage type
 * to 0 (power off or power supply not connected) for each entry where the
 * call-back returns a failure code.
 *
 * @param net              InOut: The p-net stack instance.
 * @param slot_number      In:    The slot.
 * @param subslot_number   In:    The subslot.
 * @param num_ports        In:    Port array size.
 * @param port_entries     InOut: Port array.
 */
void pf_cim_get_port_data (
   pnet_t * net,
   uint16_t slot_number,
   uint16_t subslot_number,
   uint16_t num_ports,
   pf_electric_power_port_data_t * port_entries);

/**
 * Profinet write request handler CIM.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_write_req      In:    The write request details
 * @param p_bytes          In:    The data to write
 * @param data_length      In:    Length of data to write
 * @param p_result         Out:   Result of the write operation
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred.
 */
int pf_cim_write_request (
   pnet_t * net,
   const pf_iod_write_request_t * p_write_req,
   const uint8_t * p_bytes,
   uint16_t data_length,
   pnet_result_t * p_result);

/**
 * Handle a write request to CIM SNMP Adjust.
 *
 * @param net            InOut: The p-net stack instance
 * @param p_write_req    In:    The write request details
 * @param p_bytes        In:    The write request data
 * @param data_length    In:    The length of the write request data
 * @param p_result       Out:   The result to return to the caller
 * @return  0 on success, -1 on error
 */
int pf_cim_snmp_adjust (
   pnet_t * net,
   const pf_iod_write_request_t * p_write_req,
   const uint8_t * p_bytes,
   uint16_t data_length,
   pnet_result_t * p_result);

#ifdef __cplusplus
}
#endif

#endif /* PF_CIM_H */
