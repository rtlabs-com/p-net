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

#ifndef PF_CPM_H
#define PF_CPM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the CPM component.
 * @param net              InOut: The p-net stack instance
 */
void pf_cpm_init (pnet_t * net);

/**
 * Create a CPM for a specific IOCR instance.
 *
 * This function creates a CPM instance for the specified IOCR instance.
 * Set the CPM state to W_START.
 * Allocate the buffer mutex on first call.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR index.
 * @return  0  if the CPM instance was created.
 *          -1 if an error occurred.
 */
int pf_cpm_create (pnet_t * net, pf_ar_t * p_ar, uint32_t crep);

/**
 * Close a CPM instance.
 *
 * This function terminates the specified CPM instance.
 * De-registers a frame handler for incoming Ethernet frames.
 * The buffer mutex is destroyed on the last call.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR index.
 * @return  0  if the CPM instance was closed.
 *          -1 if an error occurred.
 */
int pf_cpm_close_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep);

/**
 * Activate a CPM instance and of the specified CR instance.
 *
 * Registers a frame handler for incoming Ethernet frames.
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR index.
 * @return  0  if the CPM instance could be activated.
 *          -1 if an error occurred.
 */
int pf_cpm_activate_req (pnet_t * net, pf_ar_t * p_ar, uint32_t crep);

/**
 * Retrieve the specified sub-slot IOCS sent from the controller.
 * User must supply a buffer large enough to hold the received IOCS.
 * Maximum buffer size is 256 bytes.
 * @param net           InOut: The p-net stack instance
 * @param api_id        In:   The API identifier.
 * @param slot_nbr      In:   The slot number.
 * @param subslot_nbr   In:   The sub-slot number.
 * @param p_iocs        Out:  Copy of the received IOCS.
 * @param p_iocs_len    In:   Size of buffer at p_iocs.
 *                      Out:  The length of the received IOCS.
 * @return
 */
int pf_cpm_get_iocs (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint8_t * p_iocs,
   uint8_t * p_iocs_len);

/**
 * Retrieve the specified sub-slot data and IOPS received from the controller.
 * User must supply a buffer large enough to hold the received data.
 * Maximum buffer size is PF_FRAME_BUFFER_SIZE (1500) bytes.
 * User must supply a buffer large enough to hold the received IOPS.
 * Maximum buffer size is 256 bytes.
 *
 * @param net           InOut: The p-net stack instance
 * @param api_id        In:   The API identifier.
 * @param slot_nbr      In:   The slot number.
 * @param subslot_nbr   In:   The sub-slot number.
 * @param p_new_flag    Out:  true means new valid data (and IOPS) frame
 *                            available since last call.
 * @param p_data        Out:  Copy of the received data.
 * @param p_data_len    In:   Buffer size.
 *                      Out:  Length of received data.
 * @param p_iops        Out:  The received IOPS.
 * @param p_iops_len    In:   Size of buffer at p_iops.
 *                      Out:  The length of the received IOPS.
 * @return  0  if the data and IOPS could be retrieved.
 *          -1 if an error occurred.
 */
int pf_cpm_get_data_and_iops (
   pnet_t * net,
   uint32_t api_id,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   bool * p_new_flag,
   uint8_t * p_data,
   uint16_t * p_data_len,
   uint8_t * p_iops,
   uint8_t * p_iops_len);

/**
 * Get the data status of the CPM connection.
 * @param p_cpm            In:   The CPM instance.
 * @param p_data_status    Out:  The CPM data status.
 * @return
 */
int pf_cpm_get_data_status (const pf_cpm_t * p_cpm, uint8_t * p_data_status);

/**
 * Show information about a CPM instance.
 * @param net              In:    The p-net stack instance
 * @param p_cpm            In:   The CPM instance.
 */
void pf_cpm_show (const pnet_t * net, const pf_cpm_t * p_cpm);

/* Functions used by cpm driver */

/**
 * Change the CPM state.
 * @param p_spm            InOut: The CPM instance.
 * @param state            In:    The new CPM state.
 */
void pf_cpm_set_state (pf_cpm_t * p_cpm, pf_cpm_state_values_t state);

/**
 * Notify other components about CPM events.
 * @param net              InOut: The p-net stack instance
 * @param p_ar             InOut: The AR instance.
 * @param crep             In:    The IOCR index
 * @param start            In:    Start/Stop indicator. True if CPM is starting.
 */
void pf_cpm_state_ind (pnet_t * net, pf_ar_t * p_ar, uint32_t crep, bool start);

/**
 * Perform a check of the source address of the received frame.
 * @param p_cpm            In:    The CPM instance.
 * @param p_buf            In:    The frame buffer.
 * @return 0 if the source address is OK.
 *         -1 if the source address is wrong.
 */
int pf_cpm_check_src_addr (const pf_cpm_t * p_cpm, const pnal_buf_t * p_buf);

/************ Internal functions, made available for unit testing ************/

/**
 * Perform the cycle counter check of the received frame.
 *
 * Profinet 2.4, section 4.7.2.1.2
 *
 * In general, the current counter value should be larger than the
 * previous counter value.
 *
 * However as we use cyclic counters, also allow it to be a lot less (not the
 * same or slightly less).
 *
 * @param prev             In:   The previous cycle counter.
 * @param now              In:   The current cycle counter.
 * @return  0  If the cycle check is OK.
 *          -1 if the cycle check fails.
 */
int pf_cpm_check_cycle (int32_t prev, uint16_t now);

#ifdef __cplusplus
}
#endif

#endif /* PF_CPM_H */
