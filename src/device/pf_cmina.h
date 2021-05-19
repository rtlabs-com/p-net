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

#ifndef PF_CMINA_H
#define PF_CMINA_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the CMINA component.
 *
 * Sets the IP address if necessary.
 *
 * Schedules sending HELLO frames.
 *
 * @param net              InOut: The p-net stack instance
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_init (pnet_t * net);

/**
 * Remove the stack's data files.
 *
 * @param file_directory   In:    File directory
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_remove_all_data_files (const char * file_directory);

/**
 * Show port statistics
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_cmina_port_statistics_show (pnet_t * net);

/**
 * Show the CMINA status.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmina_show (pnet_t * net);

/**
 * Convert IPv4 address to string
 * @param ip               In:    IP address
 * @param outputstring     Out:   Buffer with resulting string. Should have
 *                                size PNAL_INET_ADDRSTR_SIZE.
 */
void pf_cmina_ip_to_string (pnal_ipaddr_t ip, char * outputstring);

/**
 * Retrieve the path to the directory for saving files.
 * @param net              In:    The p-net stack instance
 * @return the absolute path to the file directory.
 *         Terminated string or NULL.
 */
const char * pf_cmina_get_file_directory (const pnet_t * net);

/**
 * Retrieve the current station name of the device.
 *
 * TODO: Not yet thread safe. Fix this, e.g. using a mutex.
 *
 * @param net              In:    The p-net stack instance
 * @param station_name     Out:   Resulting string buffer with a terminated
 *                                string. Should have size
 *                                PNET_STATION_NAME_MAX_SIZE.
 */
void pf_cmina_get_station_name (const pnet_t * net, char * station_name);

/**
 * Retrieve the current IP address of the device.
 * @param net              In:    The p-net stack instance
 * @return the ip_address.
 */
pnal_ipaddr_t pf_cmina_get_ipaddr (const pnet_t * net);

/**
 * Retrieve the current netmask for the device.
 * @param net              In:    The p-net stack instance
 * @return the netmask
 */
pnal_ipaddr_t pf_cmina_get_netmask (const pnet_t * net);

/**
 * Retrieve the gateway address.
 * @param net              In:    The p-net stack instance
 * @return the gateway address.
 */
pnal_ipaddr_t pf_cmina_get_gateway (const pnet_t * net);

/**
 * Retrieve the device MAC address.
 * @param net              In:    The p-net stack instance
 * @return the MAC address
 */
const pnet_ethaddr_t * pf_cmina_get_device_macaddr (const pnet_t * net);

/**
 * Save one block of data for incoming DCP set command.
 *
 * Triggers the \a pnet_reset_ind() callback for some incoming DCP set commands.
 *
 * @param net              InOut: The p-net stack instance
 * @param opt              In:    The option key.
 * @param sub              In:    The sub-option key.
 * @param block_qualifier  In:    The block qualifier.
 * @param value_length     In:    The length of the data to set.
 * @param p_value          In:    The data to set.
 * @param p_block_error    Out:   The block error code, if any.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_dcp_set_ind (
   pnet_t * net,
   uint8_t opt,
   uint8_t sub,
   uint16_t block_qualifier,
   uint16_t value_length,
   const uint8_t * p_value,
   uint8_t * p_block_error);

/**
 * Reset the configuration to default values.
 *
 * Triggers the application callback \a pnet_reset_ind() for some \a reset_mode
 * values.
 *
 * Reset modes:
 *
 * 0:  Power-on reset
 * 1:  Reset application parameters
 * 2:  Reset communication parameters
 * 99: Reset application and communication parameters.
 *
 * Populates net->cmina_nonvolatile_dcp_ase from file (and/or from default
 * configuration), and copies the resulting values to net->cmina_current_dcp_ase
 *
 * In order to actually change IP etc, the function pf_cmina_dcp_set_commit()
 * must be called afterwards.
 *
 * @param net              InOut: The p-net stack instance
 * @param reset_mode       In:    Reset mode.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_set_default_cfg (pnet_t * net, uint16_t reset_mode);

/**
 * Commit changes to the IP-suite.
 *
 * This shall be done _after_ the answer to DCP set has been sent.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmina_dcp_set_commit (pnet_t * net);

/**
 * Find data and its size for use in one block in response to a DCP get command.
 *
 * Also used in DCP Hello and DCP Identify.
 *
 * @param net              InOut: The p-net stack instance
 * @param opt              In:    The option key.
 * @param sub              In:    The sub-option key.
 * @param value_length     In:    The length of the pp_value buffer.
 *                         Out:   The length of the read data.
 * @param pp_value         Out:   The retrieved data.
 * @param p_block_error    Out:   The block error code, if any.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_dcp_get_req (
   pnet_t * net,
   uint8_t opt,
   uint8_t sub,
   uint16_t * p_value_length,
   uint8_t ** pp_value,
   uint8_t * p_block_error);

/**
 * Check if a time interval has passed
 *
 * @param now_us           In:    Interval end in microseconds
 * @param previous_us      In:    Interval start in microseconds
 * @param period           In:    Time interval length in 1/32 of millisecond.
 *                                For 1 millisecond use 32
 *                                    2 milliseconds use 64 etc
 * @param factor           In:    Multiplication factor for the timeout.
 *                                Use 1 if the given time interval length should
 *                                be used, or 3 if we should return true after
 *                                3 time interval lengths etc
 * @return  true if the measured time interval is longer than period*factor
 *          false if it not has passed
 */
bool pf_cmina_has_timed_out (
   uint32_t now_us,
   uint32_t previous_us,
   uint16_t period,
   uint16_t factor);

/**
 * Save the IP settings (and station name) to nonvolatile memory if necessary.
 * This is a blocking call intended to be executed by the background worker.
 *
 * Compares with the content of already stored settings (in order not to
 * wear out the flash chip)
 *
 * @param net              InOut: The p-net stack instance
 * @param p_ase            In:    Settings to be saved
 */
void pf_cmina_save_ase (pnet_t * net, pf_cmina_dcp_ase_t * p_ase);

/************ Internal functions, made available for unit testing ************/

bool pf_cmina_is_stationname_valid (const char * station_name, uint16_t len);

bool pf_cmina_is_netmask_valid (pnal_ipaddr_t netmask);

bool pf_cmina_is_ipaddress_valid (pnal_ipaddr_t netmask, pnal_ipaddr_t ip);

bool pf_cmina_is_gateway_valid (
   pnal_ipaddr_t ip,
   pnal_ipaddr_t netmask,
   pnal_ipaddr_t gateway);

bool pf_cmina_is_ipsuite_valid (const pf_ip_suite_t * p_ipsuite);

bool pf_cmina_is_full_ipsuite_valid (const pf_full_ip_suite_t * p_full_ipsuite);

#ifdef __cplusplus
}
#endif

#endif /* PF_CMINA_H */
