/*
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA512

p-net Profinet device stack
===========================

Copyright 2018 rt-labs AB

This instance of p-net is licensed for use in a specific product, as
follows:

Company: Bender GmbH & Co. KG
Product: COMTRAXX
Product description: COMTRAXX products provided by Bender GmbH & Co. KG

Use of this instance of p-net in other products is not allowed.

You are hereby given a non-exclusive, non-transferable license to use
the SOFTWARE and documentation according to the SOFTWARE LICENSE
AGREEMENT document shipped together with this instance of the
SOFTWARE. The SOFTWARE LICENSE AGREEMENT prohibits copying and
redistribution of the SOFTWARE.  It prohibits reverse-engineering
activities.  It limits warranty and liability on behalf of rt-labs.
The details of these terms are explained in subsequent chapters of the
SOFTWARE LICENSE AGREEMENT.
-----BEGIN PGP SIGNATURE-----

iQEzBAEBCgAdFiEE1e4U+gPgGJXLsxxLZlG0bopmiXIFAl/TMvQACgkQZlG0bopm
iXJE1QgAlEKblKkIrLF+hVRm6I1IfbGV6VWnBx+9IPwaHZmgQZAVTOlp4RHiNAre
Il1l1xheENWpuihbVlvz5hMCVto4fkT13IUXxOo9Va9MK8lm6JOu2lYpl1HgVIwV
R6I6dsWLeNBSBaA38DDBHIfjP4LBZC/HVYO2ISc3/OKutqasz/WIp+AjAURiKRsf
yaWGay88FITHymmqJpGaWzB+jAOvRFs8hW/GsOyX/r17AK5F5N51O+UqUTKAWNPT
f6M8QF0b07Hw+eOuB7A/0D5nvSl/rOd9LvYkiUhIUmQdHqERNkF9EJdroIjxTMXi
lmoOkPEqyGV58fh7JjMYlI60sPlGBw==
=TaN+
-----END PGP SIGNATURE-----
*/

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
 * Show interface statistics
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_cmina_interface_statistics_show (const pnet_t * net);

/**
 * Show the CMINA status.
 * @param net              InOut: The p-net stack instance
 */
void pf_cmina_show (pnet_t * net);

/**
 * Convert IPv4 address to string
 * @param ip               In:    IP address
 * @param outputstring     Out:   Resulting string. Should have length
 *                                PNAL_INET_ADDRSTRLEN.
 */
void pf_cmina_ip_to_string (pnal_ipaddr_t ip, char * outputstring);

/**
 * Retrieve the path to the directory for saving files.
 * @param net                 InOut: The p-net stack instance
 * @param pp_file_directory   Out:   The absolute path to the file directory.
 *                                   Terminated string or NULL.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_get_file_directory (pnet_t * net, const char ** pp_file_directory);

/**
 * Retrieve the current station name of the device.
 * @param net              InOut: The p-net stack instance
 * @param pp_station_name  Out:   The station name. Terminated string.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_get_station_name (pnet_t * net, const char ** pp_station_name);

/**
 * Retrieve the current IP address of the device.
 * @param net              InOut: The p-net stack instance
 * @param p_ipaddr         Out:   The ip_address.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_get_ipaddr (pnet_t * net, pnal_ipaddr_t * p_ipaddr);

/**
 * Retrieve the device MAC address.
 * @param net              InOut: The p-net stack instance
 * @param p_macaddr        Out:   The MAC address.
 * @return  0  if the operation succeeded.
 *          -1 if an error occurred.
 */
int pf_cmina_get_device_macaddr (pnet_t * net, pnet_ethaddr_t * p_macaddr);

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
