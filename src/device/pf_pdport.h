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
 * Initialize PDPort.
 * Load PDPortport configuration from nvm.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred
 */
int pf_pdport_init (pnet_t * net);

/**
 * Reset PDPort configuration data for all ports.
 * Clear PDPort configuration in nvm.
 * Remove all active diagnostics items related to PDPort.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if not found or an error occurred
 */
int pf_pdport_reset_all (pnet_t * net);

/**
 * Remove configuration files for all ports.
 *
 * @param  * @param file_directory   In:    File directory
 */
void pf_pdport_remove_data_files (const char * file_directory);

/**
 * Notify PDPort that a new AR has been set up.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 */
void pf_pdport_ar_connect_ind (pnet_t * net, const pf_ar_t * p_ar);

/**
 * Restart LLDP transmission
 *
 * The current PDPort port adjustments is checked.
 * @param net              InOut: The p-net stack instance
 */
void pf_pdport_lldp_restart_transmission (pnet_t * net);

/**
 * Read a PDPort data record/index
 * Record to read defined by index in request
 * Parse request and write response to output buffer.
 * The following indexes are supported:
 *   - PF_IDX_SUB_PDPORT_DATA_REAL
 *   - PF_IDX_SUB_PDPORT_DATA_ADJ
 *   - PF_IDX_SUB_PDPORT_DATA_CHECK
 *   - PF_IDX_DEV_PDREAL_DATA
 *   - PF_IDX_DEV_PDEXP_DATA
 *   - PF_IDX_SUB_PDPORT_STATISTIC
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param p_read_req       In:    The read request.
 * @param p_read_res       In:    The result information.
 * @param res_size         In:    The size of the output buffer.
 * @param p_res            Out:   The output buffer.
 * @param p_pos            InOut: Position in the output buffer.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_pdport_read_ind (
   pnet_t * net,
   pf_ar_t * p_ar,
   const pf_iod_read_request_t * p_read_req,
   const pf_iod_read_result_t * p_read_res,
   uint16_t res_size,
   uint8_t * p_res,
   uint16_t * p_pos);

/**
 * Write a PDPort data record/index
 * Record to write defined by index in request
 * The following indexes are supported:
 *   - PF_IDX_SUB_PDPORT_DATA_REAL
 *   - PF_IDX_SUB_PDPORT_DATA_ADJ
 *   - PF_IDX_SUB_PDPORT_DATA_CHECK
 *   - PF_IDX_DEV_PDREAL_DATA
 *   - PF_IDX_DEV_PDEXP_DATA
 *   - PF_IDX_SUB_PDPORT_STATISTIC
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             In:    The AR instance.
 * @param p_write_req      In:    The IODWrite request.
 * @param p_req_buf        In:    The request buffer.
 * @param data_length      In:    Size of the data to write.
 * @param p_result         Out:   Detailed error information.
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_pdport_write_req (
   pnet_t * net,
   const pf_ar_t * p_ar,
   const pf_iod_write_request_t * p_write_req,
   const uint8_t * p_req_buf,
   uint16_t data_length,
   pnet_result_t * p_result);

/**
 * Notify PDPort that the LLDP peer information has changed.
 *
 * PDPort will later verify that correct peer is connected to the port.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 */
void pf_pdport_peer_indication (pnet_t * net, int loc_port_num);

/**
 * Start Ethernet link monitoring
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_pdport_start_linkmonitor (pnet_t * net);

/**
 * Run PDPort observers.
 * Run enabled checks and set diagnoses.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_pdport_periodic (pnet_t * net);

/**
 * Save PDPort configuration to NVM for all ports.
 * This function is intended to be executed in the background task,
 * as the system calls to save files may be blocking.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if operation succeeded.
 *          -1 if an error occurred.
 */
int pf_pdport_save_all (pnet_t * net);

/**
 * Verify that at least one operational port has a speed of at
 * least 100 MBit/s, full duplex.
 *
 * Otherwise we should refuse to make a connection.
 *
 * To support the special case of devices with 10 Mbit/s, this function must be
 * modified.
 *
 * See PdevCheckFailed() in Profinet 2.4 Protocol 5.6.3.2.1.4 CMDEV state table
 *
 * @param net              InOut: The p-net stack instance
 * @return true if at least one port has the required speed, else false.
 */
bool pf_pdport_is_a_fast_port_in_use (pnet_t * net);

/**
 * Update the ethernet status for all ports.
 * This function is intended to be executed in a background task,
 * as the system calls to read the ethernet status may be blocking.
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_pdport_update_eth_status (pnet_t * net);

/**
 * Get current ethernet status for local port.
 * This is a non-blocking function reading from mutex-protected memory.
 * The status values are updated by the background worker task
 * in a periodic job executing the pf_pdport_update_eth_status() function.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @return The ethernet status for local port
 */
pnal_eth_status_t pf_pdport_get_eth_status (pnet_t * net, int loc_port_num);

/**
 * Get current ethernet status for local port.
 * This is a non-blocking function reading from mutex-protected memory.
 * The status values are updated by the background worker task
 * in a periodic job executing the pf_pdport_update_eth_status() function.
 *
 * The MAU type value in the resulting \a pnal_eth_status_t struct
 * will be replaced by the default MAU type from the configuration
 * when the link is down (and the actual MAU type not can be read from
 * hardware).
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @return The ethernet status for local port
 */
pnal_eth_status_t pf_pdport_get_eth_status_filtered_mau (
   pnet_t * net,
   int loc_port_num);

#ifdef __cplusplus
}
#endif

#endif /* PF_PDPORT_H */
