/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef PF_PORT_H
#define PF_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init port data
 *
 * @param net              In:    The p-net stack instance.
 */
void pf_port_init (pnet_t * net);

/**
 * Init main interface data, and schedule Ethernet link monitoring
 *
 * @param net              In:    The p-net stack instance.
 */
void pf_port_main_interface_init (pnet_t * net);

/**
 * Get number of physical ports.
 *
 * @param net              In:    The p-net stack instance.
 * @return the number of physical ports.
 */
uint8_t pf_port_get_number_of_ports (const pnet_t * net);

/**
 * Get list of local ports.
 *
 * This is the list of physical ports on the local interface.
 * The management port is not included.
 *
 * @param net              In:    The p-net stack instance.
 * @param p_list           Out:   List of local ports.
 */
void pf_port_get_list_of_ports (
   const pnet_t * net,
   pf_lldp_port_list_t * p_list);

/**
 * Initialise iterator for iterating over local ports.
 *
 * This iterator may be used for iterating over all physical ports
 * on the local interface. The management port is not included.
 * See pf_port_get_next().
 *
 * @param net              In:    The p-net stack instance.
 * @param p_iterator       Out:   Port iterator.
 */
void pf_port_init_iterator_over_ports (
   const pnet_t * net,
   pf_port_iterator_t * p_iterator);

/**
 * Get next local port.
 *
 * If no more ports are available, 0 is returned.
 *
 * Will return: 1 2 3 ... num_physical_ports 0 0 0
 *
 * @param p_iterator       InOut: Port iterator.
 * @return Local port number for next port on local interface.
 *         If no more ports are available, 0 is returned.
 */
int pf_port_get_next (pf_port_iterator_t * p_iterator);

/**
 * Get next local port, over and over again.
 *
 * After highest port number has been returned, next time 1 will be returned
 * (and then 2 etc)
 *
 * Will return: 1 2 3 ... num_physical_ports 1 2 3 ... etc
 *
 * @param p_iterator       InOut: Port iterator.
 * @return Local port number for next port on local interface.
 */
int pf_port_get_next_repeat_cyclic (pf_port_iterator_t * p_iterator);

/**
 * Get DAP port subslot using local port number
 *
 * If the local port number is out of range this operation will assert.
 *
 * @param net              In:    The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range:  1 .. num_physical_ports
 * @return DAP subslot number for port identity
 */
uint16_t pf_port_loc_port_num_to_dap_subslot (
   const pnet_t * net,
   int loc_port_num);

/**
 * Check if a DAP port subslot is mapped to a local port
 *
 * @param net              In:    The p-net stack instance
 * @param subslot          In:    Subslot number
 * @return true  if the subslot is mapped to a local port.
 *         false if the subslot is not supported.
 */
bool pf_port_subslot_is_dap_port_id (const pnet_t * net, uint16_t subslot);

/**
 * Get local port from DAP port subslot
 *
 * @param net              In:    The p-net stack instance
 * @param subslot          In:    Subslot number
 * @return The port number mapping to the subslot.
 *         0 if the subslot is not supported.
 */
int pf_port_dap_subslot_to_local_port (const pnet_t * net, uint16_t subslot);

/**
 * Get port runtime data.
 *
 * If the local port number is out of range this operation will assert.
 * NULL will never be returned.
 *
 * See also \a pf_port_get_config() for configuration of the port.
 *
 * @param net              In:    The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @return port runtime data
 */
pf_port_t * pf_port_get_state (pnet_t * net, int loc_port_num);

/**
 * Get port configuration.
 *
 * If the local port number is out of range this operation will assert.
 * NULL will never be returned.
 *
 * See also \a pf_port_get_state() for port runtime data.
 *
 * @param net              In:    The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @return port configuration
 */
const pnet_port_cfg_t * pf_port_get_config (pnet_t * net, int loc_port_num);

/**
 * Check if port number corresponds to a valid physical port.
 *
 * @param net              In:    The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. num_physical_ports
 * @return  true  if port number is valid,
 *          false if not
 */
bool pf_port_is_valid (const pnet_t * net, int loc_port_num);

/**
 * Get port number for network interface. If network interface
 * is not a physical port, zero is returned.
 *
 * @param net              In:    The p-net stack instance.
 * @param eth_handle       In:    Network interface handle.
 * @return local port number
 *         0 if no local port matches the network interface handle
 */
int pf_port_get_port_number (const pnet_t * net, pnal_eth_handle_t * eth_handle);

/**
 * Decode media type from Ethernet MAU type.
 * Media types listed in PN-AL-protocol (Mar20) Table 727.
 *
 * The MAU type 0 represents "unknown" when the link is down.
 * When the link is up a MAU type 0 represents "Radio".
 *
 * This function assumes that the link is up. Make sure you use
 * some default MAU type if the link is down, before using this function.
 *
 * @param mau_type         In:   Ethernet MAU type
 * @return media type
 */
pf_mediatype_values_t pf_port_get_media_type (pnal_eth_mau_t mau_type);

/**
 * Show port details
 *
 * @param net              InOut: The p-net stack instance
 */
void pf_port_show (pnet_t * net);

#ifdef __cplusplus
}
#endif

#endif /* PF_PORT_H */
