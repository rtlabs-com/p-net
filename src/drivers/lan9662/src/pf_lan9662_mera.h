/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef PF_MERA_H
#define PF_MERA_H

#ifdef __cplusplus
extern "C" {
#endif

/* PPM/CPM frame configuration.
 * The LAN9662 RTE configuration is generated from this.
 */
typedef struct pf_mera_frame_cfg
{
   pf_iocr_t * p_iocr;           /* IO-data configuration. From active AR. */
   uint16_t port;                /* RTE Interface mapping */
   pnet_ethaddr_t main_mac_addr; /* MAC address for management port */
} pf_mera_frame_cfg_t;

typedef struct pf_mera_event
{
   bool stopped;
   bool data_status_mismatch;
   bool invalid_dg;
} pf_mera_event_t;

/**
 * Allocate a mera PPM frame and its RTE resources.
 * @param net              InOut: The p-net stack instance
 * @param cfg              In:    PPM configuration
 * @return Reference to mera frame, NULL on error.
 */
pf_drv_frame_t * pf_mera_ppm_alloc (
   pnet_t * net,
   const pf_mera_frame_cfg_t * cfg);

/**
 * Free a mera PPM frame and its RTE resources.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera frame
 * @return 0 on success. -1 on error.
 */
int pf_mera_ppm_free (pnet_t * net, pf_drv_frame_t * frame);

/**
 * Start cyclic data sender / provider.
 * Calling this operation loads and starts the RTE
 * configuration defined by the frame.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera frame
 * @return 0 on success. -1 on error.
 */
int pf_mera_ppm_start (pnet_t * net, pf_drv_frame_t * frame);

/**
 * Start cyclic data sender / provider.
 * Calling this operation stops the RTE
 * configuration defined by the frame.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera frame
 * @return 0 on success. -1 on error.
 */
int pf_mera_ppm_stop (pnet_t * net, pf_drv_frame_t * frame);

/**
 * Update value of an IO object.
 * Only allowed for iodata objects mapped to SRAM.
 *
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera PPM frame
 * @param p_iodata         In:    Iodata object
 * @param data             In:    Input data
 * @param len              In:    Input data length
 * @return 0 on success, -1 on error;
 */
int pf_mera_ppm_write_input (
   pnet_t * net,
   pf_drv_frame_t * frame,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * data,
   uint32_t len);

/**
 * Update provider status of an IO object.
 * Only allowed for iodata objects mapped to SRAM.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera PPM frame
 * @param p_iodata         In:    Iodata object
 * @param iops             In:    Provider status
 * @param len              In:    Provider status length
 * @return 0 on success, -1 on error;
 */
int pf_mera_ppm_write_iops (
   pnet_t * net,
   pf_drv_frame_t * frame,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * iops,
   uint8_t len);

/**
 * Update consumer status of an IO object.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera PPM frame
 * @param p_iodata         In:    Iodata object
 * @param iocs             In:    Consumer status
 * @param len              In:    Consumer status length
 * @return 0 on success, -1 on error;
 */
int pf_mera_ppm_write_iocs (
   pnet_t * net,
   pf_drv_frame_t * frame,
   const pf_iodata_object_t * p_iodata,
   const uint8_t * iops,
   uint8_t len);

/**
 * Update PPM frame data status.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera PPM frame
 * @param data_status      In:    Data status
 * @return 0 on success, -1 on error;
 */
int pf_mera_ppm_write_data_status (
   pnet_t * net,
   pf_drv_frame_t * frame,
   uint8_t data_status);

/**
 * Retrieve the data and IOPS for a sub-module.
 * Note that this is not from the PLC, but previously set by the application.
 * @param net              InOut: The p-net stack instance
 * @param api_id           In:    The API id.
 * @param p_iodata         In:    Iodata object
 * @param p_data           Out:   The application data.
 * @param p_data_len       In:    The length of the p_data buffer.
 * @param p_iops           Out:   The IOPS of the application data.
 * @param p_iops_len       In:    Size of buffer at p_iops.
 * @return  0  if the input data and IOPS could e retrieved.
 *          -1 if an error occurred.
 */
int pf_mera_ppm_read_data_and_iops (
   pnet_t * net,
   pf_drv_frame_t * frame,
   const pf_iodata_object_t * p_iodata,
   uint8_t * p_data,
   uint16_t data_len,
   uint8_t * p_iops,
   uint8_t iops_len);

/**
 * Allocate a mera PPM frame and its RTE resources.
 * @param net              InOut: The p-net stack instance
 * @param cfg              In:    CPM configuration
 * @return Reference to mera frame, NULL on error.
 */
pf_drv_frame_t * pf_mera_cpm_alloc (
   pnet_t * net,
   const pf_mera_frame_cfg_t * cfg);

/**
 * Free a mera CMP frame and its RTE resources.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera frame
 * @return 0 on success. -1 on error.
 */
int pf_mera_cpm_free (pnet_t * net, pf_drv_frame_t * frame);

/**
 * Start cyclic data receiver / consumer.
 * Calling this operation loads and starts the RTE
 * configuration defined by the frame.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera frame
 * @return 0 on success. -1 on error.
 */
int pf_mera_cpm_start (pnet_t * net, pf_drv_frame_t * frame);

/**
 * Start cyclic data receiver / consumer.
 * Calling this operation stops the RTE
 * configuration defined by the frame.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera frame
 * @return 0 on success. -1 on error.
 */
int pf_mera_cpm_stop (pnet_t * net, pf_drv_frame_t * frame);

/**
 * Retrieve latest sub-slot data and IOPS received from the controller.
 * This function may be called to retrieve the latest data and IOPS values
 * of a sub-slot sent from the controller.
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera frame
 * @param slot             In:    The slot.
 * @param subslot          In:    The sub-slot.
 * @param p_new_flag       InOut: Not updated by this function
 * @param p_data           Out:   The received data.
 * @param data_len         In:    Size of data.
 * @param p_iops           Out:   The controller provider status (IOPS).
 *                                See pnet_ioxs_values_t
 * @param iops_len         In:    Size of IOPS
 * @return  0  if a sub-module data and IOPS is retrieved.
 *          -1 if an error occurred.
 */
int pf_mera_cpm_read_data_and_iops (
   pnet_t * net,
   pf_drv_frame_t * frame,
   uint16_t slot,
   uint16_t subslot,
   bool * p_new_flag,
   uint8_t * p_data,
   uint16_t data_len,
   uint8_t * p_iops,
   uint8_t iops_len);

/**
 * Fetch the controller consumer status of one sub-slot.
 *
 * This function may be called to retrieve the IOCS value of a sub-slot
 * sent from the controller.
 *
 * TODO - Always returns hardcoded GOOD
 *
 * @param net              InOut: The p-net stack instance
 * @param frame            InOut: Mera frame
 * @param slot_nbr         In:    The slot.
 * @param subslot_nbr      In:    The sub-slot.
 * @param p_iocs           Out:   The controller consumer status (GOOD or BAD).
 *                                See pnet_ioxs_values_t
 * @param iocs_len         In:    The IOCS length
 * @return  0  if a sub-module IOCS was set.
 *          -1 if an error occurred.
 */
int pf_mera_cpm_read_iocs (
   pnet_t * net,
   pf_drv_frame_t * frame,
   uint16_t slot_nbr,
   uint16_t subslot_nbr,
   uint8_t * p_iocs,
   uint8_t iocs_len);

/**
 * Get mera CPM events
 * @param net               InOut: The p-net stack instance
 * @param frame             InOut: Mera frame
 * @param p_data_status     Out:   CPM Events.
 * @return  0  if events was successfully retrieved
 *          -1 if an error occurred.
 */
int pf_mera_poll_cpm_events (
   pnet_t * net,
   pf_drv_frame_t * frame,
   pf_mera_event_t * event);

/**
 * Get the data status of a CPM connection.
 * @param frame            InOut: Mera frame
 * @param p_data_status    Out:   The CPM data status.
 * @return  0  if data_status was successfully retrieved
 *          -1 if an error occurred.
 */
int pf_mera_cpm_read_data_status (
   pf_drv_frame_t * frame,
   uint8_t * p_data_status);

/**
 * Read the number of received frames. Calling this operation
 * resets the counter and the returned value is the number of
 * received frame since last call to this function.
 * @param frame            InOut: Mera frame
 * @param rx_count         Out:   Number of received frames since last call.
 * @return  0  if frame counter was successfully read
 *          -1 if an error occurred.
 */
int pf_mera_cpm_read_rx_count (pf_drv_frame_t * frame, uint64_t * rx_count);

/**
 * Enable the RTE frame received timeout.
 * A RTE stopped event will occur if no io data frames are
 * received within the DHT period.
 *
 * @param frame           InOut: CPM frame
 * @return  0  if RTP ID was successfully updated
 *          -1 if an error occurred.
 */
int pf_mera_cpm_activate_rte_dht (pf_drv_frame_t * drv_frame);

/**
 * Get active status for a frame. A frame is active
 * if the corresponding RTE configuration is running.
 * @param frame           In: PPM or CPM frame
 * @return true if frame is active
 *         false in not
 */
bool pf_mera_is_active_frame (pf_drv_frame_t * frame);

/**
 * Show the status of the pf_mera component
 * @param net              InOut: The p-net stack instance
 * @param cpm              In: Show CPM info
 * @param ppm              In: Show PPM info
 */
void pf_mera_show (pnet_t * net, bool cpm, bool ppm);

/**
 * Utility for finding the RTE port on which a
 * mac address is found when a network bridge is used.
 * If no bridge is configured operation fails and *p_port
 * is set to 0.
 * @param mac_address      In:  Controller mac address
 * @param p_port           Out: RTE port on which controller is found
 * @return 0 on success, -1 on error
 */
int pf_mera_get_port_from_remote_mac (
   pnet_ethaddr_t * mac_address,
   uint16_t * p_port);

#ifdef __cplusplus
}
#endif

#endif /* PF_MERA_H */
