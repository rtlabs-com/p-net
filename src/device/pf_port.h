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
 * Get list of local ports.
 *
 * This is the list of physical ports on the local interface.
 * The management port is not included.
 *
 * @param net              In:    The p-net stack instance.
 * @param max_port         In:    Max port number supported.
 *                                Typically PNET_NUMBER_OF_PHYSICAL_PORTS.
 *                                Parameter added for testability.
 * @param p_list           Out:   List of local ports.
 */
void pf_port_get_list_of_ports (
   pnet_t * net,
   uint16_t max_port,
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
   pnet_t * net,
   pf_port_iterator_t * p_iterator);

/**
 * Get next local port.
 *
 * If no more ports are available, 0 is returned.
 *
 * @param p_iterator       InOut: Port iterator.
 * @return Local port number for next port on local interface.
 *         If no more ports are available, 0 is returned.
 */
int pf_port_get_next (pf_port_iterator_t * p_iterator);

/**
 * Get DAP port subslot using local port number
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range:
 *                                1 .. PNET_NUMBER_OF_PHYSICAL_PORTS.
 * @return DAP subslot number for port identity
 */
uint16_t pf_port_loc_port_num_to_dap_subslot (int loc_port_num);

/**
 * Check if a DAP port subslot is mapped to a local port
 *
 * @param subslot              In: Subslot number
 * @return true  if the subslot is mapped to a local port.
 *         false if the subslot is not supported.
 */
bool pf_port_subslot_is_dap_port_id (uint16_t subslot);

/**
 * Get local port from DAP port subslot
 *
 * Considers PNET_NUMBER_OF_PHYSICAL_PORTS
 *
 * @param subslot              In: Subslot number
 * @return The port number mapping to the subslot.
 *         0 if the subslot is not supported.
 */
int pf_port_dap_subslot_to_local_port (uint16_t subslot);

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
 *                                Valid range:
 *                                1 .. PNET_NUMBER_OF_PHYSICAL_PORTS.
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
 *                                Valid range:
 *                                1 .. PNET_NUMBER_OF_PHYSICAL_PORTS
 * @return port configuration
 */
const pnet_port_cfg_t * pf_port_get_config (pnet_t * net, int loc_port_num);

/**
 * Check if port number corresponds to a valid physical port.
 *
 * @param net              In:    The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range:
 *                                1 .. PNET_NUMBER_OF_PHYSICAL_PORTS.
 * @return  true  if port number is valid,
 *          false if not
 */
bool pf_port_is_valid (pnet_t * net, int loc_port_num);

/**
 * Get port number for network interface. If network interface
 * is not a physical port, zero is returned.
 *
 * @param net              In:    The p-net stack instance.
 * @param eth_handle       In:    Network interface handle.
 * @return local port number
 *         0 if no local port matches the network interface handle
 */
int pf_port_get_port_number (pnet_t * net, pnal_eth_handle_t * eth_handle);

/**
 * Decode media type from Ethernet MAU type.
 * Media types listed in PN-AL-protocol (Mar20) Table 727.
 * @param mau_type         In:   Ethernet MAU type
 * @return media type
 */
pf_mediatype_values_t pf_port_get_media_type (pnal_eth_mau_t mau_type);

#ifdef __cplusplus
}
#endif

#endif /* PF_PORT_H */
