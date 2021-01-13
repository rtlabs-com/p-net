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
 *                                Typically PNET_MAX_PORT.
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
 * Get a reference to port runtime data.
 *
 * If the local port number is out of range this operation will assert.
 * NULL will never be returned.
 *
 * See also \a pf_port_get_config() for configuration of the port.
 *
 * @param net              In:    The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 * @return Address to port runtime data
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
 *                                Valid range: 1 .. PNET_MAX_PORT
 * @return Address to port configuration
 */
const pnet_port_cfg_t * pf_port_get_config (pnet_t * net, int loc_port_num);

#ifdef __cplusplus
}
#endif

#endif /* PF_PORT_H */
