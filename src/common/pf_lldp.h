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

/**
 * @file
 * @brief Periodical transmission outgoing LLDP frames
 *        Reception of incoming LLDP frames
 */

#ifndef PF_LLDP_H
#define PF_LLDP_H

#ifdef __cplusplus
extern "C" {
#endif

#define PF_LLDP_SUBTYPE_MAC              4
#define PF_LLDP_SUBTYPE_LOCALLY_ASSIGNED 7

/**
 * Get time when new information about remote device was received.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_timestamp_10ms Out:   Time when the LLDP packet with the info
 *                                was first received, in units of
 *                                10 milliseconds with system startup
 *                                as the zero point (as in SNMP sysUpTime).
 *                                Note that if an LLDP packet with identical
 *                                info is received, the timestamp is not
 *                                updated.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_timestamp (
   pnet_t * net,
   int loc_port_num,
   uint32_t * p_timestamp_10ms);

/**
 * Get LLDP port configuration for a port.
 *
 * If the local port number is out of range this operation will assert.
 * NULL will never be returned.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
const pnet_lldp_port_cfg_t * pf_lldp_get_port_config (
   pnet_t * net,
   int loc_port_num);

/**
 * Get Chassis ID of local device.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.2 "Chassis ID TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param p_chassis_id     Out:   Chassis ID of local device.
 */
void pf_lldp_get_chassis_id (pnet_t * net, pf_lldp_chassis_id_t * p_chassis_id);

/**
 * Get Chassis ID for remote device connected to local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.2 "Chassis ID TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_chassis_id     Out:   Chassis ID of remote device.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_chassis_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_chassis_id_t * p_chassis_id);

/**
 * Get Port ID for local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.3 "Port ID TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_port_id        Out:   Port ID of local port.
 */
void pf_lldp_get_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id);

/**
 * Get Port ID for remote port.
 *
 * Note that the remote device may have multiple ports. Only the remote
 * port connected to the local port is relevant here.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.3 "Port ID TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_port_id        Out:   Port ID of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_port_id (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_id_t * p_port_id);

/**
 * Get port description for local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.5 "Port Description TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_port_desc      Out:   Port description of local port.
 */
void pf_lldp_get_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc);

/**
 * Get port description for remote port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.5 "Port Description TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_port_desc      Out:   Port description of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_port_description (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_port_description_t * p_port_desc);

/**
 * Get Management Address for local interface.
 *
 * The management address should usually be the IP address for the local
 * interface the local ports belong to. It could also be the MAC address of
 * the local interface in case no IP address has been assigned.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param p_man_address    Out:   Management address of local interface.
 */
void pf_lldp_get_management_address (
   pnet_t * net,
   pf_lldp_management_address_t * p_man_address);

/**
 * Get Management Address for remote interface connected to local port.
 *
 * The management address should usually be the IP address for the remote
 * interface the remote port belongs to. It could also be the MAC address of
 * the remote interface in case no IP address has been assigned.
 *
 * Note that the remote device may have multiple interfaces. Only the remote
 * interface connected to the local port is relevant here.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_man_address    Out:   Management address of remote interface.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_management_address (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_address_t * p_man_address);

/**
 * Get ManAddrIfId for local interface.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param p_man_port_index Out:   Index in IfTable for Management port.
 */
void pf_lldp_get_management_port_index (
   pnet_t * net,
   pf_lldp_management_port_index_t * p_man_port_index);

/**
 * Get ManAddrIfId for remote interface connected to local port.
 *
 * Note that the remote device may have multiple interfaces. Only the remote
 * interface connected to the local port is relevant here.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) ch. 9.5.9 "Management Address TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_man_port_index Out:   Index in remote IfTable for Management port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_management_port_index (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_management_port_index_t * p_man_port_index);

/**
 * Get remote station name for remote interface connected to local port.
 *
 * The station name is usually a string, but may also be the MAC address of
 * the remote interface in case no string has been assigned.
 *
 * Note that the remote device may have multiple interfaces. Only the remote
 * interface connected to the local port is relevant here.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_station_name   Out:   Station name of remote interface.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_station_name (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_station_name_t * p_station_name);

/**
 * Get measured signal delays on local port.
 *
 * If a signal delay was not measured, its value is zero.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol): LLDP_PNIO_DELAY.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_delays         Out:   Measured signal delays on local port.
 */
void pf_lldp_get_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays);

/**
 * Get measured signal delays on remote port.
 *
 * If a signal delay was not measured, its value is zero.
 *
 * See IEC CDV 61158-6-10 (PN-AL-Protocol): LLDP_PNIO_DELAY.
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_delays         Out:   Measured signal delays on remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_signal_delays (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_signal_delay_t * p_delays);

/**
 * Get link status of local port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) Annex G.2 "MAC/PHY Configuration/Status TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_link_status    Out:   Link status of local port.
 */
void pf_lldp_get_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status);

/**
 * Get link status of remote port.
 *
 * See IEEE 802.1AB-2005 (LLDPv1) Annex G.2 "MAC/PHY Configuration/Status TLV".
 *
 * @param net              In:    The p-net stack instance.
 * @param loc_port_num     In:    Local port number for port directly
 *                                connected to the remote device.
 *                                Valid range: 1 .. PNET_MAX_PORT
 *                                See pf_port_get_list_of_ports().
 * @param p_link_status    Out:   Link status of remote port.
 * @return  0 if the operation succeeded.
 *         -1 if an error occurred (no info from remote device received).
 */
int pf_lldp_get_peer_link_status (
   pnet_t * net,
   int loc_port_num,
   pf_lldp_link_status_t * p_link_status);

/**
 * Initialize the LLDP component.
 *
 * This function initializes the LLDP component and
 * sends the initial LLDP message.
 * @param net              InOut: The p-net stack instance
 */
void pf_lldp_init (pnet_t * net);

/**
 * @internal
 * Start or restart a timer that monitors reception of LLDP frames from peer.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 * @param timeout_in_secs  In:    TTL of the peer, typically 20 seconds.
 */
void pf_lldp_reset_peer_timeout (
      pnet_t * net,
      int loc_port_num,
      uint16_t timeout_in_secs);

/**
 * Enable send of lldp frames on local port
 *
 * An initial lldp frame is transmitted when operation is called.
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
void pf_lldp_send_enable (pnet_t * net, int loc_port_num);

/**
 * Disable send of lldp frames on local port
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
void pf_lldp_send_disable (pnet_t * net, int loc_port_num);

/**
 * Build and send a LLDP message on a specific port.
 *
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 */
void pf_lldp_send (pnet_t * net, int loc_port_num);

/**
 * Restart LLDP transmission timer and optionally send LLDP frame.
 * @param net              InOut: The p-net stack instance
 * @param loc_port_num     In:    Local port number.
 *                                Valid range: 1 .. PNET_MAX_PORT
 * @param send             In:    Send LLDP message
 */
void pf_lldp_tx_restart (pnet_t * net, int loc_port_num, bool send);

/**
 * Receive an LLDP message.
 *
 * Parse LLDP tlv format and store selected information.
 * Trigger alarms if needed.
 * @param net              InOut: The p-net stack instance
 * @param p_frame_buf      In:    The Ethernet frame
 * @param offset           In:    The offset to start of LLDP data
 * @return  0     If the frame was NOT handled by this function.
 *          1     If the frame was handled and the buffer freed.
 */
int pf_lldp_recv (pnet_t * net, pnal_buf_t * p_frame_buf, uint16_t offset);

/************ Internal functions, made available for unit testing ************/

int pf_lldp_generate_alias_name (
   const char * port_id,
   const char * chassis_id,
   char * alias,
   uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* PF_LLDP_H */
